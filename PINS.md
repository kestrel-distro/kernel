# Pinned inputs

| Input | Pin |
|---|---|
| Kestrel kernel fork | `kestrel-distro/linux-cachyos` (fork of `CachyOS/linux-cachyos`) |
| Upstream (fetch-only) | `CachyOS/linux-cachyos`; adapted by hand, never pushed to |
| linux-cachyos PKGBUILD | `_major=7.2`, `_minor=10`, `_tagrel=1` |
| CachyOS kernel source | `cachyos-7.2-10-1` tarball (github.com/CachyOS/linux releases) |
| kernel-patches | `github.com/CachyOS/kernel-patches` `master/7.2` |
| Target | x86-64-v3, Clang + ThinLTO |

## Adapting upstream

Fetch the canonical recipe and bring changes in by hand onto the fork:

```
git fetch upstream              # CachyOS/linux-cachyos
git log --oneline upstream/master ^HEAD
# cherry-pick the recipe/config changes that apply, then push to `fork`
```

The build itself still follows the CachyOS tarball plus `cachyos/kernel-patches`
pins above; the fork is where Kestrel's adapted recipe and patches live so they
can be reviewed as a diff against `CachyOS/linux-cachyos`.
