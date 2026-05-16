# Fase 7 — Binær og distribusjon

Mål: gjøre Norscode praktisk å installere og kjøre uten at brukeren må sette opp et Python-prosjekt manuelt.

## Leveranser

- `nc` som Linux-binær
- `nc` som macOS-binær
- release-pakke med `bin/`, `std/`, `selfhost/`, `examples/` og manifest
- install-script som peker `current` til aktiv release
- `nc --version`
- `nc doctor`
- smoke-test for installert release

## Release-layout

```text
norscode-<versjon>/
  bin/
    nc
    norcode
  std/
  selfhost/
  examples/
  docs/
  manifest.json
  SHA256SUMS
```

## Runtime-kontrakt

`nc` skal først lete etter runtime-filer relativt til egen binær:

1. `<release>/std`
2. `<release>/selfhost`
3. `<release>/examples`

Hvis de ikke finnes, kan utviklerflyt bruke repo-root som fallback.

## Kommandoer

```bash
nc --version
nc doctor
nc run examples/hello.no
nc selfhost-check
```

## `nc doctor`

Doctor bør sjekke:

- binær finnes og kan kjøres
- stdlib finnes
- selfhost-kjerne finnes
- write access til cache/temp
- versjon i manifest matcher `nc --version`
- enkel smoke-test kan parse/semantic/IR-kompilere et lite program

## Smoke-test

Minimumsprogram:

```norscode
funksjon main() -> heltall {
    la x: heltall = 1 + 2
    returner x
}
```

Forventet:

- parser OK
- semantic OK
- IR snapshot genereres
- ingen Python-prosjektsetup nødvendig for bruker

## Exit-kriterier

Fase 7 er ferdig når en bruker kan laste ned release, kjøre install-script og deretter kjøre:

```bash
nc --version
nc doctor
nc run hello.no
```

uten å vite noe om repo-struktur eller Python-venv.
