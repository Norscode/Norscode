# NorsDB — kanon-kart

> Status: **cutover til binær motor gjort — steg 1 av 2** (2026-08-21). `std.db`
> rutar no til den binære B-tre/WAL-motoren; den gamle JSON-modulen `std/norsdb.no`
> er **fjerna**. `_vm_db_*` i `selfhost/vm.no` er no daud kode, planlagt fjerna i
> steg 2. (Tidlegare kartlegging: 2026-08-11.)

## Kort svar

Kanon er no **den binære motoren** (`std/norsdb_db` → `norsdb_sql` + `norsdb_motor`).
Utover den finst framleis to urelaterte design-/eksperiment-artefakter.

| # | Sti | Rolle | Status |
|---|-----|-------|--------|
| 1 | `std/norsdb_db.no` + `std/norsdb_sql.no` + `std/norsdb_motor.no` | **KANON** — den faktisk kjørende databasen (binær B-tre/WAL) | Aktiv |
| — | `std/norsdb.no` + `selfhost/vm.no` (`_vm_db_*`) | Gamal JSON-motor | **Fjerna (modul); vm-kode daud, fjernast i steg 2** |
| 2 | `NorsDB/` | Designskisse / MVP-spesifikasjon | Ikke koblet til runtime |
| 3 | `packages/norsdb/` | Nedskalert eksperimentbase («parsebar minimalbase») | Uverifisert |

## 1. Kanon: `std/norsdb_db.no` + `std/norsdb_sql.no` + `std/norsdb_motor.no`

Dette er databasen som faktisk brukes fra Norscode-programmer via `std.db`.

- [std/db.no](../std/db.no) rutar til [std/norsdb_db.no](../std/norsdb_db.no)
  (adapter), som byggjer på [std/norsdb_sql.no](../std/norsdb_sql.no) (SQL-lag:
  eigen tokenizer + uttrykks-evaluator) og [std/norsdb_motor.no](../std/norsdb_motor.no)
  (lagring). **Ingen `vm.no`, ingen JSON.**
- **Backend-identitet:** `backend() = "norsdb-pure-v1"` (kontrakt-kompat med gamal),
  `status()["storage"] = "binær-btre-wal"`, `external_runtime = false` — ren Norscode.
- **SQL:** SQLite-nivå SQL-flate (Fase 7 komplett): CRUD, full WHERE (AND/OR/parentesar/
  NOT/LIKE/IN/BETWEEN/IS NULL), uttrykk + funksjonar, `ORDER BY`/`LIMIT`/`OFFSET`,
  `GROUP BY`/`HAVING` + aggregat, subqueries, `UNION`/`INTERSECT`/`EXCEPT`, CTE (+RECURSIVE),
  vindusfunksjonar, `JOIN` (INNER/LEFT/RIGHT/FULL/CROSS), constraints (PK/UNIQUE/NOT NULL/
  CHECK/FK/UPSERT), `ALTER`/`VIEW`/`TRIGGER`, prepared statements, `CREATE INDEX`.
- **Handle:** eit ugjennomsiktig objekt (`ordbok`), ikkje ein streng — per-handle-state
  ligg i objektet (unngår modul-globalt register, som kodegen ikkje handterer påliteleg).
- **Transaksjoner:** snapshot via `db_serialiser`/`db_replay` (`begin`/`commit`/`rollback`).
- **Connection pool:** `pool` / `pool_acquire` / `pool_size` / `pool_close`.
- **Persistens = binærformat** (B-tre/WAL + checkpoint), ikkje JSON-blob. `open()` på ei
  gamal JSON-`.db`-fil auto-migrerer til binær (`migrer_json`).
- **Åtferdsavvik frå gamal JSON-motor** (SQLite-troskap, golden vs `sqlite3 3.51.0`):
  `migrate` dedupliserer **ikkje**; `query_rader`/`query_text` gjev **REAL** AVG (t.d. 40.5),
  medan `query_int(AVG)` framleis trunkerer til heiltal.
- **Tester:** binær-motor-suita (`tests/test_norsdb_motor_*`, `_sql*`, `_cte`, `_vindu`,
  `_join_komplett`, `_constraints`, `_typer`, `_setops`, `_skjema`, `_plan`, …), pluss
  stack-mot-binær (`tests/test_db*.no`, `test_norsdb_default*.no`, auth/admin/domenehost).
  ⚠️ `./bin/nc run` på db-testar heng/OOM-ar i sandkassen (kjent native-runtime-treghet);
  runtime-verifisering køyrast på utviklarmaskin.

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
