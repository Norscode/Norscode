# NorsDB → SQLite-kompatibel kjerne (app-nivå)

> Mål (valgt 2026-08-16): **NorsDB skal kunne erstatte SQLite for vanlig app-bruk** —
> ikke full SQLite-paritet. Ekte relasjonsdatabase i ren Norscode, uten eksterne
> avhengigheter (`external_runtime = false`). Se [NORSDB_KANON_KART.md](NORSDB_KANON_KART.md)
> for hva som er kanon (`std/norsdb.no` + `selfhost/vm.no` `_vm_db_*`).

## Hva «app-nivå» betyr

**Innafor scope:** CRUD, JOIN (INNER/LEFT/CROSS), GROUP BY/HAVING, aggregater,
subqueries, LIKE/IN/BETWEEN/IS NULL, CASE, constraints (PK/FK/UNIQUE/NOT NULL/
CHECK/DEFAULT), typer/affinity, indekser, transaksjoner med ACID, parameter-binding.

**Bevisst utafor scope:** kostnadsbasert query-planlegger, FTS, JSON1,
vindusfunksjoner, virtuelle tabeller, collations utover binær/nocase. Bygges kun
hvis en konkret app trenger det.

**Forutsetning som løper parallelt:** stabil + rask native runtime (B2-milepælen).
Uten den kan verken konformanssuite eller benchmarks kjøres — akkurat nå henger
native-runtimen på db-tester lokalt.

## Fasit: konformanssuite

«På nivå med SQLite» skal være et **grønt/rødt tall**, ikke en følelse.
`tests/norsdb_konformans/` skal inneholde SQL-scenarier som kjøres mot både ekte
`sqlite3` (system) og NorsDB, med diff av resultatet. Hver fase under legger til
scenarier; en fase er «ferdig» når dens del av suiten er grønn.

## Faser

### Fase 0 — Fundament & herding  *(~1–2 uker)*
- [x] Funn #1: `transaction()` committer gyldige UPDATE/DDL (gjort — `test_norsdb_tx_commit.no`)
- [x] Konformanssuite (golden fra ekte sqlite3): `tests/test_norsdb_konformans.no` + gap-register `docs/NORSDB_KONFORMANS_GAP.md`
- [x] **Parameter-binding** `execute_bundet`/`query_text_bundet`/`query_int_bundet` med `?`-plassholdere + escaping (SQL-injeksjon). Test: `test_norsdb_param_binding.no`
- [x] Ekte feilsignalering: ugyldig SQL → `execute` returnerer `-1` + `siste_feil(handle)`; gyldige 0-effekt-setninger gir fortsatt `0`. Test: `test_norsdb_feilsignal.no`
- [x] Pool: håndhev `maks` som idle-tak; nullstill tx-tilstand i `close()`. Test: `test_norsdb_pool.no`. (`pool_size` = ledige tilkoblinger *by design* — ttestet kontrakt i `test_db.no`.)

### Fase 1 — Relasjonell SQL  *(~3–6 uker)*
- [ ] Ekte tokenizer + recursive-descent-parser (erstatt token-heuristikk)
- [ ] Uttrykks-evaluator: aritmetikk, `||`, `CASE WHEN`, funksjoner (length/substr/upper/lower/coalesce/abs)
- [ ] WHERE: `AND/OR/NOT` + parenteser, alle sammenlikninger, `LIKE`, `IN`, `BETWEEN`, `IS [NOT] NULL`
- [ ] JOIN: INNER/LEFT/CROSS, tabellalias, kvalifiserte kolonner (`t.kol`)
- [ ] GROUP BY + HAVING + aggregater; DISTINCT fler-kolonne; ORDER BY fler-kolonne
- [ ] Subqueries: skalar, `IN (SELECT ...)`, `EXISTS`
- [ ] `INSERT ... SELECT`, `INSERT OR REPLACE`, UPSERT (`ON CONFLICT DO ...`)

### Fase 2 — Skjema, typer & constraints  *(~2–4 uker)*
- [ ] Typer/affinity (INTEGER/TEXT/REAL/BLOB/NUMERIC) + NULL-semantikk
- [ ] Håndhevede constraints: PK (+AUTOINCREMENT), UNIQUE, NOT NULL, DEFAULT, CHECK, FOREIGN KEY (ON DELETE/UPDATE)
- [ ] `ALTER TABLE` (ADD/RENAME/DROP COLUMN); enkle `VIEW`

### Fase 3 — Lagringsmotor & ytelse  *(måneder — det harde)*
- [ ] Sidebasert filformat (pager) i stedet for hele-fil-JSON
- [ ] B-tre for tabeller + ekte `CREATE INDEX`, brukt i spørring (likhet/range-oppslag)
- [ ] Delta-/side-skriving; unngå O(n)-klon per transaksjon

### Fase 4 — ACID, durabilitet & samtidighet  *(måneder, koblet til Fase 3)*
- [ ] Rollback-journal eller WAL; atomisk commit (minimum temp+rename, journal ideelt); fsync
- [ ] Crash-recovery ved åpning
- [ ] Fillåsing (mange lesere / én skriver), busy-timeout

### Fase 5 — Herding & konformans  *(løpende)*
- [ ] Parser-fuzzing; benchmark mot SQLite; utvid konformanssuite per feature
- [ ] Dokumentert støttet SQL-grammatikk

## Aksept for «app-kompatibel kjerne»

Fase 0–2 komplett + Fase 3 (indeks/pager) + Fase 4 (atomisk commit + recovery),
med konformanssuiten grønn på et definert app-SQL-korpus.
