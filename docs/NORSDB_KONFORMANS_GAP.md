# NorsDB konformans-gap mot SQLite

> Fasit: golden-verdiar frå ekte `sqlite3 3.51.0`.
> Gjeld den **binære motoren** (`std/norsdb_db → norsdb_sql → norsdb_motor`), som er
> kanon bak `std.db`. (Den gamle JSON-motoren `_vm_db_*` i `vm.no` er fjerna.)
> Mål og faser: [NORSDB_SQLITE_PLAN.md](NORSDB_SQLITE_PLAN.md).

## Bekrefta divergensar (verifisert mot sqlite3)

> Ingen kjende korrektheits-divergensar att. Sjå «Løyste gap» under.

### Løyste gap

- **G1** (AVG → heiltal) og NULL-vs-`''`: løyste. Full 3-verdi NULL — sentinel skil NULL frå
  tom streng (`IS NULL`, propagering, COUNT/COALESCE, outer-join-utfyll — golden mot sqlite3).
- **G2** (`INSERT` utan `id`, rowid-alias): **løyst**. Berre ein éin-kolonne `INTEGER PRIMARY KEY`
  (deklarert type nøyaktig `INTEGER`, kolonne- eller tabell-form) er rowid-alias og auto-tildeler
  neste rowid. Vanleg `id INTEGER` utan PK let no `NULL` stå, som SQLite. `INT PRIMARY KEY`
  (ikkje `INTEGER`) er heller ikkje alias. Verifisert i `tests/test_norsdb_konformans_gap.no`.
- **`strftime` `%f`** (fraksjons-sekund): **løyst**. Dato-pipelinen ber no millisekund, så `%f` gjev
  ekte `SS.SSS` (t.d. `56.789`, `56.500`), medan `SS.000` står ved manglande fraksjon.
  `datetime()/time()` droppar framleis fraksjon (heiltals-sekund), som SQLite. `julianday` reknar
  no i ms internt (fraksjons-korrekt). Verifisert i `tests/test_norsdb_konformans_gap.no`.

## Manglande funksjonar (parser/motor støttar ikkje)

| Funksjon | Status | Merknad |
|---|---|---|
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
`start of …`, `weekday N`; skotår-korrekt kalendermatte; **`strftime %f` fraksjons-sekund `SS.SSS`**),
**BLOB-literalar** (`x'…'`),
**full 3-verdi NULL** (NULL distinkt frå `''`; propagering, `IS NULL`, `COUNT(col)`/COALESCE,
outer-join-utfyll).

## Slik oppdaterer du fasit

Nye golden-verdiar skal alltid hentast frå ekte sqlite3, t.d.:

```bash
sqlite3 :memory: "CREATE TABLE t(a INTEGER); INSERT INTO t VALUES(1),(2); SELECT AVG(a) FROM t;"
```
