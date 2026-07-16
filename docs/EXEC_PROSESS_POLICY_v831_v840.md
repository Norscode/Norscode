# v831-v840 exec_prosess policy

Dette er eit policy- og designnotat, ikkje ein runtime-implementasjon.

`exec_prosess` er kraftig og skal vere av som standard i produksjon.

## Mål

`builtin.exec_prosess(...)` skal berre brukast når runtime eksplisitt tillèt det.

## Standardpolicy

- Produksjon: av som standard.
- DEV: kan slåast på eksplisitt.
- CI/test: kan slåast på for avgrensa kommandoar.
- Ingen shell i produksjon utan whitelist.
- Ingen arv av sensitive miljøvariablar utan eksplisitt allowlist.

## Foreslått API

```no
la res = builtin.exec_prosess(["/usr/bin/env", "echo", "ok"])
```

Shell-streng som dette skal ikkje vere produksjonsstandard:

```no
la res = builtin.exec_prosess("echo ok")
```

## Runtime-krav

- Returner strukturert resultat med exitkode, stdout og stderr.
- Ha timeout.
- Ha maks output-størrelse.
- Ha allowlist for programsti i produksjon.
- Ha tydeleg feil når funksjonen er deaktivert.
- Logg policy-blokkering utan å logge hemmelege verdiar.

## Status no

`exec_prosess` finst no i aktiv macOS runtime, macOS stage0 og Linux x86_64 stage0, men er framleis DEV-gated.

Gjeldande kontrakt:

- `NORSCODE_ENABLE_EXEC_PROSESS=1` må vere sett.
- Shell-streng fungerer i dagens runtime, men skal framleis ikkje vere produksjonsstandard.
- Produksjonsbruk skal framleis gå mot eksplisitt policy med whitelist, timeout og output-grense.

## Ikkje-gjort i denne v-runden

- Ingen endring i `dist/norscode_native`.
- Ingen overskriving av `bootstrap/stage0`.
- Ingen aktivering av shell-exec i produksjon.
