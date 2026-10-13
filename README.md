<p align="center">
  <img src="https://raw.githubusercontent.com/kestrel-distro/.github/main/profile/kestrel-mark.svg" alt="Kestrel" width="120">
</p>

<h1 align="center">Kestrel kernel</h1>

<p align="center">
  An experimental optimized kernel for Kestrel, built from the CachyOS-class recipe for x86-64-v3.
</p>

## What this is

Kestrel runs the stock upstream Arch `linux` kernel by default. This repository
builds an alternative kernel that is installed alongside it for measurement
only, never the default, until it wins on repeatable benchmarks.

## What it builds

- Clang with ThinLTO, x86-64-v3, `-O3`, from the CachyOS kernel recipe.
- Scheduler variants: EEVDF with the CachyOS tweaks, and BORE.
- An AutoFDO and BOLT build in addition to the baseline.
- Selected display patches (HDR, AMD HDMI VRR) and the POC idle-CPU selector
  as a toggle.

## Standards

- Changes ship with tests.
- English in commits, code and documentation.
- History is never rewritten.

## Building

Build with `makepkg` from `packaging/`; see `packaging/README.md`. The ISO
builder consumes this repository as a source package through
`install/kestrel-source.packages` in the `kestrel` repository. The baseline
(`packaging/local`) is `linux-kestrel`; `packaging/bore` and `packaging/autofdo`
cover the other variants.

## Measuring

`bench/README.md`: run `bench/run` once per kernel, then `bench/compare`. CPU
and scheduler only, so it is meaningful in the VM.

## Status

Early. The baseline recipe is adapted; the first build and the VM A/B run are
pending.
