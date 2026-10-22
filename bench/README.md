# Kernel benchmark harness

A/B harness for the Kestrel kernel. Run it once per kernel, then compare.

```
bench/run stock          # booted into the stock `linux` kernel
# reboot into linux-kestrel
bench/run kestrel
bench/compare bench/results/<stock-dir> bench/results/<kestrel-dir>
```

Each run writes `bench/results/<timestamp>-<label>/`: `summary.txt` (machine
readable), `meta`-level identity, and the raw tool output. `bench/compare`
prints the deltas.

## sched_ext A/B (no rebuild)

`bench/scx [label]` drives the shipped `scx-scheds` schedulers instead of the
kernel: it switches `scx_loader` (via `scxctl`), settles, runs `bench/run`
under a per-scheduler label, and restores the configured default (`scx_bpfland`)
at the end. Default pairs are `scx_bpfland:` (loader default) and
`scx_lavd:gaming`; override with `KESTREL_SCX_AB="scx_bpfland: scx_lavd:gaming
scx_rusty:"`. `KESTREL_BENCH_MODE` and `KESTREL_BENCH_SECONDS` pass through, and
`KESTREL_SCX_SETTLE` (default 5s) sets the settle window after each switch. It
prints the `bench/compare` commands for the result directories it wrote.

This exists so the scheduler choice is measured before any kernel-side
scheduler work (a bespoke Rust sched_ext scheduler) is justified. It needs root
(`sudo`) and a running `scx_loader.service`; the `bench/scx` file must be
executable (`chmod +x bench/scx`) like `bench/run`.

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
workload - terminal load in new Ghostty surfaces, Zen tabs, and Hyprland
window/workspace churn - for `KESTREL_BENCH_SESSION_SECONDS` (default 30) and
reports CPU%, wakeups/s and RSS for three classes:

- `desktop` - Hyprland and Quickshell
- `term` - Ghostty surfaces
- `browser` - Zen and its content processes

Class membership prefers the `kestrel.slice` cgroup hierarchy when it exists
(`/sys/fs/cgroup/kestrel.slice/<class>`), and otherwise falls back to process
name, so it works before the class hierarchy is deployed. Wakeups are a
`voluntary_ctxt_switches` delta, a portable proxy rather than a perf counter.

`KESTREL_SESSION_TERMS` (Ghostty surfaces), `KESTREL_SESSION_TABS` (Zen tabs)
and `KESTREL_SESSION_NO_APPS=1` (measure churn only, spawn nothing) tune the
workload. It needs a live Hyprland session; without one it records
`session=skipped`. The spawned busy loops self-terminate; spawned browser tabs
are left open and are not cleaned up.

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
| `session_powertop_wakeups` | session | powertop (root) | lower |

Missing tools are recorded as `missing` and skipped by `compare`.

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
