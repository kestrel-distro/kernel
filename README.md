<p align="center">
  <img src="https://raw.githubusercontent.com/kestrel-distro/.github/main/profile/kestrel-mark.svg" alt="Kestrel" width="120">
</p>

<h1 align="center">Kestrel kernel</h1>

<p align="center">
  <code>linux-kestrel</code>: the kernel the distribution boots, with the
  per-task scheduling hint bridge.
</p>

## What this is

`linux-kestrel` is the default kernel of the Kestrel distribution. It is built
from the CachyOS-class recipe with Clang ThinLTO for `x86-64-v3`, and it carries
the Kestrel scheduling hint extension. The live ISO boots it; the installed
system receives it from `[kestrel]` as `linux-kestrel` /
`linux-kestrel-headers`.

The hint extension is the point of this repository:

- `struct sched_attr` gains `sched_ext_hint` (`u32`).
- Userspace sets it with `sched_setattr(2)` and `SCHED_FLAG_KESTREL_HINT`
  (`1ULL << 40`); valid values are `0..1024`, zero meaning "no hint".
- `struct task_struct` carries the value in the sched_ext state;
  `scx_bpf_task_hint(struct task_struct *)` reads it from a BPF scheduler.
- The hint-only path keeps the task's policy and parameters
  (`SCHED_FLAG_KEEP_POLICY | SCHED_FLAG_KEEP_PARAMS`) and re-enqueues at the
  head of the DSQ.

`scx_kestrel` (in `kestrel-distro/scx`) consumes the hint; `libkestrel-hint`
and `kestrel-hint` (in `kestrel-distro/kestrel`) set and read it. None of this
changes the ABI for tasks that never use the flag.

## What it builds

- Clang with ThinLTO, `x86-64-v3`, `-O3`, from the CachyOS recipe.
- Full preemption, 1000 Hz tick, sched_ext, DAMON reclaim and NTSYNC.
- Scheduler variants: EEVDF with the CachyOS tweaks, BORE, and the AutoFDO/BOLT
  experiments.
- Selected display patches (HDR, AMD HDMI VRR) and the idle-CPU selector as a
  toggle.
- The Kestrel hint patch (`packaging/local/kestrel/0001-kestrel-sched-hint.patch`)
  and its apply check.

## Building

```bash
cd packaging/local
makepkg -s -f --noconfirm --nocheck
```

`packaging/bore` and `packaging/autofdo` cover the other variants. The ISO
builder consumes this repository through
`install/kestrel-source.packages` in `kestrel-distro/kestrel`, so a local ISO
build uses the sibling checkout when `--local-source` is passed.

CI (`kestrel-patch`) applies the hint patch to the pinned upstream tarball and
checks that it still applies cleanly.

## Measuring

`bench/README.md` documents the harness:

```bash
bench/run <label>                 # one kernel/one run, CPU + scheduler workload
bench/compare results/A results/B
bench/foot-workers 8              # worker-count matrix, server PSS per setting
```

`bench/run` is CPU and scheduler work only, so it is meaningful in the VM.
`foot-workers` starts a private `foot --server`, runs the requested number of
clients, and reports wall/max/avg latency, server CPU ticks and peak server PSS
for each `workers=` value.

`kestrel-bench` in the payload runs the same harness against `scx_bpfland` and
`scx_kestrel`, and adds peak PSS sampling for the desktop apps.

## Standards

- Changes ship with tests (patch apply, bench, or both).
- English in commits, code and documentation.
- History is never rewritten.
- Nothing is promoted on a single run.

## Status

`linux-kestrel` builds, boots and installs. The hint ABI is implemented and
exercised: `kestrel-hint supported` reports yes on the live image, set/get
round-trips, `scx_kestrel` loads and consumes the hints, and `kestrel-bench`
completes the `scx_bpfland` vs `scx_kestrel` A/B. `scx_bpfland` remains the
session default until repeatable benchmark results justify changing it.

## Related

[`kestrel`](https://github.com/kestrel-distro/kestrel) ·
[`iso`](https://github.com/kestrel-distro/iso) ·
[`scx`](https://github.com/kestrel-distro/scx) ·
[`scx-loader`](https://github.com/kestrel-distro/scx-loader) ·
[`docs`](https://github.com/kestrel-distro/docs)
