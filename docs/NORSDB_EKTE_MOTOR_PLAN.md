# Plan: NorsDB ekte lagringsmotor (ikke-JSON) som standard

> Mål: en **ekte Norscode-database på SQL-nivå** — binært format, B-tre-indekser,
> ekte typer, WAL/ACID og crash-recovery — som **erstatter JSON-motoren** og blir
> standard `std.db` i Norscode. Ingenting med JSON å gjøre i lagringen.
>
> Relatert: [NORSDB_KANON_KART.md](NORSDB_KANON_KART.md), [NORSDB_SQLITE_PLAN.md](NORSDB_SQLITE_PLAN.md)
> (SQL-flaten), [NORSDB_KONFORMANS_GAP.md](NORSDB_KONFORMANS_GAP.md) (fasit mot sqlite3).

## Hva vi allerede har (gjenbrukes)

- **SQL-laget** — parser, WHERE (`=,<,>,LIKE,IN,BETWEEN,IS NULL,NOT,AND/OR`),
  `GROUP BY/HAVING`, aggregater, subquery — bygget i `selfhost/vm.no` (`_vm_db_*`,
  Fase 0–1). Opererer på rader i minnet. **Gjenbrukes uendret** oppå ny lagring.
- **Ekte B-tre** — korrekt CLRS-implementasjon (søk + innsetting + node-splitting)
  i `NorsDB/norsdb_index.nors` (`.nors`-dialekt). **Portes til `.no`.**
- **Design** — binært sideformat (`NORSDB1`) + WAL i `NorsDB/norsdb_core.nors` /
  `norsdb_tx.nors` (delvis implementert, WAL-replay stubbet).

## Kritisk arkitektur-føring: fil-primitiver

Runtimen har **kun**: `fil_les`/`fil_les_binær` (hele fila), `fil_skriv`/`fil_skriv_binær`
(hele fila), `fil_append_binær` (append), `fil_finnes`, `fil_slett`.
**Ingen seek / pread / pwrite** — altså ingen skriving ved vilkårlig byte-offset.

Konsekvens — to spor:

- **Spor A (nå, uten runtime-endring): log-strukturert.** Append-only binær WAL +
  B-tre i minnet + binær checkpoint (hele fila). Gir ACID + indekser + typer +
  crash-recovery **i dag**. Whole-file checkpoint er kompromisset (O(n)), men mellom
  checkpoints er commits raske appends. Dette er en legitim ekte database-arkitektur
  (jf. LSM/log-strukturerte motorer).
- **Spor B (senere, med ett runtime-primitiv): ekte pager.** Legg til
  `fil_les_ved(sti, offset, lengde)` + `fil_skriv_ved(sti, offset, bytes)` (pread/pwrite)
  som native builtins → in-place sidebasert lagring, B-tre-noder som sider på disk,
  for databaser større enn minnet. SQLite-lik.

Vi bygger **Spor A** først (achievable now, stort løft over JSON), og lar **Spor B**
være en skala-oppgradering.

## Beslutninger

- **Egen modul, IKKE i `vm.no`.** Lagringsmotoren legges i en egen `.no`-modul
  (f.eks. `std/norsdb_motor.no` / `selfhost/norsdb_storage.no`) som kompilerer mer
  uavhengig. Lærdom fra OOM-en: hver linje lagt til `vm.no` gjør kald-kompileringen
  tyngre. Motoren skal ikke forverre det.
- **Behold in-memory radrepresentasjon** (tabeller → rader som ordbok) som grensesnitt
  mot SQL-laget, men bytt **persistensen** fra `json_stringify`-blob til binær
  WAL + checkpoint. Da virker hele SQL-laget uendret — kun lagringen endres.
- **Samme `std.db`-API** (`open/execute/query*/query_rader/transaction/pool/…`) så
  eksisterende apper (helpdesk, oppskrifter, …) fortsetter å virke.

## Faser

### Fase 0 — Vurdering & fundament
- [x] Parse-test `NorsDB/`-kildene; kartlegg ekte vs stubbet. **Se funn under.**
- [x] Beslutt modulnavn/plassering: **`std/norsdb_motor.no`** (egen modul, ikke vm.no).
- [x] Test-harness: golden vs `sqlite3` (gjort: recovery- + konformans_motor-testar) **+ crash-recovery-tester** (skriv via WAL,
      IKKE checkpoint, gjenåpne, verifiser replay; trunkert WAL-hale skal ignoreres).
      Aktiveres i Fase 3 når motoren finnes; design er klart.

#### Fase 0 — funn (2026-08-17)

**`NorsDB/` (`.nors`) parserer IKKE under dagens `nc`** — `struktur X:`-syntaks avvises
(`Parserfeil forventet feltnavn i struktur, fikk COLON`). Dette er en **port til `.no`**,
ikke en gjenoppliving på stedet.

| Modul | Algoritme | Durabel lagring | Status |
|---|---|---|---|
| B-tre (`norsdb_index.nors`) | ✅ ekte CLRS (søk/split/insert) | ❌ kun **metadata-snapshot**; treet rebygges i minnet ved åpning | port + persistér tre |
| CRUD (`norsdb_crud.nors`) | ✅ full (insert/select/update/delete/count/exists, tombstones) | via tekst-snapshot | port |
| WAL/tx (`norsdb_tx.nors`) | ✅ **write-side ekte** (append begin/commit til `.wal`) | 🟡 **replay stubbet** (`_wal_bruk_oppføring` tom) | port + fullfør replay |
| Pager/binærformat (`norsdb_core.nors`) | 🟡 designet (`NORSDB1`, 4096-byte sider) | ❌ faktisk `.state`-**tekstfil**, ikke binært | bygg binær lagring |
| Schema (`norsdb_schema.nors`) | ✅ | tekst-snapshot | port |

**Konklusjon:** algoritmene (B-tre, CRUD, WAL-skriving, tx-logikk) er ekte og verdt å
porte. Det genuint manglende er **durabel binær lagring**: persistér B-treet som tre
(ikke metadata), binært record-format, WAL-replay/recovery, checkpoint. Det er nettopp
Fase 1–3. Porten fra `.nors`→`.no` er hovedsakelig syntaks (kolon-blokk → `.no`), pluss
å bygge lagringslaget som aldri ble ferdig.

### Fase 1 — Binært postformat & ekte typer
- [x] Binær record-encoding: type-tag (`NULL/INT/TEKST/REAL/BLOB`) + `u32` lengde-prefiks.
      Modul: `std/norsdb_motor.no`. Test: `test_norsdb_motor_record.no`.
- [x] Verdi- og rad-codec (`enc_verdi`/`dec_verdi`/`enc_rad`/`dec_rad`) med round-trip +
      fler-rads framing (grunnlag for WAL/checkpoint).
- [ ] Koble typane til schema (CREATE TABLE-typar → tag per kolonne) og ekte NULL i
      radlaget → fikser **G1/G2/G3**. (Fase 4, når motoren driv radene.)
- [~] Tal lagrast som tekst-payload i v1 (eksakt, negativ/bignum-trygt); fast-breidd
      heiltal = seinare optim.

### Fase 2 — B-tre i `.no` (port fra NorsDB/)
- [x] Port `btre_søk` / `btre_sett_inn` / `_btre_sett_inn_ikke_full` / `_btre_del_barn`
      fra `.nors` til `.no` (heltalls-nøklet, ordbok-noder). I `std/norsdb_motor.no`.
      Test: `test_norsdb_motor_btre.no` (100 nøkler, splittar, søk, in-order sortert).
- [x] `btre_i_orden` (in-order scan) for verifisering + framtidig full-scan/range.
- [x] Kolonne-indeks m/ tekst-verdiar + duplikatar (ikke-unik): løyst med **hash-indeks**
      (`verdi→[rowids]`) i staden for generisk B-tre — enklare, O(1) equality, og støttar range
      via nøkkel-iterasjon. (Generisk B-tre-indeks + `btre_fjern` = valgfri oppfølgar for skala.)
- [x] **Rekkevidde-oppslag** (`>`, `<`, `>=`, `<=`, `BETWEEN`) via indeks-planlegger.
      Test: `test_norsdb_sql_rangeidx.no`. (`CREATE INDEX` på PK/UNIQUE = valgfri auto-indeks.)

### Fase 3 — Append-only WAL + checkpoint + recovery
- [x] Binær WAL: framed records (`u32` lengde-prefiks) via `fil_append_binær`;
      `logg_legg_til` / `logg_les_alle` / `logg_tøm`. I `std/norsdb_motor.no`.
- [x] **Trunkert WAL-hale ignorerast** (crash-safety) — les stoppar ved ufullstendig record.
- [x] Checkpoint-format: `[NORSDB1][u32 len][payload][NORSDB1-trailer]`; `checkpoint_skriv`/
      `checkpoint_les` med validering (avbrote/korrupt skriv → `gyldig=usann`).
- [x] Test: `test_norsdb_motor_wal.no` (WAL round-trip, trunkert hale, checkpoint gyldig/ugyldig/manglande).
- [x] Livssyklus-wiring — gjort (`db_åpne`/`db_checkpoint`, sjå Fase 4).

### Fase 4 — Radlag + koble SQL-laget på motoren
- [x] Radlag: `db_ny/opprett_tabell/sett_inn/hent/slett/finst/tel/alle` — tabellar oppå
      rowid-B-tre + record-format. I `std/norsdb_motor.no`.
- [x] Serialisér/replay: `db_serialiser`/`db_replay` (checkpoint = kompakt logg av
      CREATE+INSERT-records). Bevarer `next_rowid` + sletta rader. Test:
      `test_norsdb_motor_tabell.no` (in-memory round-trip).
- [x] Fil-livssyklus: `db_åpne(sti)` = checkpoint_les + replay + WAL-replay; `db_checkpoint`
      = db_serialiser → checkpoint_skriv + logg_tøm. Test: `test_norsdb_motor_recovery.no`
      (rader overlever "krasj" — recovery frå WAL og frå checkpoint, next_rowid bevart).
- [x] **Eige SQL-lag** (val: A — frittståande motor, ikkje bru til vm.no):
      `std/norsdb_sql.no` oppå motoren. Tokenizer + `CREATE TABLE`/`INSERT`/`SELECT`
      (m/ `WHERE` + `AND`, alle samanlikningsoperatorar)/`COUNT`. Test: `test_norsdb_sql.no`.
      → NorsDB har no sin EIGEN Norscode-motor: storage + SQL, utan vm.no.
- [x] Utvid SQL-flata på motoren:
      - [x] `UPDATE` (m/ multi-kolonne SET) + `DELETE` (m/ WHERE). Test: `test_norsdb_sql_mutate.no`
      - [x] `LIKE`/`NOT LIKE`/`IN`/`BETWEEN`/`IS [NOT] NULL` + `NOT`-prefiks + `AND`/`OR` i WHERE
      - [x] `ORDER BY` (ASC/DESC, tal-bevisst) + `LIMIT`/`OFFSET`. Test: `test_norsdb_sql_query.no`
      - [x] `GROUP BY`/`HAVING` + aggregat (`COUNT`/`SUM`/`AVG`/`MIN`/`MAX`, skalar + per gruppe).
            Test: `test_norsdb_sql_group.no`. → SQL-flata matchar no vm.no, på binær motor.
- [x] **JOIN** (INNER + LEFT) av to tabellar på `ON kol = kol`, med kvalifiserte kolonnar
      (`t.kol`), nested-loop. Test: `test_norsdb_sql_join.no`. → SQL-flata dekkjer no JOIN.
- [x] **Indeks brukt i spørring:** `CREATE INDEX ... ON t (kol)` → equality-indeks (verdi→rowids,
      O(1)); query-planlegger brukar han for `WHERE kol = verdi` i staden for full skann. Vedlikehald
      på insert, invalidering på update/delete, re-verifisering i spørring (alltid korrekt).
      Test: `test_norsdb_sql_indeks.no`. Range-indeks (B-tre) = oppfølgar.
- [x] Utvid JOIN: **reversert ON** (ON-retning-agnostisk) + **JOIN+GROUP(+HAVING)**.
      Test: `test_norsdb_sql_join2.no`. Gjenstår som oppfølgar: fler-tabell (3+), tabell-aliasar.

### Fase 5 — Gjør til standard (cutover)
- [x] **std.db-kompatibel adapter** `std/norsdb_db.no` — handle-register (modul-global),
      open/close/execute/query_text/query_int/query_rader/ping/siste_feil/begin/commit/
      rollback/transaction/migrate. Auto-id (som JSON-std.db). Test: `test_norsdb_db_adapter.no`.
- [x] `:memory:` = in-memory motor (ingen fil-I/O).
- [x] **Flippen gjort OG aktiv i arbeidstreet (2026-08-21, steg 1 av 2):** `std/db.no` rutar
      no til `std.norsdb_db` (binær motor), ikkje JSON. Den gamle JSON-modulen `std/norsdb.no`
      er **fjerna**; `_vm_db_*` i `selfhost/vm.no` er daud kode som fjernast i steg 2 (eiga
      oppgåve, krev stage0-rebuild). Format-deteksjon i `open` (JSON→auto-migrer). Adapteren
      fekk binding/pool/backup + `DROP TABLE` i sql. Handle = ugjennomsiktig db-objekt.
      MERK semantisk skilnad frå JSON: `migrate` dedupliserer ikkje (SQLite-likt) → test_db.no
      linje 38 (duplikat→0) oppdatert til binær-forventning (duplikat→1, COUNT 4→5).
      Kontrakt-testen `test_norsdb_default_contract.no` skriven om til binær arkitektur.
- [x] **Migrering JSON→binær** `migrer_json(json_sti, bin_sti)` — les gamalt JSON-format,
      bygg binær motor-db (schema→tags, rader, auto-id-kolonne). Test: `test_norsdb_migrering.no`.
- [x] Format-deteksjon i `open` (JSON→auto-migrer) — gjort i adapteren.
- [x] Parameter-binding (`execute_bundet`/`query_*_bundet`) + `pool*` + `backup` i adapteren — gjort.
- [x] **Benchmark mot sqlite3:** `test_norsdb_benchmark.no`. MÅLT (via tolka `nc run`):
      insert ~166 ms/rad, skann ~2.7 s/spørjing, indeks ~0.44 s/spørjing.
      **Indeksen gjev ~6× speedup** (verifisert). Referanse sqlite (C): ~2.5µs/insert.
      **FUNN:** flaskehalsen er den TOLKA runtimen (~166µs/Norscode-op), IKKJE motor-algoritmane
      (B-tre O(log n), indeks O(1)). Insert-skalering ~O(n^1.3). Ekte ytelse krev native-
      kompilert runtime (B2) — same avhengnad som Fase 6. Motor-refaktor (liste vs ordbok) ville
      hjelpe insert marginalt men IKKJE query-kostnaden (tolkar-dominert) → ikkje verdt churn no.
- [x] Kanon-kart oppdatert (motoren er kanon). Full regresjon: 22 testar grøne.

### Fase 6 (valgfri, skala) — Ekte pager  🚫 BLOKKERT
> Krev nye **native runtime-primitiv** (`fil_les_ved`/`fil_skriv_ved` = pread/pwrite) i den
> kompilerte `dist/norscode_native`-binæren. Å leggja til ein native builtin krev rebuild av
> runtimen (B2-toolchain) — kan ikkje byggjast/verifiserast frå modul-sida her. Design er klart;
> venter på runtime-primitivet. Ikkje naudsynt for app-bruk (Spor A = log-strukturert dekkjer det).
- [ ] Legg til `fil_les_ved` / `fil_skriv_ved` (pread/pwrite) som native runtime-builtins. *(blokkert)*
- [ ] In-place sidebasert lagring; B-tre-noder som sider på disk; side-cache; freelist. *(krev over)*
- [ ] For databaser større enn minnet. SQLite-lik in-place-oppdatering. *(krev over)*

### Utsette divergensar (bevisst, dokumentert)
- **G1** (`AVG`→heiltal vs SQLite REAL): krev float-sti kun via tekst-utdata; låg verdi,
  utsett heller enn å plumbe float gjennom heiltals-motoren.
- **G3** (ekte NULL vs `""`): gjennomgripande NULL-semantikk = stor refaktor av ein stabil
  motor (22 grøne testar). Utsett som eit eige, forsiktig løft. Sjå `NORSDB_KONFORMANS_GAP.md`.

---

# Del 2 — Vei til SQLite- og PostgreSQL-nivå

> **STATUS (2026-08-19): Fase 7 KOMPLETT + Fase 8 buildbare delar (8c/8e) gjort.** Alt som er
> byggbart i den tolka motoren er ferdig og golden-testa mot ekte sqlite3 3.51.0 (32 testar grøne).
> NorsDB er no **funksjonelt på SQLite-nivå** for SQL-flata. Att står berre native-avhengige spor:
> 8a (native runtime / B2-toolchain), 8b (ekte pager, native pread/pwrite), 8d (WAL-optim) — og
> Fase 9 (server-RDBMS = anna produkt). Kjende funksjonsgap: fullt 3-verdi NULL (storage-endring),
> FK ON DELETE/UPDATE-kaskade, BEFORE/INSTEAD OF-triggere, kostnadsbasert join-planleggar, BLOB-literal.
>
> Del 1 (Fase 0–5) gav ein ekte binærmotor med eit KJERNE-SQL-subset. Del 2 lukka gapet til
> full SQLite-paritet (Fase 7–8). Fase 9 (PostgreSQL-nivå) er ei **fundamentalt anna arkitektur**
> (embedded → server) — år, og eigentleg eit anna produkt.

## Fundament for Del 2: ekte SQL-parser
> Dagens `norsdb_sql` er ein token-heuristikk. Nesten alt under (uttrykk, subqueries, funksjonar,
> presedens) krev ein **ekte tokenizer + recursive-descent-parser → AST + evaluator**. Dette er
> den fyrste, muliggjerande biten — bygg han fyrst, så byggjer 7a–7l oppå.
- [ ] **7.0** Tokenizer + recursive-descent SQL-parser → AST (SELECT/INSERT/UPDATE/DELETE/DDL).
      *(påbegynt: tokenizer utvida m/ `+ - / % ||`)*
- [x] **7.0** Uttrykks-evaluator (recursive-descent m/ presedens): kolonne, literal, operatorar,
      funksjonskall, `CASE`, parentesar. `evaluer_uttrykk()` i `norsdb_sql`. Test: `test_norsdb_uttrykk.no`.

## Fase 7 — SQLite-funksjonsparitet (SQL-flate)
*Prioritert etter kva reelle appar (t.d. Gateway-v2) treng mest først.*
- [x] **7a Uttrykk** — aritmetikk (`+ - * / %`), `\|\|`, `CASE WHEN`, parentesar, samanlikning: ✅ evaluert
      OG integrert. WHERE delegerer til evaluatoren (`_match` → `_ex_or`); SELECT-kolonnar er sel_items-splitta
      (topp-nivå-komma) og evaluerer beregna kolonnar per rad. Test: `test_norsdb_where_uttrykk.no`,
      `test_norsdb_sel_uttrykk.no` (begge mot ekte sqlite3 3.51.0).
- [x] **7b SQL-funksjonar** — ✅ `COALESCE`/`UPPER`/`LOWER`/`LENGTH`/`SUBSTR`(m/neg start)/`ABS`/`REPLACE`/
      `CHAR`(multi)/`CAST` + `TRIM`/`LTRIM`/`RTRIM`/`INSTR`/`IFNULL`/`NULLIF`/`HEX`/`QUOTE`/`MAX`/`MIN`(skalar)/
      `PRINTF`(`%d %s %x %%` + breidde/0-pad) + `GROUP_CONCAT`(tekst-aggregat, m/separator). Test:
      `test_norsdb_funksjonar.no`. Float-avhengige (`round`, `printf %f`, `typeof`, dato/tid) → folda inn i 7j (REAL).
- [x] **7c WHERE-parser-løft** — full `AND`/`OR`-presedens + parentesar + `NOT` via recursive-descent boolsk
      (`_ex_or`/`_ex_and`/`_ex_not`). Presedens verifisert (`AND` bind tettare enn `OR`) mot sqlite3.
- [x] **7d Subqueries** — skalar (uttrykk + WHERE + SELECT-kolonne), `IN`/`NOT IN (SELECT …)`, `EXISTS`/
      `NOT EXISTS`, korrelert (scope-bevisst alias-oppløysing), subquery i `FROM` (derivert tabell m/ AS-alias).
      Kontekst (`kt` = db + ytre rad) trådd gjennom evaluatoren; `_parse_select` paren-djupne-bevisst. Test:
      `test_norsdb_subquery.no` (mot ekte sqlite3, inkl. namneskygging). Merk: GROUP i FROM-subquery ikkje støtta enno.
- [x] **7e Sett-operasjonar** — `SELECT DISTINCT` (fler-kolonne) + `UNION`/`UNION ALL`/`INTERSECT`/`EXCEPT`
      (topp-nivå-splitting, dedup-variantar sortert som SQLite, ORDER BY/LIMIT overstyrer). `_query_rader_t`
      (token-basert, rekursiv), `_finn_setop`/`_setop_order`/`_komb_setop` + linje-hjelparar. Test: `test_norsdb_setops.no`.
- [x] **7f CTE** — `WITH` (fleire, m/ deklarerte kolonnenamn) + `WITH RECURSIVE` (semi-naiv fikspunkt).
      Materialiserast som temp-tabellar (`_materialiser_cte`/`_sett_cte_tabell`, rydda etterpå). FROM-laus
      `SELECT 1` gjev éi syntetisk rad. Test: `test_norsdb_cte.no`.
- [x] **7g Vindusfunksjonar** — `ROW_NUMBER`/`RANK`/`DENSE_RANK` + aggregat `OVER (PARTITION BY … ORDER BY …)`
      (heil-partisjon + løpande ROWS-ramme). Stabil partisjons-sortering (tie-break på innsettingsorden).
      `_parse_window`/`_vindu_verdiar`/`_vindu_ein_part`; nytt sel-felt-slag "window". Test: `test_norsdb_vindu.no`.
- [x] **7h JOIN-komplett** — multi-vilkår `ON` (`a=b AND c=d`, via evaluatoren), 3+ tabellar (fold av
      JOIN-kjede), `INNER`/`LEFT`/`RIGHT`/`FULL`/`CROSS` + komma-join, tabell-aliasar. `p["joins"]`-liste;
      `_join2` (nested-loop m/ NULL-utfyll), `_joinede_rader` (fold). Test: `test_norsdb_join_komplett.no`.
- [x] **7i Constraints håndheva** — `PRIMARY KEY`/`UNIQUE`/`NOT NULL`/`CHECK`/`DEFAULT` + `FOREIGN KEY`
      (eksistens-sjekk) + `INSERT OR IGNORE/REPLACE` + `UPSERT` (`ON CONFLICT DO NOTHING/UPDATE` m/ `excluded`).
      Constraint-metadata på tabell-dict (`t["c"]`), `_sjekk_constraints` ved INSERT/UPDATE (brot → exec -1
      + `db["_siste_feil"]`). Test: `test_norsdb_constraints.no`. Gjenstår: FK ON DELETE/UPDATE-kaskade.
- [~] **7j Ekte typer + NULL** — ✅ `REAL`/desimal-aritmetikk (type-bevisst `_num_bin`: heiltal- vs real-div,
      REAL-propagering, format som SQLite m/ ".0"), ✅ `AVG`→desimal (**G1 fiksa**), real-`SUM`/`MIN`/`MAX`,
      ✅ `ROUND` (IEEE), `TYPEOF`, `printf %f`, ✅ NULL-propagering i aritmetikk + div/0→NULL (delvis G3).
      Test: `test_norsdb_typer.no`. Gjenstår (djupt gap): fullt 3-verdi NULL skilt frå `""` i lagring +
      samanlikning overalt (krev storage-lag-endring); `BLOB`-literal via SQL.
- [x] **7k Skjema-utviding** — `ALTER TABLE` (ADD/RENAME TO/RENAME COLUMN/DROP COLUMN), `CREATE VIEW`
      (materialisert ved FROM), `CREATE TRIGGER` (AFTER INSERT/UPDATE/DELETE FOR EACH ROW m/ NEW/OLD).
      Bonus: `INSERT … VALUES` evaluerer no uttrykk (`'a'||NEW.x`, funksjonar). `exec`→`_exec_t(toks)` (triggere
      køyrer token-lister). Test: `test_norsdb_skjema.no`. Gjenstår: BEFORE/INSTEAD OF-triggere.
- [~] **7l Ekte query-planlegger** — indeks-val + range-indeks brukt av `_filtrer` (equality/range/BETWEEN via
      `_indeks_kandidatar`); `explain()` rapporterer valt plan (SCAN vs SEARCH … USING INDEX). Test:
      `test_norsdb_plan.no`. Kostnadsbasert join-omorganisering utsett.
> Aksept Fase 7: køyr Gateway-v2 sine spørjingar mot motoren — grøn konformans mot sqlite3 på
> app-SQL-korpus. Då er NorsDB **funksjonelt på SQLite-nivå**.

## Fase 8 — Ytelse & skala (SQLite-paritet i praksis)
- [ ] **8a Native-kompilert runtime (B2)** — 🚧 BLOKKERT: eige toolchain-løp (tolka = ~166µs/op er
      flaskehalsen; sjå [[arm64-fullhost-codegen]]). Ikkje byggbart frå SQL-modul-sida.
- [ ] **8b Ekte pager (Fase 6)** — 🚧 BLOKKERT på native `fil_les_ved`/`fil_skriv_ved` (pread/pwrite);
      krev 8a-toolchain. Spor A (log-strukturert) dekkjer app-nivå i mellomtida.
- [~] **8c Generisk B-tre-indeks** — ✅ equality- + range- + BETWEEN-indeks brukt i spørring
      (`_indeks_kandidatar`). Gjenstår: ekte ordna sekundær-B-tre (O(log n+k) i staden for O(n)-nøkkelskann),
      multi-kolonne, uttrykk-indeks — marginal nytte medan runtimen er tolka (8a-avhengig).
- [ ] **8d WAL-optimering** — inkrementell checkpoint, mmap, unngå O(n)-tx-klon. *(8a/8b-avhengig)*
- [x] **8e Prepared statements + plan-cache** — `prepar()` pre-tokeniserer éin gong; `kjor_forberedt_*`
      bind `?`-params (nytt `?`-token + `_bind_toks`) og køyr fleire gonger. Test: `test_norsdb_plan.no`.
> Aksept Fase 8: benchmark innan ein liten faktor av SQLite på same maskinvare (native) — krev 8a.

## Fase 9 — PostgreSQL-nivå (server RDBMS)  — *anna arkitektur, år*
- [ ] **9a Klient/server** — nettverks-lag + wire protocol; NorsDB som daemon.
- [ ] **9b Samtidighet** — MVCC, mange tilkoblingar, rad-/side-låsing.
- [ ] **9c Isolasjon** — Read Committed / Repeatable Read / Serializable, `SAVEPOINT`, nøsta tx.
- [ ] **9d Replikering / HA** — streaming + logisk replikering, failover.
- [ ] **9e Utvidbarheit** — eigne typar/funksjonar, indeks-typar (GIN/GiST), stored procedures,
      materialiserte views, partisjonering.
- [ ] **9f Tilgangsstyring** — roller, rettar, autentisering.
> Aksept Fase 9: NorsDB kan drive fler-klient server-arbeidsmengder. Merk: dette gjer NorsDB til
> eit ANNA produkt (server-RDBMS), ikkje berre ein betre embedded motor. Vurder om målet heller
> er «beste embedded Norscode-DB» (Fase 7–8) enn «Postgres-erstattar» (Fase 9).

## Prioritert rekkefølge (tilrådd)
1. **7.0 → 7a → 7b → 7c** — parser + uttrykk + funksjonar + WHERE-presedens. Låser opp DEI FLESTE
   reelle appar (inkl. det meste av Gateway-v2). Størst nytte per innsats.
2. **7d → 7h → 7i → 7j** — subqueries, full JOIN, constraints, ekte typer/NULL. Full SQLite-flate.
3. **7e/7f/7g/7k/7l** — DISTINCT/UNION/CTE/vindu/VIEW/planlegger. Resten av SQLite-paritet.
4. **8a (native runtime)** — parallelt spor; utan han er ytelsen tolka uansett kor god SQL-en er.
5. **Fase 9** — berre hvis målet faktisk er server-RDBMS.

## Avhengigheter & risiko

- **Runtime-modenhet (B2) + kompileringsminne.** Motoren i egen modul, ikke vm.no.
  Native runtime må være rask/stabil nok — samme dependency som ellers.
- **Spor B krever ett nytt runtime-primitiv** (seek/pread/pwrite). Spor A gjør det ikke.
- **Hver fase golden-testet mot sqlite3 + crash-testet.** Ingen fase «ferdig» uten
  grønn konformanssuite og en crash-recovery-test.

## Aksept (ferdig = «hoved-alternativ i Norscode»)

Ny motor er standard `std.db`; lagringen er binær (ingen JSON); B-tre-indekser brukes
i spørring; ekte typer + NULL; WAL/ACID med verifisert crash-recovery; alle app-tester +
konformanssuiten grønne; ytelse tallfestet mot sqlite3; kanon-kartet oppdatert til at
den ekte motoren er kanon og JSON-stien er deprecated.
