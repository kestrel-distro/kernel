# Packaging

The Kestrel kernel is built with `makepkg` from `packaging/local/`, which the
`kestrel` repository consumes as a source package through
`install/kestrel-source.packages`.

## Layout

- `packaging/local/PKGBUILD`: adapted from `linux-cachyos`. Builds
  `linux-kestrel` (Clang, ThinLTO, x86-64-v3, EEVDF + CachyOS tweaks, sched_ext,
  DAMON, NTSYNC).
- `packaging/local/config`: the CachyOS device config, used verbatim. Kestrel
  overrides are applied by `scripts/config` in `prepare()` (see the "Kestrel
  config enforcement" block); do not hand-edit `config`.

## Build

```
cd packaging/local
makepkg -s -f --noconfirm --nocheck
```

The source is the CachyOS fork tarball, pinned by b2sum. The detached PGP
signature is not used because the build keyring does not carry the CachyOS
keys; the b2sum pin is the integrity check.

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

## Variants

- `local`: `linux-kestrel` (baseline). Built by the Kestrel ISO.
- `bore`: `linux-kestrel-bore`, its own `PKGBUILD` with `_cpusched=bore`.
- `autofdo`: documented two-pass build, see `autofdo/README.md`. Not built by
  the ISO.

The `kestrel` ISO builds `local` only; the other variants are built by hand for
A/B measurement.
