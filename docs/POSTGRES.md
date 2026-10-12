# Postgres-klient i rein Norscode (std.pg + std.pg_wire)

Wire-protokoll v3 over `std.socket`, utan libpq, C eller Python. Verifisert mot PostgreSQL 17.

| Modul | Innhald |
|---|---|
| `std/pg_wire.no` | Rein protokoll: meldingsbyggjarar, meldingsdekoding, SCRAM-SHA-256, MD5, typekonvertering (tekst og binær). Ingen sokkel, ingen tilstand, testbar byte-eksakt. |
| `std/pg.no` | Transport (sokkel, TLS), tilkoplingstilstand og API. |
| `std/pg_csv.no` | COPY ... FORMAT csv. |
| `std/db_url.no` | Same flate som `std.db`, motor vald av adressa (`postgresql://` eller NorsDB-fil). Pool og ein gongs retry for SELECT. |

## Bruk

```
bruk std.pg som pg

la c = pg.open("host=localhost port=5432 dbname=app user=ola password=hemmeleg")
pg.execute(c, "INSERT INTO brukar (namn, alder) VALUES ($1, $2)", ["Ola", "30"])
la rader = pg.query_typa(c, "SELECT id, namn, alder FROM brukar WHERE alder > $1", ["18"])
pg.close(c)
```

DSN: `host port dbname user password` pluss `sslmode` (`disable` standard, `prefer`, `require`, `verify-full`),
`sslrootcert=<PEM>`, `hostaddr`, `application_name`. URL: `pg.open_url("postgresql://u:pw@host:5432/db?sslmode=verify-full&sslrootcert=/ca.pem")`.

## Dekning

| Område | Status |
|---|---|
| Autentisering | trust, klartekst, MD5, SCRAM-SHA-256 (RFC 5802/7677, med SASLprep og verifisering av tenarsignaturen). Ikkje: SCRAM-SHA-256-PLUS (kanalbinding), GSS/SSPI/Kerberos. |
| Enkel spørring (`Q`) | `query_raw`, `execute_raw`, `query_fleire` (fleire setningar, eitt resultat per kommando). |
| Utvida spørring | Parse/Bind/Describe/Execute/Sync med parametrar (`$1`...), tekst- og binærformat. `query`, `execute`, `query_typa`, `query_binar`. |
| Førebudde setningar | `prepare`, `query_prep`, `query_prep_binar`, `execute_prep`, `lukk_prep`. |
| Parametrar | Tekst som standard (tenaren vel type), `pg.sql_null()` for NULL, `pg.param(oid, verdi)` for binær parameter med deklarert type (bool, int2/4/8, float8, text/varchar/json, bytea, uuid). |
| Typar | bool, int2/int4/int8/oid, float4/float8 (desimaltall), numeric (tekst, taplaus), text/varchar/char/name, bytea (liste_heiltall), date, time, timestamp, timestamptz (tekst), json, jsonb, uuid (tekst), arrayar av desse (nøsta lister, NULL-element). Ukjende typar kjem som tekst. Ikkje: interval, geometriske typar, range, inet i binærformat (kjem då som rå bytestreng). |
| Transaksjonar | `begin`, `commit`, `rollback`, `savepoint`, `rollback_to`, `med_transaksjon(conn, f)`, `tx_status(conn)` (`I` / `T` / `E` frå ReadyForQuery), `i_transaksjon`. |
| Feil | Sjå under. |
| Notice | `notisar(conn)` (tøm og hent), `sett_notiskrok(f)` (kalla for kvar NoticeResponse). |
| LISTEN/NOTIFY | `lytt`, `varsle`, `varsel(conn)`. Varsel blir lesne med neste svar frå tenaren (køyr t.d. `SELECT 1`). Ingen blokkerande venting. |
| COPY | `copy_inn`, `copy_ut` (tekstformat), `copy_inn_csv`, `copy_ut_csv`. Ikkje binært COPY-format. |
| Tilkopling | `parameter(conn, namn)` (ParameterStatus), `backend_pid`, `avbryt(conn)` (CancelRequest). |
| Pool | `pool_ny(dsn, maks)`, `pool_hent`, `pool_slepp` (rullar tilbake open transaksjon, kastar broten tilkopling), `pool_stat`, `pool_lukk`. Ikkje trådsikker. |
| TLS | SSLRequest, så TLS 1.3 med `std.tls13_klient` (rein Norscode). Sertifikatet blir ALLTID verifisert mot `sslrootcert`, inkludert vertsnamn; det finst ingen modus utan verifisering. Berre TLS 1.3. |

## Feil (R4, sjå `docs/UNNTAK.md`)

Unntak er tekst `"Type: melding"`, så `fang (e: Type)` verkar utan nytt språkstøtte og òg i AOT:

* `PgServerFeil: melding (SQLSTATE 23505)` – ErrorResponse frå tenaren (òg autentiseringsfeil, 28P01). Tilkoplinga er brukbar etterpå.
* `PG-feil: ...` – klient- og transportfeil (tidsavbrot, broten tilkopling, manglande passord, ikkje støtta metode). Tilkoplinga blir forkasta.
* `PgProtokollFeil: ...` – brot på protokollen (frå `std.pg_wire`).

```
prøv { pg.execute(c, "INSERT ...", [...]) } fang (e: PgServerFeil) {
    hvis pg.er_unik_brot(e) { ... }                 # 23505
    la felt = pg.feil_felt()                        # S, C, M, D, H, P, W, s, t, c, d, n
    skriv(pg.feil_sqlstate(e) + " " + felt["n"])    # t.d. constraint-namnet
}
```

Hjelparar: `feil_sqlstate`, `feil_klasse` (to første teikn, `23` integritet), `er_server_feil`, `er_unik_brot`,
`er_fremmednokkel_brot`, `er_vranglas` (40P01), `er_serialiseringsfeil` (40001), `feil_felt`. Passord blir aldri
teke med i feilmeldingar.

## NorsDB og ORM

`std.db_url.open("postgresql://...")` gir eit handtak med same flate som `std.db` (`execute`, `query`, transaksjonar,
pool), så NorsDB-kode kan byte backend med adressa. `std.orm` er bygd på `std.db` (strenghandtak) og følgjer ikkje
`db_url`-handtaket; ORM mot Postgres er ikkje kopla på.

## Testar

Rein protokoll (ingen nettverk): `tests/test_pg_wire.no` (byte-eksakte meldingar rekna ut uavhengig, SCRAM-testvektoren
frå RFC 7677 §3, MD5, dekoding av alle backend-meldingar, binær og tekst-konvertering inkl. numeric, dato/tid, arrayar),
`tests/test_saslprep.no`, `tests/test_pg_utan_tenar.no`.

Mot ekte tenar (miljøstyrt; utan variablane hoppar testen over med melding og består, `NORSCODE_PG_REQUIRE=1` gjer
manglande DSN til feil). `tools/nc_test.no` sender `NORSCODE_PG_TEST_*` vidare til testbarna.

| Variabel | Test |
|---|---|
| `NORSCODE_PG_TEST_DSN` | `test_pg_integrasjon`, `test_pg_klient`, `test_pg_copy_csv` (trust, superbrukar) |
| `NORSCODE_PG_TEST_CLEAR_DSN`, `_MD5_DSN`, `_SCRAM_DSN` | autentiseringsmetodane i `test_pg_integrasjon` / `test_pg_klient` |
| `NORSCODE_PG_TEST_URL` | `test_db_url` |
| `NORSCODE_PG_TEST_TLS_DSN` + `_TLS_CA` | `test_pg_tls` |

Eigen tenar (Docker):

```
docker run -d --name rCpg-db -e POSTGRES_PASSWORD=adm -e POSTGRES_DB=test -p 127.0.0.1:55442:5432 postgres:17
docker exec rCpg-db psql -U postgres -d test -c "SET password_encryption='md5'; CREATE ROLE u_md5 LOGIN SUPERUSER PASSWORD 'md5-pw'; SET password_encryption='scram-sha-256'; CREATE ROLE u_scram LOGIN SUPERUSER PASSWORD 'scram-pw'; CREATE ROLE u_clear LOGIN SUPERUSER PASSWORD 'clear-pw'; CREATE ROLE u_trust LOGIN SUPERUSER"
# pg_hba.conf: host all u_trust all trust / u_md5 md5 / u_clear password / u_scram scram-sha-256, så pg_reload_conf()
```

Port 55432 er tatt av kontainaren `norscode-pg` på utviklarmaskina, så testane brukte 55442.

## Avgrensingar

* Ytelse: tolka VM-en gjer SCRAM (PBKDF2 4096 rundar) og TLS-handtrykket (~20 s målt) dyre per tilkopling; bruk pool.
* Synkron klient: éi førespurnad om gongen per tilkopling, ingen pipelining, ingen asynkron venting på varsel.
* Resultatrader blir bygde i minnet (ingen cursor/portal-straumlesing med `Execute(maks_rader)`).
* AOT: ikkje verifisert. Alt er testa gjennom `./bin/nc` (VM-vegen); klienten er ikkje køyrd som AOT-binær.
