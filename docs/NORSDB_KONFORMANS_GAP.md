# NorsDB konformans-gap mot SQLite

> Fasit: `tests/test_norsdb_konformans.no` (golden-verdier fra ekte `sqlite3 3.51.0`).
> Denne fila sporer der NorsDB **avviker fra** eller **manglar** SQLite-åtferd.
> Mål og faser: [NORSDB_SQLITE_PLAN.md](NORSDB_SQLITE_PLAN.md).

## Bekrefta divergensar (verifisert mot sqlite3)

| # | SQL | SQLite | NorsDB | Fase | Merknad |
|---|-----|--------|--------|------|---------|
| G1 | `SELECT AVG(alder) FROM folk` | `40.5` | `40` | 1/2 | Heiltalsdivisjon; manglar REAL/typer. Krev tekst-/desimal-retur. |
| G2 | `INSERT` utan `id` i tabell `id INTEGER` (ikkje PRIMARY KEY) | `id = NULL` | auto-inkrement | 2 | NorsDB auto-tildeler alltid id; SQLite berre for `INTEGER PRIMARY KEY`. |

## Manglande funksjonar (parser/motor støttar ikkje)

Verifisert fråverande i `selfhost/vm.no` (`_vm_db_*`). Kvar blir ein testcase i suiten når han er implementert.

| Funksjon | Status | Fase |
|---|---|---|
| `JOIN` (INNER/LEFT/CROSS) | manglar | 1 |
| `GROUP BY` / `HAVING` | manglar | 1 |
| `LIKE` / `GLOB` | manglar | 1 |
| `IN (...)` / `IN (SELECT ...)` | manglar | 1 |
| `BETWEEN` | manglar | 1 |
| `IS [NOT] NULL` / NULL-semantikk | manglar | 1/2 |
| `AND` / `OR` / `NOT` + parentesar i WHERE | delvis/uklar | 1 |
| `CASE WHEN` + uttrykk (`\|\|`, aritmetikk) | manglar | 1 |
| `INSERT ... SELECT`, `INSERT OR REPLACE`, UPSERT | manglar | 1 |
| Constraints: PK/FK/UNIQUE(håndheva)/NOT NULL/CHECK/DEFAULT | manglar | 2 |
| Typer / affinity / NULL | strengbasert | 2 |
| `ALTER TABLE`, `VIEW` | manglar | 2 |
| Ekte indeks (`CREATE INDEX`) brukt i plan | manglar | 3 |
| Aggregat på REAL, fleir-kolonne GROUP | manglar | 1/2 |

## Dekt i dag (grøn konformans)

CREATE/INSERT/UPDATE/DELETE/DROP, `SELECT` (éin tabell + subquery i FROM),
`WHERE` med `= != <> < <= > >=`, `ORDER BY ASC/DESC`, `LIMIT`/`OFFSET`,
`COUNT/SUM/MIN/MAX`, `DISTINCT`, parameter-binding (`?`), transaksjonar
(`begin/commit/rollback`, `transaction`).

## Slik oppdaterer du fasit

Nye golden-verdiar skal alltid hentast frå ekte sqlite3, t.d.:

```bash
sqlite3 :memory: "CREATE TABLE t(a INTEGER); INSERT INTO t VALUES(1),(2); SELECT AVG(a) FROM t;"
```
