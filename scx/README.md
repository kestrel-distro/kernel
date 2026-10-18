# Kestrel sched_ext scheduler (draft)

Status: draft, not implemented, not measured. This is the design for the
Phase 1b scheduler from Part C of the kernel plan. It is **gated** on the
no-rebuild A/B in `bench/scx`: if `scx_bpfland` or `scx_lavd` already win on the
compositor and shell metrics, this scheduler does not get written.

## Goal

Give the Hyprland desktop a wakeup-latency-first scheduler: compositor, shell,
render and audio threads get the CPU back sooner than ordinary work, without a
throughput collapse under load. Judged on presented frame time and dropped
frames (bare metal), main-loop stalls, PipeWire xruns and shell CPU/RSS, per
`bench/README.md`. Not on synthetic CPU throughput.

## Why a custom one, and when not

sched_ext already ships schedulers that cover most of this (`scx_bpfland` is
vruntime plus interactive priority; `scx_lavd` has a latency/gaming mode). A
bespoke scheduler is only justified if the A/B shows both fall short on the
desktop metrics. The cheap moves first: pick the better shipped scheduler and
its mode (`kestrel scx`), and only then fork or write one.

## Approach

A small vtime scheduler in Rust, built on the `scx` Rust crate (the same shape
as `scx_rusty`/`scx_rustland`), not a from-scratch policy. The pieces:

1. **Task classification.** Put compositor (`Hyprland`), shell (`quickshell`),
   the GPU/render threads and the audio threads (`pipewire`, `wireplumber`)
   into a latency class. Classify by cgroup and/or main-thread process name at
   fork/exec; a runtime `scxctl`-visible knob overrides the list. This is the
   same criterion the plan uses for the userspace Rust ports.
2. **Latency class policy.** Shorter slice, an early vtime boost so a woken
   latency task preempts ordinary work, and a bias to an idle or L2-local CPU.
   Keep the ordinary class plain vtime. Bound the boost so a busy latency task
   cannot starve the housekeeping class (anti-starvation is the failure mode
   that makes gaming scheds feel worse under load).
3. **Keep it small.** One class split, not a taxonomy. Every rule has to earn
   its place on the metrics, since each is a place the scheduler can misfire.

Fork base: start from `scx_rusty` (multi-domain, vtime, Rust) if the class split
is enough, or `scx_rustland` if userspace-decision flexibility matters more than
syscall overhead. Decision deferred to the draft's first prototype measurement.

## Build and packaging

Built with cargo + libbpf + the BPF toolchain, the same way `scx-scheds` builds
its schedulers. Shipped as its own package in the `kestrel` overlay
(`kestrel-distro/packages`), `depends` on `scx-scheds`/`scx-tools` for the
loader. Not part of the kernel package: sched_ext schedulers are userspace BPF
and load through `scx_loader`, so this needs no kernel rebuild.

## Integration

`etc/scx_loader/config.toml` in `kestrel-distro/kestrel` already selects the
default; switch the default to the Kestrel scheduler only after it wins, and
keep it switchable at runtime through `kestrel scx`.

## Measurement

Extend the `bench/scx` pair list with the Kestrel scheduler and run it in the
same window as `scx_bpfland` and `scx_lavd:gaming`. Same machine, same load,
same settle. A win means a repeatable improvement on the desktop metrics, not a
single run.

## Risks and open questions

- Wakeup path overhead: a userspace scheduler pays for every decision; a simple
  in-BPF policy may beat a flexible one here.
- Correctness under load: starvation and priority inversion are the likely ways
  a latency-first policy regresses, so the anti-starvation bound needs stress
  testing, not just idle-desktop runs.
- Verify how the running scheduler is identified in `scxctl get` output so the
  A/B label and the settle check stay correct for a non-`scx_*` name.
- Confirm the `scx` Rust crate API against the `scx-scheds` version Arch ships
  before writing code.
