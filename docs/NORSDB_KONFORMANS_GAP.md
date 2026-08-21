# NorsDB konformans-gap mot SQLite

> Fasit: golden-verdiar frå ekte `sqlite3 3.51.0`.
> Gjeld den **binære motoren** (`std/norsdb_db → norsdb_sql → norsdb_motor`), som er
> kanon bak `std.db`. (Den gamle JSON-motoren `_vm_db_*` i `vm.no` er fjerna.)
> Mål og faser: [NORSDB_SQLITE_PLAN.md](NORSDB_SQLITE_PLAN.md).

## Bekrefta divergensar (verifisert mot sqlite3)

| # | SQL | SQLite | NorsDB | Merknad |
|---|-----|--------|--------|---------|
| G2 | `INSERT` utan `id` i tabell `id INTEGER` (ikkje PRIMARY KEY) | `id = NULL` | auto-inkrement | NorsDB auto-tildeler alltid rowid; SQLite berre for `INTEGER PRIMARY KEY`. |
| NULL | 3-verdi-logikk / NULL vs `''` | eigen NULL-type | `NULL ≈ ""` | Strengbasert lagring; `IS NULL` fungerer, men NULL er ikkje fullt skilt frå tom streng. |

> G1 (AVG → heiltal) er **løyst**: `query_rader`/`query_text` gjev REAL (40.5); heiltals-kontekst
> (`query_int`) trunkerer som forventa.

## Manglande funksjonar (parser/motor støttar ikkje)

| Funksjon | Status | Merknad |
|---|---|---|
| Dato-/tidsfunksjonar (`date/time/datetime/strftime/julianday`) | manglar | Krev sjølv-implementert kalendermatematikk (`builtin.now_iso` finst ikkje i seed). |
| BLOB-literalar (`x'…'`) | manglar | Strengbasert lagring; blob ikkje distinkt frå TEXT. |
| Full 3-verdi NULL i lagring/samanlikning | manglar | Krev storage-endring (skilje NULL frå `''`). |
| Kostnadsbasert join-planleggar | manglar | I dag nested-loop + indeks-guard; ingen statistikk-basert omordning. |
| `SET DEFAULT` referanseaksjon (full DEFAULT-verdi) | delvis | Handterast som SET NULL. |

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
REAL-typar, parameter-binding, transaksjonar (`begin/commit/rollback`, `transaction`).

## Slik oppdaterer du fasit

Nye golden-verdiar skal alltid hentast frå ekte sqlite3, t.d.:

```bash
sqlite3 :memory: "CREATE TABLE t(a INTEGER); INSERT INTO t VALUES(1),(2); SELECT AVG(a) FROM t;"
```
