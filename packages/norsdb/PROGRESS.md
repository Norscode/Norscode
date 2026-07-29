# NorsDB - Progress

## Ferdig eller delvis lukket

- [x] `norsdb_smoke.nors` gjør en reell kjerne-/livssyklus-sjekk i stedet for bare faste `sann`-verdier
- [x] `norsdb.nors` sin `status()` bruker faktisk `smoke_test()`
- [x] Indeksmetadata og indeksdata skrives nå til `.idx` og leses inn igjen ved åpning
- [x] Brukere og roller kan lagres og leses via sidecar JSON-filer
- [x] Pakke-selftesten kjører nå den reelle smoke-testen
- [x] En egen NorsDB-integrasjonstest er lagt til for åpne/skriv/lukk/gjenåpne-flyten
- [x] En kjørbar smoke-runner i `.no`-format er lagt til for runtime-verifisering
- [x] `norsdb_smoke.no` finnes som midlertidig runtime-entrypoint
- [x] `norsdb_smoke.no` er nå en ren, stabil wrapper
- [x] En `norsdb_smoke.no`-wrapper er lagt til for å matche runtime-forventet filnavn
- [x] `norsdb.no` er koblet til pakke-selftesten som ekte entrypoint
- [x] Midlertidige runtime-broer er merket tydelig i dokumentasjonen
- [x] Ubrukte midlertidige runtime-filer er ryddet bort
- [x] Repo-rotens midlertidige `norsdb_*.no`-broer er dokumentert som eksperimentelle
- [x] Midlertidige runtime-broer er merket direkte i filene
- [x] Pakke-lokale `norsdb_*.no`-broer er fjernet for å unngå duplikater
- [x] Pakke-entrypoint `norsdb.no` er gjenopprettet som midlertidig bro
- [x] Dokumentasjonen sier nå klart at `norsdb.no` er midlertidig
- [x] `norsdb.nors.hoved()` bruker nå pakken sin egen selftest
- [x] `norsdb_smoke.nors.smoke_status()` speiler nå faktisk smoke-testresultat
- [x] `norsdb_core.nors` er gjenoppbygd til en parsebar kjerne igjen
- [x] `norsdb_crud.nors` er gjenoppbygd til en parsebar, minimal CRUD-base
- [x] `norsdb_schema.nors` er gjenoppbygd til en parsebar, minimal schema-base
- [x] `norsdb_schema.nors` lagrer og henter nå en enkel tabell-/snapshotbase
- [x] `norsdb_schema.nors` kan nå legge til og fjerne kolonner i metadata
- [x] `norsdb_schema.nors` har enkel statusrapportering igjen
- [x] `norsdb_schema.nors` har en enkel table_exists-hjelper igjen
- [x] `norsdb_crud.nors` har enkel, reell radlagring og telling igjen
- [x] `norsdb_crud.nors` har enkel filtrering, oppdatering og sletting igjen
- [x] `norsdb_crud.nors` har filtrert count/exists igjen
- [x] `norsdb_crud.nors` har enkel statusrapportering igjen
- [x] `norsdb_crud.nors` har enkel sortering i select igjen
- [x] `norsdb_crud.nors` har enkel paginering i select igjen
- [x] `norsdb_crud.nors` har enkel tabelloversikt igjen
- [x] `norsdb.nors` er gjenoppbygd til en stabil, importfri pakkebase
- [x] `norsdb_index.nors` er gjenoppbygd til en parsebar, minimal indeks-base
- [x] `norsdb_index.nors` har enkel statusrapportering igjen
- [x] `norsdb_index.nors` har enkel oppdatering og sletting av indeksdata igjen
- [x] `norsdb_tx.nors` er gjenoppbygd til en parsebar, minimal transaksjons-base
- [x] `norsdb_tx.nors` logger nå enkle transaksjonsoperasjoner og status
- [x] `norsdb_tx.nors` har enkel statusrapportering for aktiv transaksjon
- [x] `norsdb_tx.nors` har en enkel tx_status-alias
- [x] `norsdb_tx.nors` har enkel loggstatus for transaksjoner
- [x] `norsdb_tx.nors` har enkel transaksjonsliste igjen
- [x] `norsdb_security.nors` er gjenoppbygd til en parsebar, minimal sikkerhets-base
- [x] `norsdb_security.nors` har enkel rolleoppretting og status igjen
- [x] `norsdb_security.nors` har enkel bruker- og rolleliste igjen
- [x] `norsdb_security.nors` kan nå aktivere og deaktivere brukere
- [x] `norsdb_security.nors` har enkel sikkerhetsstatus igjen
- [x] `norsdb_security.nors` har enkel backup-status igjen
- [x] `norsdb_security.nors` har enkel brukerstatus igjen

## Neste arbeid

- [ ] Verifisere at åpning/lukking, snapshot og gjenoppretting fungerer i praksis
- [ ] Bygge modulene dypere igjen, hvis vi vil løfte NorsDB utover den nåværende minimalbasen
