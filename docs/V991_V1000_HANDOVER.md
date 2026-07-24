# v991-v1000 handover

v1000 er ei trygg overlevering, ikkje ein produksjonsrelease.

## Klart

- Web/admin demo er samla.
- Statussider og JSON viser blokkeringar.
- AI-demo er tekstbasert og skriv ikkje automatisk.
- CMS-demo er DEV-only.

## Ikkje klart

- `random_hex` er ikkje ferdig i native/stage0.
- `exec_prosess` er ikkje produksjonssikra.
- Tid-funksjonar er ikkje endeleg runtime-kontrakt.

## Neste fase

1. Bygg ny stage0-løype.
2. Implementer sikker random i native.
3. Lag native testpakke for random/tid/exec-policy.
4. Fjern DEV-only login berre etter at random-gate er grønn.

