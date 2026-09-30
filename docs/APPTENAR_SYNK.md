# Apptenar og synk-tenar (A6 og A7)

Status: milepælane A6 (`std/apptenar.no`) og A7 (NSP/1-tenaren i `std/synk.no`) frå app-kjensle-planen. Klienten (W9, `std/synk_klient.no` i WASM) er skildra i [WASM_KLIENT.md](WASM_KLIENT.md) og kort under «Klienten (W9)». Alt er rein Norscode utanfor `nc_main`-lukkinga, så ingenting krev reseed.

| Fil | Rolle |
|---|---|
| `std/apptenar.no` | HTTP/1.1-tenar under `nc run` med same ruter, vakter og responskontrakt som `nc serve` |
| `std/synk_kjerne.no` | NSP/1-kjernen: HLC, samanfletting per felt, validering, JSON. Same kode i VM-en og som WASM |
| `std/synk_lager.no` | Lagring for synk-tenaren: minne + journal + atomisk bilete |
| `std/synk.no` | NSP/1-endepunkta `hallo`, `push`, `pull` og `hendingar` (SSE) |
| `std/deploy.no` | `apptenar_miljø`, `generer_apptenar_systemd` og `generer_apptenar_nginx` (nye funksjonar) |

## Når apptenar og når `nc serve`

`nc serve` er framleis rett for sider utan lagring: han er native, startar raskt og treng ingen caps. Apptenaren er for appar som treng det `nc serve` ikkje har på dagens seedar [V i kartlegginga]:

| | `nc serve` | apptenar (`nc run`) |
|---|---|---|
| Tilkoplingar | éin om gongen, ingen keep-alive | mange opne, keep-alive, tak (standard 64) |
| Ledig socket | stoppar alle andre (3,7 s TTFB målt) | stoppar ingen (4 ms målt) |
| Header i fleire TCP-segment | 400 | les i lykkje |
| Tidsgrenser | ingen | header 5 s, ledig 5 s, kropp 10 s, skriving 30 s |
| HEAD | feil Content-Length, manglar Content-Type og ETag | same headerar som GET, utan kropp |
| 304 | berre nøyaktig lik sterk ETag | sterk og svak ETag, liste og `*`, med Cache-Control og Vary |
| Response-mellomvare | berre `/helse` | alle ruter |
| Unntak i vakt | tek ned tenaren | 500, tenaren lever |
| Handlarunntak | 500 med feilteksten i kroppen | 500 «Intern feil», teksten berre i loggen |
| Disk og env | nekta | etter prosessen sin policy (deploy-malen: disk i datamappa) |
| Modulglobalar | «Ukjent global variabel» | verkar |
| SSE | éi hending, så lukka | straumar med heartbeat og kanalar |
| Stiparametrar | berre `{namn:type}` | `{namn:type}` og `{namn}` |

## Apptenaren (`std/apptenar.no`)

```
bruk std.apptenar som app

funksjon start() -> heltall {
    returner app.lytt({"port": 8080, "respons_mw": ["__main__.merk_svar"]})
}
```

`lytt(konfig)` blokkerer til `app.stopp()` blir kalla (frå ein handlar) eller `maks_ms` er gått, og gjev 0 (1 om lyttinga feila). Han skriv `APPTENAR klar <vert>:<port> backend=<poll|…>` når socketen lyttar. Port 0 gjev ein ledig port (brukt av testane).

### Konfigurasjon

| Nøkkel | Standard | |
|---|---|---|
| `vert`, `port` | `127.0.0.1`, 8080 | Nett-scope i deploy-malen er loopback; nginx står framfor |
| `maks_tilkoplingar` | 64 | Tilkopling nr. 65 får 503 og blir lukka |
| `maks_sse_per_kanal` | 4 | SSE-straum nr. 5 på same kanal (t.d. same brukar) får 503 |
| `header_ms`, `ledig_ms`, `kropp_ms`, `skriv_ms` | 5000, 5000, 10000, 30000 | Halv header etter fristen gjev 408 og lukking; ledig keep-alive blir lukka stilt |
| `sse_hb_ms` | 25000 | Heartbeat (`: hb`) på SSE-straumar |
| `kroppsgrense`, `kroppsgrenser` | 1 MiB, `{"/_synk/": 262144}` | 413 og lukking (også før kroppen er send) |
| `maks_header_byte` | 16384 | 431 |
| `maks_forespurnader_per_tilkopling` | 1000 | Så `Connection: close` |
| `respons_mw`, `forespurnad_mw`, `feil_mw` | `[]` | Mellomvare (sjå under) |
| `logg` | `feil` | `feil`, `alt` (éi linje per førespurnad) eller `ingen` |
| `maks_ms` | 0 | 0 = til `stopp()` |

### Lykkja

Éin tråd, éi lykkje: ta imot nye tilkoplingar (høgst 32 per runde), og for kvar tilkopling éin ikkje-blokkerande `read` (og `write` om det ligg noko i utbufferen). Når ingenting skjedde i ein runde, ventar lykkja med `poll` på lytte-socketen (vaknar straks ved ny tilkopling), med venting som doblar seg frå 1 ms til 10 ms (200 ms utan opne tilkoplingar) og aldri lenger enn til neste frist. Ein ledig socket kostar difor eitt `read` per runde (om lag 8 µs i fastmodus), ikkje ei blokkering.

`poll_many` (batch over mange socketar) finst i ABI-en, men gav `unknown handle` på macOS-seeden; lykkja brukar difor `read` per socket. Handlarane køyrer framleis éin om gongen: ein handlar som brukar 50 ms, held dei andre att i 50 ms.

Socketane er `std.socket` sine `native_*` og `network_operation`-forma deira (brukte, ikkje endra). Skrivefeil mot ein lukka peer gjev `feil`, ikkje SIGPIPE [V].

### Dispatch (som `nc serve`)

- Rutene kjem frå `ncb_route_handlers()` og blir kompilerte éin gong (metode, segment, fullt kvalifisert handlar og vakter). Rutene i std-modular (t.d. `std.synk`) er med.
- Vaktnamn med punktum blir brukte som dei er, elles `__main__.<namn>` (som `nc serve`). Ei vakt må returnere `sann` (bool); alt anna gjev 403, og eit unntak gjev 500 og ei loggline.
- `ctx` har same nøklar som under `nc serve` (`__method__` i små bokstavar, `__path__`, `__query__`, `__headers__`, `__body__`, `__params__`, `method`), men headernamna er i **små bokstavar** (som bak `std.https_front`). `web.request_header(ctx, "Cookie")` og `app.header(ctx, "X-Test")` finn dei uansett kasus. Namn over 128 byte blir avviste (431), så `lower` aldri får lange tekstar (Linux-x86-stage0 heng på `lower` over 256 byte).
- `/helse` og `/health` svarar som i `nc serve`.
- OPTIONS gjev 204 med `Allow`; feil metode gjev 405 med `Allow`; ukjend sti 404.
- Query blir ikkje URL-dekoda (som `nc serve`).
- `Transfer-Encoding` i førespurnader gjev 501 (chunked opplasting er ikkje støtta); både TE og Content-Length gjev 400.
- `Expect: 100-continue` gjev `100 Continue` før kroppen.

### Responskontrakt

Same kontrakt som `nc serve` (planen §2): toppnøklane `status`, `body`, `content_type`, `etag` (og dei andre toppnøklane `http_response.no` kjenner, t.d. `cache_control`, `location`), og alle andre headerar i `headers`. Apptenaren serialiserer sjølv (same tabell over toppnøklar som `selfhost/http_response.no`), fordi `http_response.response_to_http` tolka i VM-en kosta 3 ms per svar; eigen serialisering kostar 0,3 ms.

- Ein handlar som returnerer tekst, gjev 200 `text/plain; charset=utf-8`; tom tekst og `null` gjev 204.
- Standard Content-Type blir sett før serialiseringa, så GET og HEAD har same Content-Type og Content-Length.
- CR, LF eller NUL i eit headernamn eller ein headerverdi gjev 500 (ingen headerinjeksjon).
- `Connection`, `Keep-Alive`, `Transfer-Encoding`, `Content-Length` og `Date` eig tenaren; handlaren sine blir ignorerte.
- 304 kjem berre frå 200-svar på GET og HEAD. If-None-Match blir samanlikna svakt (`W/` blir fjerna på begge sider), lister blir delte på komma, og `*` passar når svaret har ein ETag. If-Modified-Since krev nøyaktig lik `last_modified` (som `nc serve`). 304-svaret har ETag, Cache-Control, Vary, Content-Location, Expires og Last-Modified. If-Match er ikkje støtta.
- Apptenaren legg aldri til CORS-headerar.

### Mellomvare

Response-mellomvare køyrer på alle ruter, request-mellomvare får sende `ctx` vidare, og feil-mellomvare køyrer på 403/404/405/500 frå tenaren. Unntak i mellomvare blir logga.

**Hol i `nc run` på dagens seedar:** `køyr_ncb` i `selfhost/vm.no` kopierer `route_handlers`, `dependency_providers` og `guard_providers` inn i VM-en, men ikkje lista over mellomvare, så `ncb_metadata("response_middlewares")` er tom [V]. Gje difor mellomvaren i konfig (`"respons_mw": ["__main__.merk_svar"]`) i tillegg til markøren `web.response_middleware()` (som `nc serve` treng). Når metadata-lista ikkje er tom (etter ein reseed som rettar `køyr_ncb`), blir ho brukt.

### Server-Sent Events

```
funksjon hendingar(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /hendingar")
    returner app.sse("brukar:" + uid, app.sse_hending("v", "3"))
}
# frå ein annan handlar:
app.send_hending("brukar:" + uid, "v", "4")
```

`app.sse(kanal, start)` gjer tilkoplinga om til ein straum (`text/event-stream`, `Cache-Control: no-store`, `X-Accel-Buffering: no`) og sender `start` straks. `send_hending` gjev talet på mottakarar. Heartbeat kvart `sse_hb_ms`; tidsgrensene gjeld ikkje straumar. Ein mottakar som ikkje les (over 1 MiB i utbufferen), blir lukka.

### Caps og deploy

Handlarane arvar prosessen sin capability-policy. `nc run` gjev elles alt (env, process.exec, disk, net, jit). Deploy-malen avgrensar:

```
deploy.apptenar_miljø(app_fil, datamappe, med_env)
  NORSCODE_CMD=run  NORSCODE_FILE=<app>  NORSCODE_VM_FAST=1
  NORSCODE_VM_(TARGET_)CAPABILITIES=disk.read,disk.write,net.tcp[,env.read]
  NORSCODE_VM_(TARGET_)DISK_ROOT=<datamappe>
  NORSCODE_VM_(TARGET_)NET_SCOPE=127.0.0.1,localhost
```

`generer_apptenar_systemd` legg dette i `Environment=`-linjer med `Restart=always` (ein SIGSEGV i VM-en kan ikkje fangast), `RestartSec=1`, `NoNewPrivileges`, `ProtectSystem=strict` og `ReadWritePaths=<datamappe> <mappe>/build`. `generer_apptenar_nginx` gjev upstream-keepalive (`proxy_http_version 1.1`, `Connection ""`), `client_max_body_size 256k` for `/_synk/` og ein eigen location for `/_synk/hendingar` med `proxy_buffering off`.

Under malen [V i `test_apptenar_integrasjon`]: skriving i datamappa gjev 200; skriving utanfor (`<data>/../x`), lesing av `/etc/hosts` og å starte ein prosess gjev 500 med «manglar capability» i loggen. Disk-scope godtek berre absolutte stiar.

## NSP/1 (`std/synk.no`)

```
bruk std.synk som synk
bruk std.apptenar som app

funksjon synk_brukar(ctx: ordbok_tekst) -> ordbok_tekst {
    # appen sitt sesjonsoppslag; {} = ikkje innlogga
    returner {"uid": "u1", "sesjon": "<sesjons-id>"}
}

funksjon start() -> heltall {
    synk.sett_opp({"datamappe": "/var/lib/app", "brukar_fn": "__main__.synk_brukar",
                   "samlingar": {"oppg": {"felt": {"tittel": "tekst", "ferdig": "bool", "pri": "heltall"}, "maks_byte": 4096}}})
    returner app.lytt({"port": 8080})
}
```

Konfig: `datamappe` (absolutt), `brukar_fn` (fullt kvalifisert), `samlingar`, `opphav` (liste over tillatne Origin; tom = same vert), `hemmeleg` (tom = laga og lagra i `synk.hemmeleg`), `maks_dok_per_brukar` (10 000), `maks_ops_per_push` (500), `hlc_grense_ms` (60 000), `pull_grense` (500), `sjekkpunkt_byte` (1 MiB). Andre funksjonar: `synk.status()`, `synk.sjekkpunkt()`, `synk.slett_brukar(uid)`, `synk.ny_epoke()`, `synk.csrf_token(sesjon)`, `synk.csrf_gyldig(sesjon, token)` og `synk.opphav_ok(ctx)` (same opphavssjekk som push for appen sine eigne tilstandsendrande ruter, t.d. utlogging; strengare: utan både `Origin` og `Sec-Fetch-Site`, og med `Sec-Fetch-Site: same-site`, blir han avvist).

### Endepunkt

| | Førespurnad | Svar |
|---|---|---|
| `GET /_synk/hallo` | | `{p, epoke, hlc, csrf, brukar, v, samlingar, tak}` |
| `POST /_synk/push` | `{p:1, epoke, klient, kvittert, ops:[{seq, op:"put"\|"del", s, id, hlc, felt}]}` | 200 `{p, siste_seq, hlc, v, res:[{seq, ok, v\|dup\|endra:false\|feil}]}` |
| `GET /_synk/pull?sidan=N&grense=M` | | `{p, epoke, v, meir, endringar:[{v, s, id, dok\|null, hlc, del}]}` med ETag |
| `GET /_synk/hendingar` | | SSE: `event: v` / `data: <brukaren sin høgaste v>` |

Alle svar er `application/json` med `Cache-Control: no-store` (pull i tillegg `Vary: Cookie` og ETag). Pull hadde først `private, no-cache`, men det hindrar ikkje at nettlesaren lagrar svaret i HTTP-cachen på disk (RFC 9111 §5.2.2.4), og då låg alle dokumenta til brukaren på eininga utan at han hadde valt «hugs». 304 kjem likevel, fordi klienten sjølv set `If-None-Match`. Feil har forma `{p:1, feil:"…"}`:

| Status | `feil` | Når |
|---|---|---|
| 401 | `ikkje_innlogga` | `brukar_fn` gav ikkje ein brukar (også når han kasta: fail closed) |
| 403 | `csrf` | manglande/feil token, framand Origin, `Sec-Fetch-Site: cross-site` |
| 403 | `klient_eigar` | klient-id-en høyrer til ein annan brukar |
| 409 | `hol` | første nye seq er ikkje `siste_seq + 1` (svaret har `siste_seq`) |
| 410 | `resync` | feil epoke, eller `kvittert` over det tenaren kjenner (restore) |
| 400 | `ugyldig`, `protokoll`, `klient`, `ops`, `seq`, `kvittert`, `kontrollteikn`, `parameter` | |
| 413 | `for_stor_batch` (og 413 frå apptenaren for kroppar over 256 KiB) | |
| 415 | `content_type` | ikkje `application/json` |

### Push

1. Autentisering (401), Content-Type (415), Origin og Sec-Fetch-Site, CSRF-token (403).
2. Kroppen: ingen rå kontrollteikn og ingen `\u0000`–`\u001f`-escapar utanom tab, LF og CR (`kontrollteikn_ok`), så `json()` treng berre fem erstatningar og alt tenaren sender, er gyldig for `JSON.parse`.
3. Epoke ulik → 410. Klient eigd av ein annan → 403 `klient_eigar`. `kvittert` > tenaren sin `siste_seq` for klienten → 410 (tenaren er sett attende; klienten tek ny klient-id, startar seq på 1 og sender heile køa).
4. Op for op: `seq ≤ siste` → `dup`; `seq > siste + 1` → stopp og 409 `hol` (op-ane før holet er lagra); elles valider (`valider_op`), klem HLC-en til no + 60 s (`hlc_klem_etter`: ei klemd op får alltid ein HLC etter den førre op-en i same push, så to redigeringar av same felt frå ei eining med klokka for fort ikkje får like HLC-ar og blir avgjorde av likskapsregelen i staden for rekkjefølgja), bruk op-en (`bruk_op`), sjekk kvote og storleik etter samanfletting, lagre. Ei ugyldig op blir **brukt opp** (seq går vidare, `res` har `ok:false` og feilkoden), så éi dårleg op ikkje stengjer køa.
5. Éin `fil_append` med alle journallinjene, sjekkpunkt om journalen er over `sjekkpunkt_byte`, SSE-hending `v` til kanalen `synk:<uid>`.

### HLC og samanfletting (`std/synk_kjerne.no`)

HLC-en er tekst `"<ms, 15 siffer>:<teljar, 5 siffer>:<node>"`. Med fast breidd er tekstsamanlikning det same som (ms, teljar, node). `hlc_send(lokal, no, node)` og `hlc_motta(lokal, fjern, no, node)` følgjer Kulkarni m.fl.; teljaren går over i neste millisekund etter 99 999. `hlc_klem(h, no, 60000)` set ms ned til no + 60 s; `hlc_klem_etter(h, no, 60000, førre)` gjer det same, men aukar teljaren når resultatet elles ikkje ville kome etter `førre`.

Eit dokument er `{data, hlc (per felt), del (HLC-en til siste sletting)}`. Siste skrivar vinn **per felt**: eit felt blir skrive når HLC-en til op-en er større enn feltet sin (eller `del` om feltet manglar). Ei sletting set `del` og fjernar alle felt med eldre eller lik HLC, så ei samtidig nyare redigering overlever. Tombstone = ingen felt; pull sender `dok:null` utan HLC per felt, og data er borte frå filene etter neste sjekkpunkt. Ved lik HLC (ein feil klient) vinn den største JSON-verdien, og mot ei sletting vinn slettinga, så resultatet aldri avheng av rekkjefølgja. Same op to gonger endrar ingenting.

Validering (`valider_op`): `seq ≥ 1`, `op` put/del, kjend samling, id `[A-Za-z0-9_.:-]{1,128}`, kanonisk HLC, felt ei ikkje-tom ordbok med kjende namn (ingen `__`-prefiks), type `tekst`, `heltall`, `desimaltall`, `tal` eller `bool` (null er lov), og `json(felt)` under `maks_byte`. Feilkodane er `seq`, `op`, `samling`, `id`, `hlc`, `felt`, `felt_namn:<k>`, `felt_ukjent:<k>`, `felt_type:<k>` og `for_stor`; tenaren legg til `kvote`.

Kjernen brukar berre builtins som både VM-en og WASM-backenden har, og tek tida som argument. `tests/test_synk_kjerne_wasm.no` køyrer korpusprogrammet `tests/wasm_korpus/synk_kjerne.no` i VM-en, byggjer det med `tools/nc_wasm.no` (25,7 KB, validert) og krev identisk utskrift i Chrome.

### Autentisering og CSRF

Autentiseringa ligg i handlarane, ikkje i vakter: ein app-funksjon som heiter `krev_brukar` (eller noko anna) kan ikkje skugge henne. Ein utgått sesjon gjev 401, ikkje 403.

CSRF-tokenet er `sha256(hemmeleg | sesjon | hemmeleg)`, maskert per svar med ein tilfeldig pad: `pad + (pad XOR token)` (128 hex-teikn). To svar gjev ulike token som begge blir godtekne, så komprimerte svar ikkje lekk tokenet (BREACH). Klienten sender det i `x-nc-csrf`. I tillegg: `Origin` (om han finst) må vere eit av `opphav`, eller same vert som `Host`; `Sec-Fetch-Site: cross-site` blir avvist. Full T1 (rotasjon av sesjonar, `std/csrf.no`, `std/auth.no`) er ikkje gjort.

### ETag og pull

ETag-en er `"<epoke>:<sha256(uid)[0:12]>:<brukaren sin høgaste v>:<sidan>:<grense>"`. Brukar B som sender ETag-en til A, får 200 og sine eigne data. Ny epoke gjev ny ETag. Pull er sidevis (`meir: true` og `v` = siste v på sida). Svaret har ikkje tenaren sin HLC (han endrar seg med andre brukarar og ville gjort ETag-en feil); klienten får han frå `hallo` og `push`.

### Lagring (`std/synk_lager.no`) og kvifor ikkje NorsDB

Planen sa NorsDB. Målt i VM-fastmodus på macOS (ein probe utanfor repoet: motor-API-et, sju tekstkolonnar), med rader på om lag 200 byte:

| NorsDB-motoren | 500 rader | 2 000 rader | per rad |
|---|---|---|---|
| innsetjing (WAL) | 2 528 ms | 10 312 ms | 5,1 ms |
| sjekkpunkt | 3 331 ms | 14 075 ms | 7,0 ms |
| opning | 1 206 ms | 5 068 ms | 2,5 ms |

Ved 5 000 dokument blir det om lag 35 s sjekkpunkt og 13 s opning, og kvar skriving kostar 5 ms. Årsaka er at motoren kodar rader som lister av byte i Norscode. Tenaren er éin tråd, så eit sjekkpunkt stoppar alle klientar. Synk-lageret held difor alt i minnet og skriv JSON som blir lese med den native `json_parse_raw`:

- `synk.<gen>.journal`: éi linje per endring, `<lengd>\t<json>\n`, lagt til med éin `fil_append` per push. Ei linje med feil lengd eller utan linjeskift (avbroten skriving) stoppar avspelinga, og halen blir skoren bort ved opning.
- `synk.bilete`: heile tilstanden. Sjekkpunkt: skriv `synk.bilete.tmp-<hex>` med `gen + 1`, publiser med `native_erstatt_fil_atomisk` (rename), og slett den gamle journalen. Krasj før publiseringa: det gamle biletet og journalen gjeld. Krasj etter: det nye biletet gjeld og den nye journalen er tom.
- Éin skrivar: berre apptenar-prosessen opnar mappa.
- `slett_brukar(uid)` fjernar dokument og klientar i minnet, skriv ei `x`-linje og tek eit sjekkpunkt, så dataa er borte frå filene òg.
- `ny_epoke()` (etter restore frå backup) gjev ny epoke, gløymer klient-seq og tek eit sjekkpunkt.
- Om `fil_append` gjer fsync, er ikkje undersøkt; eit straumbrot kan miste siste push(ar) sjølv om klienten fekk 200.

Atomisk sjekkpunkt i `std/norsdb_motor.no` (planen A7) er ikkje gjort, fordi synk ikkje brukar NorsDB.

## Målingar

Alle tal er målte 2026-09-30 på Apple M3 (lastsnitt 2–3) med `NORSCODE_VM_FAST=1`, macOS-seeden `dist/norscode_native` (0ce71842…) og Linux-x86-stage0 frå `bootstrap/stage0/norscode-linux-x86_64` i Docker (`nc-x86tools`, linux/amd64). Klienten er curl (eit ytre orakel, som Chrome i WASM-sporet); skripta ligg utanfor repoet (`notat/a6-scratch/maal_*.sh`).

### Apptenar mot `nc serve` (same app: `tests/fixtures/apptenar_app.no`, GET `/om`)

| | `nc serve` | apptenar |
|---|---|---|
| Ny tilkopling per førespurnad, 200 stk | p50 0,39 ms, p95 0,48 ms | p50 3,29 ms, p95 3,52 ms |
| Keep-alive, 200 på éi tilkopling | ingen keep-alive (0,25 ms per ny tilkopling) | 3,01 ms per førespurnad (332/s) |
| curl `Re-using existing connection` | 0 | 1 |
| TTFB medan ein annan socket er open utan data i 4 s | **3 695 ms** | **4,1 ms** |
| 6 parallelle GET `/` (TTFB) | 0,3–0,7 ms | 7,8 / 12,4 / 16,6 / 20,6 / 24,4 / 28,1 ms (maks < 2 × median) |
| HEAD `/` | Content-Length 7, ingen Content-Type | same Content-Type og Content-Length (60) som GET |
| `If-None-Match: W/"heim-v1"` | 200 | 304 |
| Header i to segment (0,5 s mellom) | 400 | 200 |

`nc serve` er om lag 8 gonger raskare per førespurnad: accept, parsing og serialisering er native der, medan apptenaren tolkar alt i VM-en (primitive operasjonar kostar 2–5 µs i fastmodus). Profil per førespurnad i apptenaren: tolking av hovudet 0,8 ms, dispatch (med éi response-mellomvare) 1,6 ms, serialisering 0,8 ms, resten er lykkja. Planmålet (p50 ≤ 5 ms på ei triviell rute) held. Fordelen er at ingenting blokkerer: den ledige socketen kosta 4 ms mot 3,7 s.

### Minne over tid (10 000 førespurnader, 10 rundar à 1 000 på keep-alive, fire ruter)

| | Start | Etter 1 000 | Etter 10 000 | ms per førespurnad (runde 1 → 10) |
|---|---|---|---|---|
| macOS-seed | 87,1 MB | 87,5 MB | 87,5 MB | 3,14 → 3,12 |
| Linux-x86-stage0 | 1 715,5 MB | 1 715,5 MB | 1 715,5 MB | 2,58 → 2,75 |

Ingen vekst og ingen aukande tid per førespurnad (som ein collect-storm ville gjeve). RSS-en på Linux-stage0 er heap-reservasjonen ved oppstart (verifikasjonsfelle 9), så ein lekkasje under om lag 1 GB ville ikkje synast der; tida per runde er det betre signalet. GC-prefikslekkasjen frå minnenotatet (fersk seed, 64 MiB per syklus) viste seg ikkje innanfor 10 000 førespurnader.

### Synk-tenaren med 5 000 dokument (`tests/fixtures/synk_app.no`, éin brukar)

| | |
|---|---|
| Push, 10 batchar à 500 nye dokument (om lag 81 KB kvar) | 3,5–4,0 s per batch, **7–8 ms per op**; batchen med automatisk sjekkpunkt (journal over 1 MiB) 5,1 s |
| Oppdatering av 500 eksisterande | 4,1 s |
| Pull av alle, 10 sider à 500 (første gong) | 793 ms totalt |
| Pull side 1 (118 KB) / same med If-None-Match | 48 ms / 304 på 42 ms |
| Pull utan endringar (`sidan` = høgaste v) | 6 ms |
| Sjekkpunkt med 5 000 dokument | 1 976 ms (biletet 1,29 MB) |
| Omstart til første svar (kompilering + opning av biletet) | 1 538 ms |
| Første pull etter omstart (500 endringar blir serialiserte) | 942 ms |
| RSS etter 5 000 dokument / etter omstart | 154 MB / 97 MB |

Push er den dyre vegen: i isolasjon kostar validering 0,6 ms, samanfletting 0,2 ms og lagring 1 ms per op, men i tenaren, med tusenvis av levande objekt, blir det 7–8 ms (truleg GC-arbeid som blir fordelt på kalla). Første versjon brukte 10 ms per op og 1,7 ms per endring i pull; HLC-tolking utan lykkje per siffer og JSON rekna ut éin gong per skriving (gjenbrukt i journal, bilete og pull) gav tala over. Eit sjekkpunkt stoppar tenaren i om lag 2 s ved 5 000 dokument; det skjer når journalen passerer `sjekkpunkt_byte` (1 MiB, om lag 2 500 endringar).

### NorsDB (grunnlaget for valet av lagring)

Sjå tabellen under «Lagring»: 5,1 ms per innsetjing, 7,0 ms per rad i sjekkpunkt og 2,5 ms per rad ved opning, målt med 500 og 2 000 rader. Synk-lageret: sjekkpunkt 0,4 ms per dokument (5 000 på 2,0 s) og opning av 5 000 dokument innanfor 1,5 s inkludert kompilering.

## Testar

Alle seks er grøne med `./bin/nc test` og `NC_TEST_VM_FAST=1 ./bin/nc test` på macOS og i Linux-x86-Docker. Tunge sjekkar køyrer i barneprosessar i fastmodus, så kvar test tek under 15 s også i standard-VM-en.

| Test | Kva han dekkjer | macOS std/fast | Linux std/fast |
|---|---|---|---|
| `tests/test_apptenar.no` | Same prosess: tolking av hovudet (små bokstavar, samanslåtte headerar, keep-alive-reglar, 400/414/431/501/505), `sti_match`, `etag_passar`, `http_dato`, HEAD, 304-matrisa, tekstretur, status som tekst, vakt som kastar (500) og ikkje-bool-vakt (403), headerinjeksjon (500), 404/405/OPTIONS med `Allow`, `/helse`, response-mellomvare på alle ruter | 7 s / 1 s | 3 s / 2 s |
| `tests/test_apptenar_integrasjon.no` | Barne-`nc run` under deploy-malen: keep-alive (GET, HEAD, GET på éin socket), header i to segment, ledig socket blokkerer ikkje (svar på 32 ms i testklienten), tidsgrense lukkar den ledige etter 1,5 s, halv header gjev 408, 304 (sterk, svak, liste, `*`, HEAD), vakt- og handlarunntak gjev 500 og tenaren lever, modulglobal, disk berre i datamappa og ingen prosessar, 413 over 256 KiB for `/_synk/`, `Expect: 100-continue`, SSE (heartbeat, hending, 503 for straum nr. 5), 503 for tilkopling nr. 9 (tak 8), stopp gjev exit 0 | 7 s / 6 s | 11 s / 7 s |
| `tests/test_synk_kjerne.no` | (barneprosess `tests/fixtures/synk_kjerne_sjekk.no`) HLC strengt monoton over 200 hendingar med klokke fram og tilbake, mottak større enn begge, klemme (også 90 s), treg klokke vinn etter mottak, kanonisk form; 12 tilfeldige op-sekvensar (med like HLC-ar) gjev same dokument i fire rekkjefølgjer og med duplikat, LWW per felt, lik HLC, put/del med lik HLC, tombstone utan data; 17 valideringsfeil; kontrollteikn; JSON-rundtur | 1 s / 1 s | 3 s / 3 s |
| `tests/test_synk_kjerne_wasm.no` | Korpusprogrammet `tests/wasm_korpus/synk_kjerne.no` i VM-en mot fasit, bygt til WASM (25,7 KB, validert) og køyrt i Chrome med identisk utskrift (SKIP av Chrome-delen i Docker) | 12 s / 10 s | 11 s / 9 s |
| `tests/test_synk_tenar.no` | Push og idempotens, hol → 409 og vidare, `klient_eigar` → 403, LWW frå ein annan klient, ETag per brukar (304 for eigen, 200 for B med ETag-en til A), sidevis pull, tombstone utan data (også i filene), HLC-klemme og treg klokke, SSE til rett brukar, krasj med øydelagd og avbroten journallinje, persistens over omstart, restore → 410 og resync utan tap og utan evig 409, feil og gammal epoke → 410, `slett_brukar` frå minne og filer | 13 s / 4 s | 12 s / 7 s |
| `tests/test_synk_negativ.no` | 401 (utan sesjon, utgått, kastande oppslag) på alle endepunkt trass ein open `krev_brukar`; 403 csrf (manglar, anna sesjon, endra, framand Origin, cross-site), 415, to ulike maskerte token gyldige; ingen ACAO for framand Origin; validering per op (også for stort etter samanfletting); kontrollteikn, feil form, 413 for batch og kropp; kvote | 6 s / 2 s | 6 s / 3 s |

**Kvar test kan feile** (mutasjonar i implementasjonen, éin om gongen, testen raud, mutasjonen fjerna): svak ETag ikkje avkledd (`test_apptenar`); unntak i vakt blir kasta vidare, ingen 408 ved halv header, blokkerande venting på første data frå ein ny socket, ingen `100 Continue` (`test_apptenar_integrasjon`); ingen `\u00`-kontroll, sletting med `<` i staden for `<=` (`test_synk_kjerne`, etter at to nye tilfelle vart lagde til fordi mutasjonen overlevde først); klemme på 2 × grensa (`test_synk_kjerne_wasm`, etter eit nytt 90 s-tilfelle); `dup` berre for `seq < siste`, ingen `kvittert`-kontroll, ingen journalavspeling, ingen lengdkontroll i journalen, `hent_dok` som alltid gjev null (`test_synk_tenar`); CSRF-kontroll som alltid seier ja, ingen kvote (`test_synk_negativ`). Korpustesten har dessutan den innebygde negative og positive kontrollen (side som ikkje finst, `kontroll_trap`).

Eksisterande testar er grøne etter endringane: alle 25 `tests/test_wasm*.no` (med Chrome), `test_deploy_nginx_health_contract`, `test_security_gate_contract`, `test_production_maturity` og `test_web_runtime_production_matrix`. `feature-check` er bestått på alle nye og endra `.no`-filer, active-surface-porten i ratchet-modus (fersk kopi av git-indeksen) har uendra tak, og `verify_norscode_surface_ownership` er bestått. Ingen fil i `nc_main`-lukkinga er endra.

## Klienten (W9)

Synk-klienten er `std/synk_klient.no` (Norscode, kompilert til WebAssembly), dokumentert i [WASM_KLIENT.md](WASM_KLIENT.md#synk-w9-stdsynk_klientno). Korleis han brukar protokollen:
- `hallo` før noko lokalt blir vist (berre når `fetch` ikkje får kontakt i det heile, blir kopien til den som valde «hugs» vist før, ikkje stadfesta): `brukar` (hashen) vel databasen `nc-synk-<16 hex av sha256(brukar)>` og avslører brukarbyte; `epoke`, `csrf`, `hlc` og `tak.ops` blir brukte i push.
- Utboksa (auto-nøkkel i IndexedDB, eller i minnet utan «hugs») gjev seq = nøkkel − base, så fleire faner kan skrive utan å samordne seq. `kvittert` er den høgaste seq tenaren har stadfesta.
- `push` i batchar (høgst 100 og `tak.ops`) med `x-nc-csrf`; 200 kvitterer til og med `siste_seq` (avviste op-ar blir melde med `feil` og fjerna frå køa); 409 kvitterer og sender frå `siste_seq + 1`; 410 gjev ny klient-id, seq frå 1, heile køa og full pull (dokument tenaren ikkje sende, blir fjerna etterpå); 401 stoppar og held på køa; 403 `csrf` hentar nytt token éin gong; 403 `klient_eigar` gjev ny klient-id. Køa blir aldri tømd på anna vis.
- `pull?sidan=<cursor>` side for side til `meir` er `false`, og `If-None-Match` når klienten har ETag-en for same `sidan` (304 = ingenting nytt). Ny `epoke` i pull er som 410.
- `hendingar` (SSE): hendinga `v` ≠ cursor gjev pull; eit brot gjev polling og gjenoppkopling med backoff (1 … 60 s), som blir nullstilt først når straumen har vore oppe i 30 s (tenaren sender `v` straks ved oppkopling, så ein proxy som bryt straumen etter den første hendinga ville elles gje eit forsøk i sekundet).
- HLC: klienten tek imot HLC-en i hallo, push og pull, og fylgjarfaner får HLC-en til leiarfana på kanalen; ei ny skriving av eit dokument kjem alltid etter den høgaste HLC-en i den synlege kopien.
- Kvitterte op-ar blir brukte på den lokale `snap` med ein gong, så ei stadfesta skriving blir verande synleg når pullen etterpå feilar.

Testa mot denne tenaren: 18 scenario i same prosess (`tests/test_synk_klient.no`, gjennom `app.dispatch` utan socket) og 6 til i ein eigen prosess (`tests/test_synk_klient_del2.no`: klokkeskeiv og HLC, kvittering når pullen feilar, «hugs» av med fleire faner, brukarbyte i minnemodus, hallo som feilar med nett, klemming i rekkjefølgje), der alle svara frå `/_synk/` er sjekka mot lagringsregelen til service workeren og krev `Cache-Control: no-store`, og i Chrome mot ein barne-apptenar (`tests/test_synk_klient_chrome.no`, `tests/test_synk_klient_faner.no`). Demoen `examples/wasm_synk/` er ei oppgåveliste under apptenaren med `std/synk.no`.

Funn om tenaren frå klientarbeidet:
- `/_synk/hendingar` sender `v` med ein gong og etter kvar push som endra noko; ein EventSource i Chrome får det utan eigen gjenoppkopling.
- Apptenaren sin førespurnad-mellomvare (`forespurnad_mw`) er nok til feilinjeksjon i testar (testappen fjernar den første op-en i ein push og gjev dermed 409).
- Tenaren reknar HLC-klemma ut frå si eiga klokke; klienten tek imot tenaren sin HLC i hallo, push og pull (og fylgjarfaner frå leiarfana), så ei treg klokke på eininga tapar ikkje nye skrivingar. Granskarane fann at fylgjarfaner ikkje fekk HLC-en (ei redigering etter det brukaren såg, tapte i det stille), og at klemminga kunne gje like HLC-ar innanfor ein push; begge er retta og testa i `test_synk_klient_del2`.

## Gjenstår

- **Klienten:** sjå «Gjenstår» i [WASM_KLIENT.md](WASM_KLIENT.md) (berre Chrome testa, to faner som iframes, service worker saman med synk ikkje køyrd i Chrome, ingen Background Sync, brukarbyte med usynka endringar held den førre databasen, parkerte endringar i minnemodus forsvinn om fana blir lukka, «hugs» av med fleire faner berre testa i VM-benken).
- **Klemming:** rekkjefølgja blir halden innanfor éin push; to push-ar frå ei eining med klokka meir enn 60 s for fort i same millisekund på tenaren kan framleis få like HLC-ar.
- **T1:** sesjonsrotasjon, `std/auth.no`/`std/sesjon.no`/`std/csrf.no` er ikkje endra. Synk har eige maskert CSRF-token bunde til sesjonen appen gjev i `brukar_fn`; appen må sjølv gje ein sesjon som går ut. Klienten hentar nytt token éin gong ved 403 `csrf`.
- **Mellomvare-metadata under `nc run`:** `køyr_ncb` i `selfhost/vm.no` kopierer ikkje `response_middlewares` o.l. inn i VM-en (krev reseed). Apptenaren tek dei frå konfig i mellomtida.
- **Ytelse:** push kostar 7–8 ms per op med 5 000 dokument (om lag 130 op/s), og eit sjekkpunkt stoppar tenaren om lag 2 s ved 5 000 dokument. Apptenaren er om lag 8 × tregare enn `nc serve` per førespurnad. `poll_many` gav `unknown handle` på macOS-seeden; lykkja les difor kvar socket for seg.
- **Lagring:** tombstones blir aldri rydda (dei tel i kvoten); ingen indeks per samling; `fsync` etter `fil_append` er ikkje stadfesta; berre éin prosess per datamappe. Atomisk sjekkpunkt i `std/norsdb_motor.no` (A7-planen) er ikkje gjort, fordi synk ikkje brukar NorsDB.
- **Restore:** etter ein restore (`ny_epoke`) sender klientane køa på nytt, men endringar tenaren hadde kvittert for og så mista, er borte (klienten har sletta dei frå køa); dei blir fjerna frå kopien etter den fulle pullen.
- **Deploy:** systemd- og nginx-malane er genererte og caps-miljøet er testa, men `Restart=always` og nginx-oppsettet er ikkje køyrde på ein ekte vert. TLS-fronten er framleis eit ope spørsmål (planen).
- **Ikkje testa:** fleire brukarar med mange samtidige SSE-straumar under last (`maks_sse_per_kanal` er 4 per brukar: med «hugs» opnar berre leiarfana SSE per eining, men utan «hugs» opnar kvar fane sin eigen straum, så den femte fana eller eininga til same brukar får 503; klienten tolkar det som eit brot og pollar med backoff, men det er ikkje testa), kroppar med NUL-byte (teksttransporten kan miste dei), og paritetstesten mot `nc serve` frå A6-planen (`test_apptenar_paritet.no`); forskjellane er lista i tabellen øvst.
