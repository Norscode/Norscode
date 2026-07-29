# v961-v970 backup policy

Dette er ei tryggleiksrunde for release-arbeid.

## Låste område

- Ikkje overskriv `dist/norscode_native` automatisk.
- Ikkje overskriv `bootstrap/stage0` automatisk.
- Ikkje endre native artefakt utan backup og eksplisitt godkjenning.

## Før native-endring

1. Dokumenter mål.
2. Ta backup av målfil.
3. Sjekk arkitektur og plattform.
4. Køyr separat probe/test.
5. Ha rollback-kommando klar.

