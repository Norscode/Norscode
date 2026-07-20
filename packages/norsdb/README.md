# NorsDB

NorsDB er nå pakket inn som en egen Norscode-pakke under `packages/norsdb`.

## Struktur

- `norsklang.toml`: pakke- og entrypoint-metadata
- `norsdb_core.nors`: databasefil, åpning, lukking og katalog
- `norsdb_schema.nors`: tabeller og kolonner
- `norsdb_crud.nors`: insert, select, update og delete
- `norsdb_index.nors`: indeksstruktur
- `norsdb_tx.nors`: transaksjoner og WAL
- `norsdb_security.nors`: brukere, roller, sikkerhet og backup
- `norsdb.nors`: samlet inngangspunkt med `hoved()`, `selftest()`, `samandrag()` og `status()`
- `example.nors`: demo av vanlig API-bruk, med `eksempel_kjor_alle()` og `hoved()`
- `snapshot_demo.nors`: liten demo for lagre/laste-flyten, med `hoved()` som inngang
- `norsdb_smoke.nors`: enkel smoke-test for toppnivå-flyten, info, DB-info, connection-status, snapshot-roundtrip, livssyklus, status og samandrag
- `norsdb.no`: midlertidig runtime-entrypoint brukt for å verifisere entrypoint og moduloppløsing
- Roten av repoet har også noen midlertidige `norsdb_*.no`-broer som kun finnes for runtime-oppløsing under eksperimentering

## Status

Pakken er organisert som en Norscode-databaseplattform. `example.nors` viser vanlig API-bruk, `snapshot_demo.nors` viser lagre/laste-flyten via `hoved()`, og `norsdb_smoke.nors` gir en enkel toppnivå-sjekk med info, DB-info, connection-status, snapshot-roundtrip og livssyklusflyt. `norsdb.nors` er den samlende introen for pakken og eksponerer også `samandrag()` og `status()`. `norsdb.no` er foreløpig bare en midlertidig runtime-entrypoint.

## Ferdigstatus

NorsDB er nå samlet som en stabil Norscode-basert pakke med en bevisst enkel base i flere moduler.

Det betyr at:
- kjernen er samlet i modulene under `norsdb_*.nors`
- schema, CRUD, indeks, transaksjoner og sikkerhet har en parsebar og kjørbar grunnbase
- `example.nors` viser vanlig API-bruk
- `snapshot_demo.nors` viser lagre/laste-flyten via `hoved()`
- den ekte smoke-testen lever fortsatt i `norsdb_smoke.nors`

Se også [`RELEASE_NOTES.md`](./RELEASE_NOTES.md) for en kort oppsummering av hva som ble ferdig.

## Hvordan teste

- Kjør `example.nors` for å se vanlig API-bruk via `hoved()`.
- Kjør `snapshot_demo.nors` for å teste lagre/laste-flyten.
- Kjør `norsdb_smoke.nors` for å verifisere importene, en enkel info-del, DB-info, connection-status, snapshot-roundtrip og livssyklusflyt.
- Kjør `norsdb_selftest.no` for en liten, selvstendig selftest-entry.
- Kjør `norsdb.no` for en midlertidig runtime-entrypoint.
- Se etter at tabeller, rader og indekser fortsatt finnes etter gjenåpning.

## Pakke-selftest

- Kjør `norsdb_selftest.no` for en liten, selvstendig pakke-selftest.

## Kom i Gang

1. Åpne `norsdb.nors` for å se hovedinngangspunktet og introen.
2. Kjør `example.nors` for vanlig bruk via `hoved()`.
3. Kjør `snapshot_demo.nors` for persistensflyten via `hoved()`.
4. Kjør `norsdb_smoke.nors` for den ekte smoke-testen.
