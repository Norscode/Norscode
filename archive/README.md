# Arkiv

Historiske spor (C-backend, legacy shell, `.bak`-kopiar av selfhost-kjelder og den gamle
C-minimal-VM-en) ligg ikkje lenger i treet. Git-historikken tek vare på alt:

```
git log --oneline -- archive/
git show c555afd:archive/legacy_source/selfhost/vm.no.bak_v802_args_any
git show 40922f9^:archive/legacy_c_backend/nc_native_main.c
```

Normal bruk går via `dist/norscode_native`, `./bin/nc` og `selfhost/*.no`.
