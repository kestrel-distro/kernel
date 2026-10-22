# Kestrel kernel patch: per-task scheduler hint

One patch, `0001-kestrel-sched-hint.patch`, carried by the kernel recipe. It is
the kernel half of the bridge (section 4 of the plan); the userspace half is
`libkestrel-hint` in `kestrel-distro/kestrel` and the consumer is `scx_kestrel`.

## What it adds

- `struct sched_attr::sched_ext_hint` (`include/uapi/linux/sched/types.h`): a
  `__u32` appended to the size-versioned scheduler attribute. Old callers that
  pass the previous `size` are unaffected; the field reads as zero.
- `SCHED_FLAG_KESTREL_HINT` and `SCHED_KESTREL_HINT_MAX` in
  `include/uapi/linux/sched.h`, and the flag is added to `SCHED_FLAG_ALL` so
  `__sched_setscheduler()` accepts it.
- `struct sched_ext_entity::hint` (`include/linux/sched/ext.h`): where the
  value is stored, guarded by `CONFIG_SCHED_CLASS_EXT`.
- `kernel/sched/syscalls.c`: range-checks the hint, stores it with
  `WRITE_ONCE()`, and re-enqueues the task at the head when a queued task's
  hint changes.
- `kernel/sched/ext/ext.c`: `scx_bpf_task_hint(p)`, a read-only kfunc so a
  sched_ext scheduler can read the hint, registered in `scx_kfunc_ids_any`.

Setting a hint is one `sched_setattr(2)` with
`SCHED_FLAG_KESTREL_HINT | SCHED_FLAG_KEEP_POLICY | SCHED_FLAG_KEEP_PARAMS`, so
it never disturbs the task's policy, priority or nice value.

## How it is applied

`packaging/local/PKGBUILD`'s `prepare()` applies it from this directory with an
explicit `git apply --check -p1` and no fuzz, then `git apply -p1`. A rebase
that moves the scheduler code fails the build loudly instead of patching the
wrong place. The patch is anchored to the pinned CachyOS tarball
(`PINS.md`), not to upstream main.

## Rebase procedure

1. Bump the CachyOS pin and update `PINS.md`.
2. Extract the new tarball and apply the patch with
   `git apply --check -p1` against it.
3. Resolve context drift by hand (the touched functions are stable: struct
   `sched_attr`, `SCHED_FLAG_ALL`, `struct sched_ext_entity`,
   `__sched_setscheduler()`, the sched_ext kfunc set).
4. If the sched_ext kfunc API moved, update `scx_kestrel` in the same cycle
   (the sched_ext ABI is explicitly unstable across kernels).

## Files

- `0001-kestrel-sched-hint.patch` - the change.
- `selftest-kestrel-hint.c` - the userspace selftest (sets a hint on itself and
  reads it back through `sched_setattr`/`sched_getattr`). Build it against the
  patched kernel headers; it is the `tools/testing/selftests` entry for the
  patch.
- `kestrel_hint.2` - draft man page for the extension.

Nothing here is built by the ISO or by `bench/`; the kernel package build in CI
is the only place it runs.
