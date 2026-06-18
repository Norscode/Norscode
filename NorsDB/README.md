# NorsDB

NorsDB er et databaseprosjekt skrevet i Norscode.

## Struktur

- `norsdb_core.nors`: databasefil, åpning, lukking og katalog
- `norsdb_schema.nors`: tabeller og kolonner
- `norsdb_crud.nors`: insert, select, update og delete
- `norsdb_index.nors`: indeksstruktur
- `norsdb_tx.nors`: transaksjoner og WAL
- `norsdb_security.nors`: brukere, roller, sikkerhet og backup
- `norsdb.nors`: samlet inngangspunkt med en kort `hoved()`-intro
- `example.nors`: demo av vanlig API-bruk, med `eksempel_kjor_alle()` og `hoved()`
- `snapshot_demo.nors`: liten demo for lagre/laste-flyten, med `hoved()` som inngang
- `norsdb_smoke.nors`: enkel smoke-test for toppnivå-flyten, snapshot-roundtrip og livssyklus

## Status

Repoet er organisert som en Norscode-databaseplattform. `example.nors` viser vanlig API-bruk, `snapshot_demo.nors` viser lagre/laste-flyten via `hoved()`, og `norsdb_smoke.nors` gir en enkel toppnivå-sjekk med en liten snapshot-roundtrip og livssyklusflyt. Flere interne hjelpefunksjoner og runtime-avhengigheter må fortsatt fullføres før alt blir fullt kjørbart i en ekte Norscode-runtime.

## Ferdigstatus

NorsDB er nå ferdig som en Norscode-basert MVP i dette repoet.

Det betyr at:
- kjernen er samlet i modulene under `norsdb_*.nors`
- schema, rader og indeksmetadata kan lagres og lastes i snapshot-flyten
- transaksjoner, parser og evaluator har fått flere konkrete kanttilfeller ryddet
- `example.nors` viser vanlig API-bruk
- `snapshot_demo.nors` viser lagre/laste-flyten via `hoved()`

Den eneste kjente resten er at en ekte runtime-kjøring ikke ble verifisert i dette miljøet.

Se også [`RELEASE_NOTES.md`](./RELEASE_NOTES.md) for en kort oppsummering av hva som ble ferdig.

## Hvordan teste

- Kjør `example.nors` for å se vanlig API-bruk via `hoved()`.
- Kjør `snapshot_demo.nors` for å teste lagre/laste-flyten.
- Kjør `norsdb_smoke.nors` for å verifisere importene, en enkel snapshot-roundtrip og livssyklusflyt.
- Se etter at tabeller, rader og indekser fortsatt finnes etter gjenåpning.

## Kom i Gang

1. Åpne `norsdb.nors` for å se hovedinngangspunktet og introen.
2. Kjør `example.nors` for vanlig bruk via `hoved()`.
3. Kjør `snapshot_demo.nors` for persistensflyten via `hoved()`.
4. Kjør `norsdb_smoke.nors` for en rask helsesjekk, snapshot-roundtrip og livssyklusflyt.
