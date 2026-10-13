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

## Scope

CPU and scheduler only, so it is meaningful in the VM. GPU and frame-time need
bare metal and are out of scope.

## Metrics

| Key | Tool | Better |
|---|---|---|
| `schbench_wakeup_p50/p90/p99_us` | schbench | lower |
| `schbench_rps` | schbench | higher |
| `cyclictest_max_us`, `cyclictest_avg_us` | cyclictest | lower |
| `stress_cpu_bogo_ops` | stress-ng | higher |

Missing tools are recorded as `missing` and skipped by `compare`.

## Tools

- `stress-ng`: package `stress-ng`.
- `cyclictest`: package `rt-tests`.
- `schbench`: build from https://github.com/masoncl/schbench (`make`).

`KESTREL_BENCH_SECONDS` overrides the schbench runtime (default 60).

## Notes

Run each kernel the same way: same power profile, same background load, and
after the machine has settled. The VM measures the scheduler and memory paths,
not the GPU, so treat GPU or game changes as unmeasured here.
