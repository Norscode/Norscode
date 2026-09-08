# NorsDB vs PostgreSQL

_Sist oppdatert: 2026-09-07. Alle NorsDB-påstandar er runtime-verifiserte (`./bin/nc run`,
54/54 testar grøne) mot dokumentert PostgreSQL-semantikk og `sqlite3 3.51.0`._

## Kategori-skiljet (les dette først)

NorsDB er arkitektonisk ein **innebygd, enkelt-prosess, enkelt-fil-motor** — same klasse som
**SQLite**, ikkje PostgreSQL. Backend: `norsdb-pure-v1`, `external_runtime=false`,
`storage="binær-btre-wal"`. Rein Norscode: ingen C, ingen FFI, ingen server.

PostgreSQL er ein **klient-server fleirbrukar-RDBMS**. Å samanlikne dei direkte er delvis
eple-mot-appelsin: NorsDB matchar mykje av *SQL-språket*, men er ein annan tier på
samtidigheit, typar, yting, skala og drift.

## SQL-flata: NorsDB matchar Postgres langt forbi SQLite

| Område | NorsDB | Postgres |
|---|---|---|
| CRUD, full WHERE (AND/OR/NOT/parens, LIKE/**ILIKE**/GLOB/IN/BETWEEN/IS NULL) | ✅ | ✅ |
| JOIN INNER/LEFT/RIGHT/FULL/CROSS, multi-tabell, alias | ✅ | ✅ |
| GROUP BY/HAVING, ORDER BY, LIMIT/OFFSET, DISTINCT | ✅ | ✅ |
| UNION/INTERSECT/EXCEPT, subqueries (skalar/IN/EXISTS/korrelert/FROM) | ✅ | ✅ |
| CTE inkl. WITH RECURSIVE | ✅ | ✅ |
| Vindusfn: ROW_NUMBER/RANK/DENSE_RANK, **LAG/LEAD/FIRST_VALUE/LAST_VALUE/NTH_VALUE/NTILE**, aggregat OVER | ✅ | ✅ |
| Constraints: NOT NULL/PK/UNIQUE/CHECK/FK + ON DELETE/UPDATE-kaskade | ✅ | ✅ |
| Triggere BEFORE/AFTER, VIEW, ALTER TABLE, UPSERT (ON CONFLICT) | ✅ | ✅ |
| 3-verdi NULL-logikk | ✅ | ✅ |
| **RETURNING** på INSERT/UPDATE/DELETE | ✅ | ✅ |
| **UPDATE SET col = uttrykk** (aritmetikk/konkat/kolonne-ref, gamle rad-verdiar) | ✅ | ✅ |
| **Sekvensar**: CREATE/DROP SEQUENCE, nextval/currval/setval (ikkje-transaksjonelle) | ✅ | ✅ |
| **`::`-cast-syntaks** | ✅ (type ignorert) | ✅ |
| Skalar-fn: split_part, left/right, lpad/rpad, initcap, translate, concat(_ws), greatest/least, to_hex, sign, mod, ceil/floor/trunc, power, div, pi, string_agg, … | ✅ | ✅ |

## Der Postgres er ein heil tier over (medvite ikkje bygd)

| Dimensjon | NorsDB | PostgreSQL | Kvifor blokkert |
|---|---|---|---|
| Arkitektur | Innebygd, enkelt-fil | Klient-server-daemon | «anna produkt» (Fase 9) |
| Samtidigheit / MVCC / isolasjon | Éin skrivar, snapshot-tx | Full MVCC, Serializable | Krev trådar/samtidigheits-primitiv (eige spor) |
| Typesystem | 5 dynamiske taggar (tekst-lagra) | numeric/timestamptz/JSONB/arrays/UUID/… | Storage-refaktor |
| Kostnadsbasert planleggar | Indeks-bruk + EXPLAIN, ingen kostnadsmodell | Moden optimaliserar, parallell | Moot på tolka runtime — native-gated (B2) |
| Yting | Tolka (~166 µs/op) | Kompilert C | Native B2-runtime (eige spor) |
| Ekte pager (pread/pwrite) | Heile DB i minne + WAL/checkpoint | På-disk sider, shared buffers | Native fil-primitiv (eige spor) |
| Replikering / PITR / partisjonering / FTS / extensions | Ingen | Ja | Server-RDBMS (Fase 9) |

## Kjende SQL-gap (byggbare, men utsette)

- **Aggregat-`FILTER (WHERE …)`** — utsett: rører ~8 aggregat-kallstader inkl. group-by-stien
  (høg regresjonsrisiko mot 47+ testar).
- **SAVEPOINT / nøsta transaksjonar** — krev tx-snapshot-stack i adapteren.
- **DISTINCT ON**, **generate_series i FROM**, **NULLS FIRST/LAST** — byggbare oppfølgjarar.

## Botnlinje

- **Rett samanlikning er NorsDB ↔ SQLite.** Der er NorsDB no funksjonelt jamstilt på SQL-flata,
  og har i tillegg fleire Postgres-spesifikke trekk (RETURNING, ILIKE, sekvensar, `::`, string_agg,
  Postgres-vindusfunksjonar).
- **Mot Postgres**: NorsDB dekker språket brukbart, men ikkje tieren. Dei manglande bitane er
  ikkje bugs — dei er reelt gated på native-runtime- og samtidigheits-spora, og på server-RDBMS
  som eit anna produkt.
