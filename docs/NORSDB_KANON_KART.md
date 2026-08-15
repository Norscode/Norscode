# NorsDB — kanon-kart

> Status: **kartlegging** (2026-08-11). Dette notatet endrer ingen kode — det
> dokumenterer bare hvilken NorsDB som er den ekte, kjørende motoren, og hva de
> andre artefaktene er, så vi slipper å forvirre design, eksperiment og runtime.

## Kort svar

Det finnes **tre uavhengige «NorsDB»-artefakter** i repoet. De deler *ikke* kode,
og de har ulikt filformat, API og modning.

| # | Sti | Rolle | Status |
|---|-----|-------|--------|
| 1 | `std/norsdb.no` + `selfhost/vm.no` (`_vm_db_*`) | **KANON** — den faktisk kjørende databasen | Aktiv |
| 2 | `NorsDB/` | Designskisse / MVP-spesifikasjon | Ikke koblet til runtime |
| 3 | `packages/norsdb/` | Nedskalert eksperimentbase («parsebar minimalbase») | Uverifisert |

## 1. Kanon: `std/norsdb.no` + `selfhost/vm.no`

Dette er databasen som faktisk brukes fra Norscode-programmer.

- [std/norsdb.no](../std/norsdb.no) er et tynt API-lag som delegerer til
  `_vm_db_*`-funksjonene i [selfhost/vm.no](../selfhost/vm.no) (ca. linje 1911–3200).
- **Backend-identitet:** `backend() = "norsdb-pure-v1"`, `external_runtime = false`,
  sqlite kun som valgfri adapter — ren Norscode, ingen ekstern avhengighet
  (passer selvstendighets-/B2-målet).
- **SQL:** `CREATE TABLE`, `INSERT`, `INSERT OR IGNORE`, `UPDATE`, `DELETE`,
  `DROP TABLE` ([vm.no:3149](../selfhost/vm.no)), pluss `SELECT` med `WHERE`,
  `ORDER BY`, paginering og aggregater (`COUNT` m.fl.).
- **Transaksjoner:** snapshot via `json_stringify`/`json_parse`-kloning
  (`begin`/`commit`/`rollback`). Korrekt, men O(n) på hele databasen per transaksjon.
- **Connection pool:** `pool` / `pool_acquire` / `pool_size` / `pool_close`.
- **Persistens = JSON**, ikke binærformat. En `.db`-fil er hele DB-objektet serialisert:

  ```json
  {"_tables":{"cachetest":{"schema":[{"namn":"verdi","type":"TEXT"}],
   "rows":[{"verdi":"A","id":"1"}],"next_id":2}},"_path":"...","_in_tx":false,...}
  ```

- **Tester:** [tests/test_norsdb_default.no](../tests/test_norsdb_default.no),
  `tests/test_norsdb_default_contract.no`, `tests/test_db*.no`.
  ⚠️ Merk: `./bin/nc run tests/test_norsdb_default.no` tidsavbrøt (>2 min) ved
  kartleggingen — samsvarer med kjent native-runtime-treghet. Ikke verifisert grønt her.

## 2. Designskisse: `NorsDB/`

De store, gjennomdesignede `.nors`-filene (core/schema/crud/index/tx/security).
Beskriver et **mer ambisiøst og annerledes** system enn kanon:

- Binært sidebasert filformat (`NORSDB1`-magic, 4096-byte sider, tabellkatalog, WAL —
  se [NorsDB/norsdb_core.nors](../NorsDB/norsdb_core.nors)).
- **Gap dokumentasjon ↔ kode:** headeren lover binærformat + WAL-replay, men
  persistensen er egentlig en `.state`-tekstfil (`_minne_lagre`/`_minne_last`), og
  `_wal_bruk_oppføring` er en tom stubb (`## Fase 1: replay kommer senere`).
- **Konklusjon:** designskisse med delvis implementasjon. *Ikke* koblet til runtimen.

## 3. Eksperimentbase: `packages/norsdb/`

Kraftig forenklet «gjenoppbygging etter tekstskade» — se
[packages/norsdb/PROGRESS.md](../packages/norsdb/PROGRESS.md).

- [packages/norsdb/norsdb_core.nors](../packages/norsdb/norsdb_core.nors) er en
  minimal variant av #2 med tomme stubber (`snapshot_tøm()` returnerer bare).
- Har `norsklang.toml` og pakkestruktur, men neste-arbeid-listen bekrefter at
  persistens/gjenoppretting ennå ikke er verifisert i praksis.

## Løse tråder (ikke handlet på her)

- **Testhengen** i `test_norsdb_default.no` bør undersøkes (motor vs. native runtime)
  før noe regnes som verifisert.
- **`.db`-filer i repo-rot** (`norscode_pool_test.db`, `norscode_db_integration_*.db`,
  `web_orm_mal.db`) er testartefakter og dukker opp som endret i `git status` —
  kandidater for `.gitignore`.
- **Konsolidering:** #2 og #3 kan senere arkiveres eller merkes som design/eksperiment
  slik at bare kanon ligger likestilt i repo-rota. Ikke gjort nå (bevisst valg: kun kartlegg).
