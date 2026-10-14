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

## Modes

`KESTREL_BENCH_MODE` selects the metric groups, default `all`:

| Mode | Groups | Where it runs |
|---|---|---|
| `cpu` | wakeup latency, timer latency, CPU throughput | VM or bare metal |
| `footprint` | kernel/module bytes, kernel memory, boot time | VM or bare metal |
| `desktop` | compositor + shell CPU and RSS, idle GPU, wakeups | bare metal, live session |
| `all` | all of the above | desktop metrics self-skip without a session |

A mode not requested is not collected; `compare` skips keys absent from either
side, so mixing modes across runs is safe but only shared metrics are shown.

## Scope

`cpu` and `footprint` are meaningful in the VM. `desktop` measures the running
desktop (Hyprland + Quickshell CPU and RSS, idle GPU, wakeups) and needs bare
metal. Frame-time at the renderer level needs a GPU-level tool and is not
measured here; `desktop` reports resource cost instead, which is what the
minimization effort is judged on.

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

Missing tools are recorded as `missing` and skipped by `compare`.

## Tools

- `stress-ng`: package `stress-ng`.
- `cyclictest`: package `rt-tests`.
- `schbench`: build from https://github.com/masoncl/schbench (`make`).
- `powertop`: package `powertop`; desktop wakeups need root.

`KESTREL_BENCH_SECONDS` overrides the schbench runtime (default 60).
`KESTREL_BENCH_DESKTOP_SECONDS` overrides the desktop sample window (default 10).

## Notes

Run each kernel the same way: same power profile, same background load, and
after the machine has settled. The VM measures the scheduler, memory and boot
paths, not the GPU, so treat GPU or game changes as unmeasured there.
