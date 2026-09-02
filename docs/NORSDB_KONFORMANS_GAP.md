# NorsDB konformans-gap mot SQLite

> Fasit: golden-verdiar frå ekte `sqlite3 3.51.0`.
> Gjeld den **binære motoren** (`std/norsdb_db → norsdb_sql → norsdb_motor`), som er
> kanon bak `std.db`. (Den gamle JSON-motoren `_vm_db_*` i `vm.no` er fjerna.)
> Mål og faser: [NORSDB_SQLITE_PLAN.md](NORSDB_SQLITE_PLAN.md).

## Bekrefta divergensar (verifisert mot sqlite3)

| # | SQL | SQLite | NorsDB | Merknad |
|---|-----|--------|--------|---------|
| G2 | `INSERT` utan `id` i tabell `id INTEGER` (ikkje PRIMARY KEY) | `id = NULL` | auto-inkrement | NorsDB auto-tildeler alltid rowid; SQLite berre for `INTEGER PRIMARY KEY`. |

> G1 (AVG → heiltal) og NULL-vs-`''` er **løyste**. Full 3-verdi NULL: sentinel skil no NULL frå tom
> streng (`IS NULL`, propagering, COUNT/COALESCE, outer-join-utfyll — golden mot sqlite3).

## Manglande funksjonar (parser/motor støttar ikkje)

| Funksjon | Status | Merknad |
|---|---|---|
| `julianday` fraksjons-sekund (`%f` = SS.SSS med ekte ms) | delvis | `%f` gjev SS.000 (heiltals-sekund). Resten av dato/tid dekt. |
| Kostnadsbasert join-planleggar | manglar | Perf-optimering; gated på native (B2) — tolka runtime dominerer kostnad uansett. `SET DEFAULT` handterast som SET NULL. |

## Dekt i dag (grøn konformans, golden mot sqlite3 3.51.0)

CREATE/INSERT/UPDATE/DELETE/DROP, `SELECT` med full `WHERE`
(`= != <> < <= > >=`, `LIKE`, **`GLOB`**, `IN`, `BETWEEN`, `IS [NOT] NULL`,
`AND/OR/NOT` + parentesar), uttrykk (`+ − * / %`, `||`, `CASE`) og skalarfunksjonar,
`ORDER BY`/`LIMIT`/`OFFSET`, `GROUP BY`/`HAVING`, aggregat (`COUNT/SUM/AVG/MIN/MAX/
TOTAL/GROUP_CONCAT`, inkl. `COUNT(DISTINCT)`), `DISTINCT`, subqueries (skalar/`IN`/
`EXISTS`/korrelert/`FROM`), `UNION/INTERSECT/EXCEPT`, CTE (`WITH`/`WITH RECURSIVE`),
vindusfunksjonar, `JOIN` (INNER/LEFT/RIGHT/FULL/CROSS), constraints
(PK/UNIQUE/NOT NULL/CHECK/DEFAULT/FK), **FK `ON DELETE/UPDATE` CASCADE/SET NULL/RESTRICT**,
`INSERT OR IGNORE/REPLACE` + UPSERT, `ALTER TABLE`, `CREATE VIEW`,
**`CREATE TRIGGER` BEFORE + AFTER** (INSERT/UPDATE/DELETE, NEW/OLD),
`CREATE INDEX` (equality + range i planen), prepared statements (`?`), `EXPLAIN`,
REAL-typar, parameter-binding, transaksjonar (`begin/commit/rollback`, `transaction`),
**dato/tid** (`date/time/datetime/strftime/unixepoch/julianday` + modifikatorar `±N days/months/years…`,
`start of …`, `weekday N`; skotår-korrekt kalendermatte), **BLOB-literalar** (`x'…'`),
**full 3-verdi NULL** (NULL distinkt frå `''`; propagering, `IS NULL`, `COUNT(col)`/COALESCE,
outer-join-utfyll).

## Slik oppdaterer du fasit

Nye golden-verdiar skal alltid hentast frå ekte sqlite3, t.d.:

```bash
sqlite3 :memory: "CREATE TABLE t(a INTEGER); INSERT INTO t VALUES(1),(2); SELECT AVG(a) FROM t;"
```
