# AutoFDO variant

AutoFDO is a two-pass build: first a profiling kernel, then a real build that
consumes the profile. It cannot be built unattended (it needs a profile
gathered from a running system), so this variant is documented rather than
committed as a ready PKGBUILD.

Use `packaging/local/PKGBUILD` with these overrides, from a checkout of this
repository.

## 1. Profiling kernel

```
cd packaging/local
_kestrel_variant=kestrel-autofdo _autofdo=yes _build_debug=yes \
  makepkg -s -f --noconfirm --nocheck
```

Install the resulting `linux-kestrel-autofdo` and its `-dbg` package (the dbg
package carries the unstripped `vmlinux` the profiler needs), then boot it.

## 2. Collect a profile

```
sudo sh -c "echo 0 > /proc/sys/kernel/kptr_restrict"
sudo sh -c "echo 0 > /proc/sys/kernel/perf_event_paranoid"
sudo perf record --pfm-events RETIRED_TAKEN_BRANCH_INSTRUCTIONS:k \
  -a -N -b -c 500009 -o kernel.data -- time makepkg -sfc --skipinteg
```

Use `BR_INST_RETIRED.NEAR_TAKEN` on Intel (Haswell and later). Then convert:

```
create_llvm_prof --binary=/usr/lib/modules/<kernver>/build/vmlinux \
  --profile=kernel.data --format=extbinary --out=packaging/local/kernel.afdo
```

## 3. Final build

```
cd packaging/local
_kestrel_variant=kestrel-autofdo _autofdo=yes \
  _autofdo_profile_name=kernel.afdo \
  makepkg -s -f --noconfirm --nocheck
```

Place `kernel.afdo` in `packaging/local/` before running, and add a 4th
`b2sums` entry (`SKIP` is acceptable) so the checksum count matches the extra
source.

BOLT runs after this, on the built `vmlinux`, per the LLVM `OptimizingLinux`
guide; it is not wired into the PKGBUILD.
