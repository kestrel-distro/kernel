# Packaging

The Kestrel kernel is built with `makepkg` from `packaging/local/`, which the
`kestrel` repository consumes as a source package through
`install/kestrel-source.packages`.

## Layout

- `packaging/local/PKGBUILD`: adapted from `linux-cachyos`. Builds
  `linux-kestrel` (Clang, ThinLTO, x86-64-v3, EEVDF + CachyOS tweaks, sched_ext,
  DAMON, NTSYNC).
- `packaging/local/config`: the resolved kernel config. It started as the
  CachyOS device config and is folded back after each build (the recipe dumps
  `config-<ver>-<variant>`), so it already carries Kestrel's choices. Kestrel
  overrides still run through `scripts/config` in `prepare()` as a guard (see
  the "Kestrel config enforcement" block); do not hand-edit `config`.

## Build

```
cd packaging/local
makepkg -s -f --noconfirm --nocheck
```

The source is the CachyOS fork tarball, pinned by b2sum. The detached PGP
signature is not used because the build keyring does not carry the CachyOS
keys; the b2sum pin is the integrity check.

Configuration resolution is non-interactive: `prepare()` runs `make olddefconfig`
before `make prepare`, so a symbol missing from `config` takes its Kconfig
default instead of prompting (`make prepare` alone runs `syncconfig`, which is
what used to prompt). Folding the resolved config back keeps those defaults in
the repository, so a fresh build only asks anything if you deliberately set
`_makenconfig=yes` or `_makexconfig=yes`.

## Defaults (overridable as makepkg env)

| Variable | Value | Meaning |
|---|---|---|
| `_kestrel_variant` | `kestrel` | package suffix: `linux-kestrel` |
| `_cpusched` | `cachyos` | EEVDF with CachyOS tweaks (`bore` for the BORE variant) |
| `_processor_opt` | `generic_v3` | x86-64-v3 (AVX2/BMI/FMA, 2013+) |
| `_use_llvm_lto` | `thin` | Clang ThinLTO |
| `_cc_harder` | `yes` | `-O3` |
| `_cachy_config` | `yes` | `CONFIG_CACHY` |
| `_HZ_ticks` | `1000` | timer frequency |
| `_preempt` | `full` | full preemption |
| `_tickrate` | `idle` | no `NO_HZ_FULL` (desktop consistency) |
| `_hugepage` | `always` | THP always |
| `_trim_ksyms` | `no` | `TRIM_UNUSED_KSYMS`; opt-in, breaks out-of-tree modules |
| `_footprint` | `no` | disable legacy/desktop-unused subsystems; A/B + boot-test before shipping |
| `_debug_logging` | `yes` | max kernel verbosity + pstore console/pmsg/EFI; set `no` for the clean A/B kernel |

The Kestrel enforcement block keeps `sched_ext` (the shipped `scx_bpfland`
default), so core `DEBUG_INFO_BTF` stays. Per-module BTF
(`DEBUG_INFO_BTF_MODULES`) is dropped: no shipped tool consumes it and it
bloats every module.

## Variants

- `local`: `linux-kestrel` (baseline). Built by the Kestrel ISO.
- `bore`: `linux-kestrel-bore`, its own `PKGBUILD` with `_cpusched=bore`.
- `autofdo`: documented two-pass build, see `autofdo/README.md`. Not built by
  the ISO.

The `kestrel` ISO builds `local` only; the other variants are built by hand for
A/B measurement.
