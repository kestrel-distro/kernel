# Kernel benchmark harness

A/B harness for the Kestrel kernel and the scheduler/bridge stack. Run it once
per image, then compare.

```
bench/run stock          # booted into the stock `linux` kernel
# reboot into linux-kestrel
bench/run kestrel
bench/compare bench/results/<stock-dir> bench/results/<kestrel-dir>
```

Each run writes `bench/results/<timestamp>-<label>/`: `summary.txt` (machine
readable), the variant label, package versions, and the raw tool output.
`bench/compare` groups runs by `variant=` and prints median `[min..max]` deltas.

## Factor matrix

`bench/matrix [label]` runs the bridge factor matrix: the scheduler
(`scx_loader`), the broker/cgroups and the kernel hints are each pinned per
variant, and `KESTREL_MATRIX_REPS` repeats (default 5) are **interleaved and
rotated** so a warm-up drift cannot look like a variant effect.

| Variant | Scheduler | Broker/cgroups | Hints | Isolates |
|---|---|---|---|---|
| `default` | `scx_bpfland` | off | off | the baseline |
| `cgroups` | `scx_bpfland` | on | on | broker/cgroups alone |
| `sched` | `scx_kestrel` | off | off | scheduler alone |
| `bridge` | `scx_kestrel` | on | on | the full bridge |
| `hints` | `scx_kestrel` | on | off | hint value vs scheduler |

Variant syntax is `name:scheduler:mode:hints:cgroups`; override the set with
`KESTREL_MATRIX_VARIANTS="..."`. `KESTREL_MATRIX_MODE` (default `session`),
`KESTREL_MATRIX_SESSION_SECONDS` (default 30) and `KESTREL_MATRIX_BG_CPUS`
(default 0) pass through to `bench/run`; the background CPU hog is what makes
the interactive classes actually queue.

Each result carries `packages=` and `kernel=` context (`kestrel` or `stock`,
auto-detected; override with `KESTREL_MATRIX_LABEL`). Comparing across images is
then just `bench/compare results/matrix-*/*`, which groups by the full variant
string including context. "Default" for the overall claim means **stock Arch
packages**; for the bridge phase it means the Kestrel image with `scx_bpfland`
and the bridge off.

Baselines that need a second image or boot entry:

- stock kernel + Kestrel packages: boot `linux`, then
  `KESTREL_MATRIX_VARIANTS="default:scx_bpfland::off:off" bench/matrix stock-kernel`
- stock packages: build the same sources with Arch's default flags
  (`-O2`, no LTO, no `x86-64-v3`) on a second disk and run the same matrix
  there. `bench/run` records `pkg_*=` versions and footprint metrics; the
  session metrics need the same apps running. If the stock image has no
  `scxctl`, skip `bench/matrix` and run once with an explicit label:
  `KESTREL_VARIANT="name=default sched=scx_bpfland hints=off cgroups=off
  packages=arch kernel=stock" KESTREL_BENCH_MODE=session bench/run stock`.

## sched_ext A/B (no rebuild)

`bench/scx [label]` is the simpler scheduler-only A/B: it switches
`scx_loader` (via `scxctl`), settles, runs `bench/run` under a per-scheduler
label, and restores the configured default (`scx_bpfland`) at the end. Default
pairs are `scx_bpfland:` (loader default) and `scx_lavd:gaming`; override with
`KESTREL_SCX_AB="scx_bpfland: scx_lavd:gaming scx_rusty:"`. Use `bench/matrix`
when the broker or hints are also a factor. Both need root (`sudo`) and a
running `scx_loader.service`; the scripts must be executable.

## Modes

`KESTREL_BENCH_MODE` selects the metric groups, default `all`:

| Mode | Groups | Where it runs |
|---|---|---|
| `cpu` | wakeup latency, timer latency, CPU throughput | VM or bare metal |
| `footprint` | kernel/module bytes, kernel memory, boot time | VM or bare metal |
| `desktop` | compositor + shell CPU and RSS, idle GPU, wakeups | bare metal, live session |
| `session` | per-class CPU, wakeups and RSS under the four-app workload | VM or bare metal, live session |
| `all` | all of the above | desktop/session metrics self-skip without a session |

A mode not requested is not collected; `compare` skips keys absent from either
side, so mixing modes across runs is safe but only shared metrics are shown.

## Session mode (kernel bridge A/B)

`KESTREL_BENCH_MODE=session bench/run <label>` drives a scripted four-app
workload - terminal load in new foot surfaces, Zen tabs, and Hyprland
window/workspace churn - for `KESTREL_BENCH_SESSION_SECONDS` (default 30) and
reports CPU%, wakeups/s, RSS and scheduling wait for three classes:

- `desktop` - Hyprland and Quickshell
- `term` - foot surfaces
- `browser` - Zen and its content processes

Class membership prefers the `kestrel.slice` cgroup hierarchy when it exists
(`/sys/fs/cgroup/kestrel.slice/<class>`), and otherwise falls back to process
name, so it works before the class hierarchy is deployed. Wakeups are a
`voluntary_ctxt_switches` delta. Scheduling wait is the `/proc/<pid>/schedstat`
runqueue-wait delta divided by the timeslice delta, per class: a rootless,
per-task latency that does not need tracefs. `perf sched` is not used because
it needs tracefs and writes large captures; `bench/profile` covers hotspots.

`KESTREL_SESSION_TERMS` (foot surfaces), `KESTREL_SESSION_TABS` (Zen tabs),
`KESTREL_SESSION_BG_CPUS` (stress-ng CPU hogs for contention; `session_bg_bogo_ops`
records their throughput) and `KESTREL_SESSION_NO_APPS=1` (measure churn only,
spawn nothing) tune the workload. It needs a live Hyprland session; without one
it records `session=skipped`. The spawned busy loops self-terminate; spawned
browser tabs are left open and are not cleaned up.

## Scope

`cpu` and `footprint` are meaningful in the VM. `desktop` measures the running
desktop (Hyprland + Quickshell CPU and RSS, idle GPU, wakeups) and needs bare
metal. Frame-time at the renderer level needs a GPU-level tool and is not
measured here; `desktop` reports resource cost instead, which is what the
minimization effort is judged on.

## Profiling (Phase 0)

`bench/profile [seconds]` samples Hyprland, Quickshell, PipeWire and
WirePlumber with `perf` and writes `bench/results/<timestamp>-profile/`:
`meta.txt` (identity and target PIDs), `perf.data` (raw), `perf.txt` (full
symbol report) and `hotspots.txt` (top self-cost symbols). It is the hotspot
capture for the Rust-port work and needs bare metal plus root (or
`kernel.perf_event_paranoid=1`). This is not an A/B metric; `compare` ignores
it. Run it under a representative load (audio playing, menus opening, windows
switching), not on an idle desktop.

## Metrics

| Key | Group | Tool | Better |
|---|---|---|---|
| `schbench_wakeup_p50/p90/p99_us` | cpu | schbench | lower |
| `schbench_rps` | cpu | schbench | higher |
| `cyclictest_max_us`, `cyclictest_avg_us` | cpu | cyclictest | lower |
| `stress_cpu_bogo_ops` | cpu | stress-ng | higher |
| `kernel_image_bytes` | footprint | stat | lower |
| `modules_bytes`, `modules_count`, `loaded_modules` | footprint | du/find/ls | lower |
| `meminfo_slab_kb`, `SReclaimable`, `SUnreclaim`, `KernelStack`, `PageTables`, `VmallocUsed` | footprint | /proc/meminfo | lower |
| `boot_firmware/loader/kernel/userspace/total_s` | footprint | systemd-analyze | lower |
| `desktop_hyprland_cpu_pct`, `desktop_quickshell_cpu_pct` | desktop | /proc | lower |
| `desktop_hyprland_rss_kb`, `desktop_quickshell_rss_kb` | desktop | /proc | lower |
| `desktop_nvidia_mem_mib`, `desktop_nvidia_power_w`, `desktop_nvidia_util_pct` | desktop | nvidia-smi | lower |
| `desktop_powertop_wakeups` | desktop | powertop (root) | lower |
| `session_{desktop,term,browser}_cpu_pct` | session | /proc | lower |
| `session_{desktop,term,browser}_wakeups_per_s` | session | /proc | lower |
| `session_{desktop,term,browser}_rss_kb` | session | /proc | lower |
| `session_{desktop,term,browser}_sched_wait_avg_us` | session | /proc/schedstat | lower |
| `session_{desktop,term,browser}_vram_mib` | session | fdinfo | lower |
| `desktop_{hyprland,quickshell}_vram_mib` | desktop | fdinfo | lower |
| `gpu_vram_used_mib` | all | nvidia-smi / sysfs | lower |
| `session_{desktop,term,browser}_cgroup_pids`, `session_hyprland_hint` | session | cgroupfs, `kestrel-hint` | validity check |
| `session_bg_bogo_ops` | session | stress-ng | higher |
| `session_powertop_wakeups` | session | powertop (root) | lower |

Missing tools are recorded as `missing` and skipped by `compare`.

Per-process VRAM (`session_*_vram_mib`, `desktop_*_vram_mib`) comes from
`/proc/<pid>/fdinfo` and is available on AMD/Intel; the NVIDIA proprietary
driver does not expose it, so those fields read `0` there. Use
`gpu_vram_used_mib` for the NVIDIA total.

## Tools

- `stress-ng`: package `stress-ng`.
- `cyclictest`: package `rt-tests`.
- `schbench`: build from https://github.com/masoncl/schbench (`make`).
- `powertop`: package `powertop`; desktop wakeups need root.
- `perf`: package `perf`; `bench/profile` needs root or `perf_event_paranoid=1`.

`KESTREL_BENCH_SECONDS` overrides the schbench runtime (default 60).
`KESTREL_BENCH_DESKTOP_SECONDS` overrides the desktop sample window (default 10).
`KESTREL_PROFILE_SECONDS` overrides the profile window (default 15).

## Notes

Run each kernel the same way: same power profile, same background load, and
after the machine has settled. The VM measures the scheduler, memory and boot
paths, not the GPU, so treat GPU or game changes as unmeasured there.

Matrix control checklist: one VM at a time (no concurrent build load), same
vCPU/RAM and host, same governor, reset the spawned apps between variants, and
keep the interleaved order. Report medians with min/max from at least five
repeats; do not claim a win from one run. Every result directory records the
kernel, package versions, cmdline, `scxctl get`, scheduler state and variant, so
a report can be reproduced from the artifact alone.

