# App-kjensle: tenarlaget (A0) og kvikke sider (del 1)

Sporet «app-kjensle» skal få Norscode-webappar til å kjennast som lokale appar.
[Del 1](#del-1-kvikke-sider-utan-javascript-stdkvikkno) (`std/kvikk.no`) gjer
sidebyte raske utan ein einaste linje JavaScript. A0 er grunnmuren utan klientkode:

- ein felles responskontrakt som alle seinare steg byggjer på
- rask VM (fast-modus) i deploy-malane
- tryggleiksfiksar for headerar: kasus-uavhengig oppslag og ingen Origin-refleksjon
- ein regel for totale vakter
- ein nginx-mal med gzip for ressursar, ETag som overlever gzip og immutable berre for versjonerte URL-ar

Alt nytt ligg utanfor `nc_main`-lukkinga (51 modular), så A0 treng ikkje reseed.
Det som berre kan rettast i lukkinga, står under [Kjende avvik](#kjende-avvik-krev-reseed-r1).

## 1. Felles responskontrakt

Ein handlar returnerer alltid ei ordbok:

| Kvar | Nøkkel | Merknad |
|---|---|---|
| toppnøkkel | `status` | heiltal |
| toppnøkkel | `body` | tekst |
| toppnøkkel | `content_type` | blir `Content-Type` |
| toppnøkkel | `etag` | blir `ETag`. 304-vegen i `nc serve` les berre toppnøkkelen (`selfhost/nc_main.no:471-476`) |
| `headers` | alt anna, med **små bokstavar** | `cache-control`, `vary`, `x-content-type-options`, `content-security-policy`, `referrer-policy`, `x-norscode-*` |

- Same header står aldri begge stader. Det skal altså aldri stå `content-type` eller `etag` i `headers`.
- Alle headerverdiar går gjennom `http_cache.header_trygg`. Han kastar på CR, LF, NUL og andre kontrollteikn (0x00–0x1F utanom HTAB, og 0x7F). Grunnen er at `response_to_http` (`selfhost/http_response.no`) ikkje filtrerer, så ein verdi med CRLF ville injisert headerar.

Under `nc serve` gjev forma nøyaktig éin `Cache-Control`, og `If-None-Match` gjev 304. Under `web.handle_request` respekterer `response_finalize` `cache-control` i `headers`, så han legg ikkje på ekstra `no-store`.

### `std/http_cache.no`

```norscode
bruk std.web som web
bruk std.http_cache som hc

funksjon side(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /side")
    returner hc.med_tryggleik(hc.svar_revalider("<p>hei</p>", "text/html; charset=utf-8"))
}
```

| Funksjon | Gjer |
|---|---|
| `etag_av(innhald)` | Sterk ETag: sitert prefiks (128 bit, 32 hex) av `builtin.sha256` |
| `versjon_av(innhald)` / `versjonert_url(sti, innhald)` | `?v=` med 12 hex av SHA-256 (`&v=` når stien alt har `?`) |
| `nytt_svar(status, content_type, body)` | Grunnform med tom `headers` |
| `svar_revalider(body, ct)` | 200, ETag, `cache-control: no-cache`. For HTML og fragment |
| `svar_privat(body, ct)` | 200, ETag, `cache-control: private, no-cache`. For innlogga HTML |
| `svar_immutable(body, ct, v)` | `public, max-age=31536000, immutable` **berre** når `v == versjon_av(body)`, elles `no-cache` |
| `med_header(svar, namn, verdi)` | Legg til i `headers` med små bokstavar. Avviser `content-type`/`etag` og ugyldige namn eller verdiar |
| `med_vary(svar, ledd)` | Legg til eit ledd i Vary utan duplikat og utan å skrive over |
| `med_tryggleik(svar)` | `x-content-type-options: nosniff`, `content-security-policy: csp_standard()` og `referrer-policy: no-referrer`. Set berre det som manglar |
| `csp_standard()` | `default-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'; form-action 'self'` |
| `header_trygg(verdi)` / `headernamn_trygt(namn)` | Validering. Kastar ved feil |
| `kontrakt_feil(svar)` | Tom tekst når svaret følgjer kontrakten, elles det første brotet |

Modulen har ingen globalar og brukar ingen caps, så han verkar under `nc serve`.

## 2. Headerar og CORS (same commit)

**Kasus-uavhengige headerar.** HTTP-headernamn skil ikkje mellom store og små bokstavar (RFC 9110 §5.1).

- `nc serve` legg headerane i `ctx` slik klienten sende dei, til dømes `Cookie`, `X-CSRF-Token` og `If-None-Match`.
- Før fann `web.request_header` berre nøklar med små bokstavar eller den eksakte nøkkelen. Då var cookiar, CSRF-token og `auth.hent_token` tomme under `nc serve`.
- No slår `web.request_header` opp direkte først, og skannar deretter nøklane kasus-uavhengig.
- `auth.hent_token` les `cookie` og `authorization` via `web.request_header`.

**Ingen Origin-refleksjon.**

- Før speglar `response_finalize` kvar `Origin` i `access-control-allow-origin`, saman med `access-control-allow-credentials: true`. Preflight (OPTIONS i `web.handle_request`) arva det same. Ei framand side med brukaren sin cookie kunne dermed lese private svar.
- Med små bokstavar (HTTP/2-proxy) var holet ope allereie. Kasus-fiksen ville opna det for `Origin` frå ekte nettlesarar óg. Derfor kjem begge fiksane i same commit.
- No går CORS berre via `web.response_finalize_strict_cors(ctx, svar, "https://a.no, https://b.no")`, med eksplisitt allowlist:
  - Tillaten origin blir spegla med credentials.
  - Alle svar får `Origin` lagt til i Vary. Det som alt stod der, til dømes `x-norscode-fragment`, blir ikkje skrive over lenger.
- For preflight set appen `ctx["__cors_origins__"]` (kommaseparert allowlist) i ein request-middleware. Då går OPTIONS gjennom `response_finalize_strict_cors`. Utan allowlist får OPTIONS verken `access-control-allow-origin` eller `-credentials`.

Testar:

- `tests/test_web_request_header.no` feilar mot b8e7869.
- `tests/test_web_cors.no` feilar både mot b8e7869 og når berre kasus-fiksen er gjort, utan CORS-fiksen (begge prøvd).

## 3. Vakt-regelen

1. **Ei vakt er total.** All logikk ligg i `prøv`/`fang`, og kvart unntak gjev `usann` (403). Eit unntak som slepp ut av ei vakt, tek ned heile `nc serve`, fordi `nc_main.no` kallar vakta utan `prøv`. Seinare førespurnader i same prosess får då aldri svar. `web.handle_request` blir òg avbroten.
2. **Vakter i std-modular har fullt kvalifiserte namn med heile modulstien** (`std.kvikk.vakt_ingen_spekulasjon`, ikkje aliaset `kvikk.…`). Under `nc serve` heiter funksjonane i importerte modular `std.<modul>.<namn>`. Eit alias blir ikkje funne, og vakta gjev då 403 på *alle* førespurnader (målt i del 1). `web.handle_request` godtek begge formene, so berre ein `nc serve`-test fangar feilen. Namn utan punktum blir slått opp som `__main__.<namn>`, og då vinn appen sin funksjon med same namn.
3. API-autentisering høyrer heime i handlaren (401/403 med `feil`-felt), ikkje i vakta.

```norscode
funksjon rtt_vakt(ctx: ordbok_tekst) -> bool {
    web.guard()
    prøv {
        la rå = web.request_cookie(ctx, "nc_rtt")
        hvis rå == "" { returner sann }
        la ms = heltall_fra_tekst(rå)          // kastar på «abc»
        hvis tekst(ms) != rå { kast "nc_rtt er ikkje eit heiltal: " + rå }
        hvis ms > 0 og ms <= 2000 { builtin.sov(ms) }
        returner sann
    } fang (e) {
        returner usann
    }
    returner usann
}
```

`tests/test_app_kjensle_serve.no` sender `Cookie: nc_rtt=abc` mot `/vakt` og deretter `GET /` i same køyring, og krev 403 og så 200. Ein mutant av fiksturen der vakta ikkje er total, gjev exit 1 frå `nc serve` før det siste svaret, og testen feilar (prøvd).

## 4. Deploy-malar (`std/deploy.no`)

- **Fast-modus.** systemd (`Environment=NORSCODE_VM_FAST=1`, før `EnvironmentFile=`, så env-fila kan setje `0` mellombels), supervisor (`environment=…,NORSCODE_VM_FAST="1"`) og Docker (`ENV NORSCODE_VM_FAST=1`).
  - Fast-modus slår av skyggeheap-bokhaldet og VM-metrikkane. Teljaren `security_denied` blir då ikkje talt, men nekta caps kastar framleis.
  - Treng ein metrikkar, kan ein køyre med `NORSCODE_VM_FAST=0` ei stund.
- **nginx.**
  - `gzip off` i server-blokka og `gzip on` berre i `location /_nc/` og `location /static/`. `gzip_types` utan `text/html`, men med `text/css`, `text/javascript`, `application/manifest+json` m.fl.
    - nginx komprimerer `text/html` *alltid* når gzip er på, og mange `nginx.conf` har `gzip on` i http-blokka. Derfor må gzip slåast av eksplisitt for HTML.
    - Grunnen er BREACH: HTML går ukomprimert fram til T1 maskerer CSRF-token.
  - `map $http_if_none_match` strippar `W/`. gzip gjer sterke ETag-ar svake, og `nc serve` samanliknar `If-None-Match` eksakt.
  - `map $arg_v` gjev `immutable` på `/static/` berre når URL-en har `?v=`, elles `no-cache`. `expires 1y` er fjerna, fordi han gav ein ekstra `Cache-Control`.
  - `keepalive 32` er fjerna frå upstream. Han hadde ingen verknad: `nc serve` svarar på éin førespurnad per tilkopling, og keepalive mot upstream krev òg `proxy_http_version 1.1`.
  - Variabelnamna er `nc_<appnamn med [a-z0-9_]>`, fordi map-variablar er globale i http-konteksten.
  - **Malen kan ikkje køyrast lokalt (ingen nginx her). Han er testa som tekst i `tests/test_deploy_app_kjensle.no` og må stadfestast med `nginx -t` på server.**

### Sjekkliste for eksisterande einingar

Produksjonsappane brukar ikkje malen. Einingane under har `nc serve`, men ingen av dei set `NORSCODE_VM_FAST` (lese 2026-09-29, berre lesing):

- [ ] `~/norscode.no/systemd/norscode-sites-norscode-no.service` (offentleg norscode.no)
- [ ] `~/Gateway-v2/systemd/norscode-gateway-v2-prod.service`
- [ ] `~/Gateway-v2/systemd/norscode-sites-norscode-no-cms.service`
- [ ] `~/Gateway-v2/Norscode/deploy/norscode.service`
- [ ] `~/Gateway-v2/Norscode/Dockerfile` (`ENTRYPOINT nc`, `CMD serve …`)
- [ ] `~/Gateway/systemd/norscode-homepage.service`
- [ ] `~/NorscodeHelpdeskAI/deployment/systemd/norscode-helpdesk-app.service`
- [ ] `~/NorscodeHelpdeskAI/deployment/systemd/norscode-helpdesk-control.service`
- [ ] `~/NorscodeHelpdeskAI/deployment/preview/norscode-helpdesk-preview.service`

For kvar eining:

1. Legg til `Environment=NORSCODE_VM_FAST=1` (Docker: `ENV NORSCODE_VM_FAST=1`).
2. **Stadfest at verktøykjeda på serveren les flagget.** Flagget blir lese i `selfhost/vm.no` (`vm_init`, «`__vm_fast_mode__`») og kom inn rundt #181. Eldre `nc`-binærar ignorerer det stille.
   - Namnet finst ikkje som klartekst i binæren, så `grep` på binæren beviser ingenting.
   - Stadfest med gevinstmålinga under på serveren: kopier `tests/fixtures/app_kjensle_tung.no` og køyr 20 kall mot `/liste-500` med 0 og med 1.
   - Utan tydeleg skilnad (≥ 10× på median TTFB for kall 11–20) les binæren ikkje flagget.
3. Restart eininga og sjekk helse.

## 5. Active-surface-baseline

`NORSCODE_SURFACE_GATE=ratchet ./bin/nc active-surface`. I ein worktree kan ikkje VM-en lese git-indeksen (han ligg utanfor disk-sandkassa). Derfor blir indeksen kopiert til `build/` og peika på med `NORSCODE_SURFACE_GIT_INDEX`:

```sh
cp "$(git rev-parse --git-dir)/index" build/a0-surface-git-index
NORSCODE_SURFACE_GIT_INDEX=$PWD/build/a0-surface-git-index NORSCODE_SURFACE_GATE=ratchet ./bin/nc active-surface
```

Utan indeksen feilar ratchet-modus med «modus ratchet krev flata frå git-indeksen, men ho kom frå filsystem» (exit 1), òg på b8e7869.

| | b8e7869 (før A0) | etter A0 |
|---|---|---|
| exit | 0 («Gate passerte») | 0 («Gate passerte») |
| filer / symlenker frå git-indeks | 6606 / 7 | 6615 / 7 (+9: dei nye A0-filene) |
| framandspråk | 3, tak 3 (0 nye) | 3, tak 3 (0 nye) |
| shebang | 4, tak 4 (0 nye) | 4, tak 4 (0 nye) |
| binær | 5, tak 5 (0 nye) | 5, tak 5 (0 nye) |
| c_æra | 5, tak 5 (0 nye) | 5, tak 5 (0 nye) |
| symlenke / hex | 0 / 0 | 0 / 0 |
| bytekode | 1, tak 1 | 1, tak 1 |
| json | 58 filer, tak 58 | 58 filer, tak 58 |

Utdata før og etter er identiske linje for linje, bortsett frå talet på filer. Det er dei ni nye filene: `std/http_cache.no`, fiksturen, seks testar og dette dokumentet. Ingen framandspråk-, binær- eller bytekodeflate er ny.

## 6. Gevinstmåling (fast-modus)

Metode:

- `./bin/nc serve tests/fixtures/app_kjensle_tung.no --port <p> --host 127.0.0.1` med `NORSCODE_VM_FAST=0` og `=1`, i kvar sin prosess.
- 20 sekvensielle `GET /liste-500` (500 rader, 49 184 byte HTML), målt med `curl -w %{time_starttransfer}`.
- Tenaren er varma opp med eitt `GET /` før målinga.
- macOS arm64, `dist/norscode_native` (macOS-stage0), 2026-09-29. Maskina var delt med ein annan tung jobb (sjå lastsnitt).

| | VM_FAST=0 (standard) | VM_FAST=1 |
|---|---|---|
| TTFB kall 1 | 0,837 s | 0,098 s |
| TTFB kall 2 | 1,361 s | 0,096 s |
| TTFB kall 20 | 18,090 s | 0,096 s |
| median TTFB kall 11–20 | 9,173 s | 0,0963 s |
| kall20 / kall2 | 13,3 | 1,00 |
| lastsnitt (1 min) start → slutt | 1,74 → 6,43 | 1,87 → 1,87 |

Resultat mot krava:

- Median for kall 11–20 er **95× lågare** med fast-modus (krav ≥ 10×).
- Med fast-modus er kall20/kall2 = 1,00 (krav ≤ 1,5).
- I standardmodus er kall20/kall2 = 13,3 (krav ≥ 2).
- Standardmodus blir tregare for kvart kall. Veksten er jamn frå kall 1, der lasta var låg (0,84 → 1,36 → 1,77 → 2,16 s), så han skuldast ikkje berre at lasta auka mot slutten.
- Baseline frå kritikken var 7× på første kall og 13× i snitt over 5 kall. Her er det 8,6× på første kall.

Gjentaking på same kode under tung last (ein full testsuite i ein annan worktree; lastsnitt 47 ved start, 5 ved slutt av FAST=0-køyringa), 2026-09-29 07:24:

| | VM_FAST=0 | VM_FAST=1 |
|---|---|---|
| TTFB kall 1 / 2 / 20 | 2,96 / 5,93 / 71,08 s | 0,177 / 0,241 / 0,238 s |
| median TTFB kall 11–20 | 65,9 s | 0,269 s |
| kall20 / kall2 | 12,0 | 0,99 |

Lasta bles opp absolutttala, men alle tre krava held òg her: 245×, 0,99 og 12,0. Veksten i standardmodus flatar ut etter kall 10, fordi lasta fall undervegs.

## 7. Testar

| Test | Kva |
|---|---|
| `tests/test_http_cache.no` | ETag deterministisk og kjent svar (SHA-256), header_trygg (CRLF-injeksjon, LF, CR, NUL), immutable berre ved rett `v`, kontraktforma for kvar hjelpar, `kontrakt_feil` seier nei |
| `tests/test_web_request_header.no` | `Cookie`, `X-CSRF-Token`, `If-None-Match` og `Authorization` med blanda kasus, `auth.hent_token` |
| `tests/test_web_cors.no` | Ingen ACAO/ACAC for GET, 403 og OPTIONS utan allowlist. Allowlist speglar, Vary blir slått saman. Preflight med `__cors_origins__` |
| `tests/test_web_vm_spegel_kjent_avvik.no` | Pinnar det kjende avviket i VM-spegelen (sjå under). Blir raud når R1 rettar det |
| `tests/test_app_kjensle_serve.no` | Barne-`nc serve` via miljøkontrakten, med `NORSCODE_FAKE_HTTP_REQUESTS`. Byte-identitet mellom FAST=0 og FAST=1 |
| `tests/test_deploy_app_kjensle.no` | Fast-modus i tre malar og nginx-krava som tekst |

`test_app_kjensle_serve` tek om lag 4 s i VM_FAST=0 (òg med kald modulcache), så han står ikkje i `er_slow`. Han importerer `std.http_cache`. Prefiksregelen `std.http` i `tools/nc_test.no` (`_tung_modulnamn`) klassifiserer han difor alt som tung, og det same gjeld `test_http_cache`. Begge køyrer i slow-lana i CI.

Regresjon (macOS, begge modusar): alle 52 eksisterande testar som importerer `std.web`, `std.auth` eller `std.deploy`, eller les dei som kjeldetekst, er køyrde før (b8e7869) og etter A0.

- Utan nye raude.
- `test_admin_relations` er SKIP i VM_FAST=0 både før og etter.
- Dei seks nye testane er grøne i begge modusar på macOS og i Docker linux/amd64 (`bootstrap/stage0/norscode-linux-x86_64`).

Header-kasus-fiksen slår på `std.auth` og `web.request_cookie` under `nc serve` for første gong for nettlesarar som sender `Cookie` med stor forbokstav. Svakheitene i auth, som sesjonar som aldri går ut, blir retta i T1.

Testane for del 1 står i [eigen tabell](#testar-del-1).

## Del 1: kvikke sider utan JavaScript (`std/kvikk.no`)

Brukarkravet er absolutt: **ingen JavaScript i nokon form.** Ingen `.js`-filer,
ingen JS som tekst i `.no`-strengar, ingen `<script>` (heller ikkje
`type="speculationrules"`) og ingen `on*=`-attributt. Alt i del 1 er
HTTP-headerar, JSON og CSS som tenaren lagar. Nettlesarar utan støtte for ein
del lastar sidene heilt vanleg.

| Del | Korleis | Verknad |
|---|---|---|
| Speculation Rules | Headeren `Speculation-Rules: "/_nc/spekulasjon.json?v=<12 hex>"` på kvar HTML-side. Reglane er JSON (`application/speculationrules+json`) | Chromium hentar (prefetch) eller viser ferdig (prerender) lenkja brukaren held peikaren over, so klikket opnar ei ferdig side |
| View Transitions | `@view-transition { navigation: auto; }` i `/_nc/stil.css?v=<12 hex>` (immutable) | Sidebyte glir over i kvarandre (160 ms) i staden for å blinke. Av ved `prefers-reduced-motion: reduce` |
| Cache og bfcache | Sterk ETag og `no-cache` (offentleg) eller `private, no-cache` (innlogga), aldri `no-store` | Tilbake/fram kan bruke bfcache; ei uendra side kostar eit 304 utan kropp |
| 304 | `If-None-Match` med svak samanlikning: lister, `*` og `W/`-prefiks | Verkar òg bak nginx med gzip (svekte ETag-ar), der 304-vegen i `nc serve` gjev 200 |
| Sec-Purpose | `kvikk.er_spekulativ`, `kvikk.er_prerender` og to totale vakter | Spekulative hentingar kan skiljast frå ekte sidevisingar |

### Oppsett i ein app

```norscode
bruk std.web som web
bruk std.frontend som frontend
bruk std.kvikk som kvikk

// Ingen globalar under nc serve: oppsettet blir laga i ein funksjon.
funksjon fart() -> ordbok_tekst {
    la o = kvikk.med_unntak(kvikk.oppsett(), "/admin/*")
    returner kvikk.med_stil(o, app_css())     // appen sitt stilark i /_nc/stil.css
}

// Éi rute leverer reglane (offentleg og privat) og stilarket.
funksjon nc_ressurs(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /_nc/{fil:tekst}")
    returner kvikk.ressurs(ctx, fart())
}

funksjon heim(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /")
    la o = fart()
    la html = frontend.layout_app_med_head("Tavle", nav(), innhald(), "fot",
        kvikk.stil_href(o), kvikk.head(o))
    returner kvikk.side(ctx, o, html)          // eller kvikk.privat_side for innlogga sider
}
```

Heile demoen er `examples/app_kjensle.no` (Oppgåvetavle):
`./bin/nc serve examples/app_kjensle.no --port 8080 --host 127.0.0.1`.

| Funksjon | Gjer |
|---|---|
| `oppsett()` | Standard: prefetch og prerender med iver `moderate`, berre prefetch på private sider, overgangar på |
| `med_iver(o, iver)` | `conservative` (ved klikk), `moderate` (hover), `eager` eller `immediate`. Kastar på andre verdiar |
| `utan_prefetch(o)`, `utan_prerender(o)`, `med_prerender_privat(o)`, `utan_overgang(o)` | Brytarar. Utan prefetch og prerender blir det ingen `Speculation-Rules`-header |
| `med_unntak(o, mønster)` | Eit ekstra URL Pattern-mønster for stien (`/admin/*`, `/*/slett`). Må byrje med `/` |
| `med_stil(o, css)` | Legg appen sitt stilark etter kvikk-stilen i `/_nc/stil.css` (ny `?v=` når innhaldet endrar seg) |
| `side(ctx, o, html)` | 200, ETag, `no-cache`, tryggleiksheaderar, offentlege reglar, 304 ved treff |
| `privat_side(ctx, o, html)` | Som `side`, men `private, no-cache` og `/_nc/spekulasjon-privat.json` |
| `side_med_status(ctx, o, status, html)` | T.d. 404 og 422: utan ETag, `private, no-cache` |
| `sjå_anna(sti)` | 303 See Other (Post/Redirect/Get), berre lokale stiar, `no-store` (tom kropp) |
| `ressurs(ctx, o)` | Handlar for `/_nc/{fil:tekst}`: `spekulasjon.json`, `spekulasjon-privat.json`, `stil.css`, elles 404 |
| `stil_href(o)`, `head(o)` | Til `frontend.layout_app_med_head` / `page_med_head`. `head` gjev viewport-meta og `<link rel="expect" href="#app-main" blocking="render">` |
| `er_spekulativ(ctx)`, `er_prerender(ctx)` | `Sec-Purpose` (eller eldre `Purpose`) inneheld `prefetch`/`prerender` |
| `vakt_ingen_spekulasjon`, `vakt_ingen_prerender` | Totale vakter: spekulativ henting får 403, og nettlesaren hentar sida vanleg ved klikk |
| `med_vary_spekulasjon(svar)` | `Vary: Sec-Purpose` for svar som faktisk skil mellom spekulativ og vanleg henting |

`std/frontend.no` har to nye opt-in-funksjonar: `page_med_head(tittel, innhold, stylesheet_href, ekstra_head)` og `layout_app_med_head(…, stylesheet_href, ekstra_head)`. Den siste set òg `id="app-main"` på `<main>`, so skip-lenka og `rel=expect` har eit mål. `page`, `layout_app` og `layout_app_med_stylesheet` gjev byte-identisk utdata som før (golden i `test_kvikk`).

### Reglane

```json
{"prefetch":[{"source":"document","eagerness":"moderate","where":{"and":[
   {"href_matches":{"pathname":"/*"}},
   {"not":{"href_matches":[{"pathname":"/_*"},{"pathname":"/*logout*"}, … ]}},
   {"not":{"selector_matches":["[data-nc-prefetch='false']","[rel~='nofollow']"]}}]}}],
 "prerender":[ … same, pluss "[data-nc-prerender='false']" … ]}
```

- **Same origin:** mønstra er ordbøker med berre `pathname`. Protokoll, vert og port kjem då frå dokumentet, medan søk og fragment er jokerteikn, so `/logout` dekkjer òg `/logout?neste=/` (stadfesta i Chrome).
- **Standardunntak:** `/_*` (interne endepunkt), og `logout`, `log-out`, `signout`, `sign-out`, `logg-ut`, `loggut`, `logg_ut` og `utlogg` kvar som helst i stien.
- **I markupen:** `data-nc-prefetch="false"` stoppar begge, `data-nc-prerender="false"` berre prerender, og `rel="nofollow"` stoppar begge.
- **Berre GET:** Speculation Rules gjeld berre lenkjer (GET-navigasjon), aldri skjema. Ruta `/_nc/{fil}` er GET.
- **Private sider** peikar på `spekulasjon-privat.json`, som berre har prefetch. Prerender køyrer sida i bakgrunnen før klikket, og det må appen slå på sjølv (`med_prerender_privat`).
- **Versjonering:** `?v=` er dei 12 første hex-teikna av SHA-256 over JSON-en / CSS-en. Rett `v` gjev `public, max-age=31536000, immutable`, feil eller manglande `v` gjev `no-cache` (`http_cache.svar_immutable`).

### Spekulative førespurnader (Sec-Purpose)

Chromium sender `Sec-Purpose: prefetch` ved prefetch og `Sec-Purpose: prefetch;prerender` ved prerender. Tokena er kasussensitive, og `kvikk` lowercasar ikkje verdien (sjå Linux-avviket under).

- **Ikkje tel sidevisingar** for spekulative førespurnader: `hvis ikkje kvikk.er_spekulativ(ctx) { … }` (demoen skriv `sidevising <sti>` berre då). Ei førehandslasta side som blir opna, gjev ingen ny førespurnad, so tenarteljing blir anten for låg (spekulative ikkje talde) eller for høg (alle talde). Nøyaktig teljing ved aktivering krev klientkode og høyrer ikkje heime her.
- **GET med sideeffekt** skal vere unnateke i reglane *og* verna av `web.use_guard("std.kvikk.vakt_ingen_spekulasjon")` (heile modulstien, sjå vakt-regelen). Demoen gjer det for `/logg-ut`.
- `vakt_ingen_prerender` slepp prefetch gjennom. Når både prefetch- og prerender-reglar treffer same lenkje, såg tenaren i headless Chrome berre `Sec-Purpose: prefetch` (sjå under), so ei rute som ikkje tåler prerender bør bruke `vakt_ingen_spekulasjon`, `data-nc-prerender="false"` eller eit unntak.
- Svar som endrar innhald etter `Sec-Purpose`, skal ha `Vary: Sec-Purpose` (`med_vary_spekulasjon`). Svara frå `kvikk` gjer ikkje det, so dei treng det ikkje.

### Cache-standardar og bfcache (funn)

| Veg | Standard `Cache-Control` på HTML | Endra i del 1? |
|---|---|---|
| `std/web.no` `response_finalize` (`web.handle_request`) | `no-store` når handlaren ikkje set noko | **Nei.** `no-store` er trygt for private sider, og ein ny standard kunne lagre innlogga innhald i cachar. Han blokkerer bfcache, men `response_finalize` respekterer `cache-control` som handlaren set, so `kvikk.side` får `no-cache` |
| `nc serve` (`selfhost/nc_main.no`, `http_response.no`) | Ingen header | Nei (lukkinga). Bfcache-venleg, men utan ETag blir det ikkje 304 |
| `selfhost/serve_runner.no`, `selfhost/vm.no` (VM-spegelen) | `no-store` | Nei (lukkinga / ikkje `nc serve`-vegen) |
| `std/frontend.no`-layoutar | Lagar berre tekst. `fragment_page` → `islands.fragment_or_full_response` utan `cache-control`, og får dermed `no-store` via `web.handle_request` og ingenting via `nc serve` | Nei |
| `std/mw.no` `ingen_cache`, `/helse` | `no-store` med vilje | Nei |

Éin trygg og bakoverkompatibel fiks i `response_finalize`: han la til `content-type: text/plain` i `headers` sjølv når svaret hadde toppnøkkelen `content_type` (responskontrakten), so HTML frå kontrakten vart sjølvmotseiande og `hc.kontrakt_feil` sa nei. No blir toppnøkkelen respektert. Svar utan toppnøkkelen blir handterte som før.

### CSP

`http_cache.csp_standard()` (`script-src 'self'`) blokkerer ikkje header-varianten av Speculation Rules. I Chrome 154 vart reglane brukte òg med `script-src 'none'` (målt). Stilarket er same origin og blir dekt av `default-src 'self'`. Demoen har ingen `style=`-attributt, som `default-src 'self'` ville blokkert.

### Nettlesarstøtte

| | Chromium (Chrome, Edge, Opera, Samsung) | Safari | Firefox |
|---|---|---|---|
| `Speculation-Rules`-header, document rules, eagerness | Ja (sidan 121); **stadfesta i Chrome 154** | Nei | Nei |
| Prefetch / prerender | Ja / ja; `Sec-Purpose` stadfesta | Nei | Nei |
| Kryss-dokument View Transitions (`@view-transition`) | Ja (sidan 126) | Ja (sidan 18.2) | Nei (berre same-dokument) |
| bfcache | Ja | Ja | Ja |
| ETag / 304 | Ja | Ja | Ja |

Versjonane er etter beste kunnskap per 2026-09 og bør stadfestast mot caniuse før dei blir lova nokon; berre Chrome 154 er målt her. Utan støtte blir headeren og `@view-transition` ignorerte, og sidene lastar som vanleg. Safari og Firefox får dermed cache/bfcache og (Safari) overgangar, men ingen førehandslasting.

### Stadfesta i headless Chrome 154 (utan JS i repoet)

Metode: `nc serve` på ein prøveapp i `build/` (ikkje committa) med `kvikk` og iver `immediate` (headless kan ikkje halde peikaren over ei lenkje), og `Google Chrome --headless=new --dump-dom --virtual-time-budget=… --enable-logging=stderr --log-net-log=…`. Appen skriv `Sec-Purpose` for kvar førespurnad til stdout, og netloggen viser kva Chrome henta.

- **Reglane blir aksepterte:** Chrome henta `/_nc/spekulasjon.json?v=…` og `/_nc/stil.css?v=…` og prefetcha `/ok1` og `/ok2?x=1` med `Sec-Purpose: prefetch`.
- **Unntaka verkar:** ingen henting av `/logout`, `/logout?neste=/`, `/konto/logg-ut`, `/_intern`, `/admin/x` (eige unntak), `data-nc-prefetch="false"`, `rel="nofollow"` eller ei ekstern lenkje.
- **Prerender:** med berre prerender-reglar kom `/ok1` og `/ok2` med `Sec-Purpose: prefetch;prerender`, og lenkja med `data-nc-prerender="false"` vart ikkje henta. Med begge regelsetta såg tenaren berre prefetch. Om prerenderen då gjenbrukte prefetch-svaret eller vart hoppa over i headless, kan ikkje avgjerast utan DevTools-protokollen.
- **CSP:** reglane vart brukte både med `csp_standard()` og med `script-src 'none'`.
- **Negativ kontroll:** ugyldig regel-JSON gav konsollmeldinga «While parsing speculation rules … Syntax error» i stderr. Demoen (`/oppgaver`) gav inga slik melding og ingen CSP-brot.
- **Ikkje stadfesta:** iver `moderate` (hover), sjølve overgangsanimasjonen og at sidene faktisk kjem frå bfcache. Det krev ein ekte nettlesar med peikar eller DevTools-protokollen, og ingen JS-baserte måleverktøy skal inn i repoet.

### Testar (del 1)

| Test | Kva |
|---|---|
| `tests/test_kvikk.no` | Gyldig JSON og innhald i reglane (offentleg/privat, iver, unntak, veljarar, JSON-escaping som rundtur), headerar på HTML/reglar/stilark, 304 for sterk, `W/`, liste og `*`, Sec-Purpose og vakter, ingen `<script`/`on*=`, golden for `frontend.page`, og kontrakten gjennom `web.handle_request` |
| `tests/test_kvikk_serve.no` | Oppgåvetavla under barne-`nc serve` (16 førespurnader): headerar på wire, 304, 50 rader, 404, 303/422, Sec-Purpose utan sidevising, 403 frå vakta, ingen klientkode, byte-identitet mellom VM_FAST=0 og 1 |
| `tests/test_web_request_header.no` | Nytt tilfelle: headernamn på 300 byte (Linux-avviket under) |

Kvar test er vist å kunne feile: `test_kvikk` mot ti mutantar av `std/kvikk.no` (m.a. `no-store`, utan `nofollow`, feil iver, prerender alltid på private sider, utan `W/`, utan `/_*`, JSON-escaping, `<script>` i `head`) og mot `std/web.no` utan content_type-fiksen; `test_kvikk_serve` mot åtte mutantar (m.a. sidevising for prefetch, vakt med alias, 200 i staden for 303, `onsubmit`, 49 rader, feil Content-Type på reglane); `test_web_request_header` gjev signal 139 på Linux mot førre `std/web.no`.

Alle tre er grøne i `./bin/nc test` og `NC_TEST_VM_FAST=1 ./bin/nc test` på macOS og i Docker linux/amd64 (`bootstrap/stage0/norscode-linux-x86_64`).

Regresjon (macOS, begge modusar): dei 49 eksisterande testane som importerer eller les `std.web`, `std.frontend` eller `std.http_cache`, er køyrde før (349abd5, eigen worktree) og etter del 1. Resultata er dei same (98 av 98 OK), og i tillegg er dei nye testane grøne. I Docker linux/amd64 er `test_web_request_header`, `test_frontend`, `test_frontend_panel_helpers`, `test_http_cache`, `test_web_cors` og `test_app_kjensle_serve` grøne i begge modusar.

Flate og lukking: alt i del 1 ligg utanfor `nc_main`-lukkinga (framleis 51 modular; `std/web.no`, `std/frontend.no` og `std/kvikk.no` er ikkje med), so det trengst ingen reseed. `./bin/nc feature-check` er grøn for alle nye og endra `.no`-filer. Active-surface i ratchet-modus (git-indeks-kopi som i A0) passerer: 6619 filer (+4 `.no`), og framandspråk, shebang, binær, c_æra, bytekode og json står uendra på taka sine.

### Del 2: offline og lokal lagring (ventar)

Offline-bruk, lokal lagring (IndexedDB) og synk krev kode som køyrer i nettlesaren. Utan JavaScript er einaste vegen kompilert Norscode → WASM (eller generert JS som kompilatorutdata, ikkje handskriven), og den backenden finst ikkje enno: `std/wasm_binary.no` lowrar berre heiltalsaritmetikk utan kontrollflyt, kall, strengar eller DOM-importar. Del 2 ventar på at brukaren vel veg.

## Kjende avvik (krev reseed, R1)

Desse ligg i `nc_main`-lukkinga eller i `bootstrap/` og kan ikkje rettast utan reseed:

1. **VM-spegelen av web-laget** (`selfhost/vm.no`):
   - `vm_web_finalize_response` speglar framleis `Origin` med credentials.
   - `builtin.web.request_header` skil mellom store og små bokstavar.
   - Spegelen blir berre nådd når eit program kallar `builtin.web.*` **utan** å importere `std.web`. Med `std.web` blir namnet løyst til `std/web.no` (målt, og testa i `test_web_cors`).
   - Utan `std.web` finst det ingen ruter, så svaret er 404. Den praktiske risikoen er difor låg.
   - Pinna i `tests/test_web_vm_spegel_kjent_avvik.no`.
2. **Unntak i vakt tek ned `nc serve`** (`nc_main.no` kallar vakta utan `prøv`). Fram til R1 gjeld vakt-regelen over.
3. **304 i `nc serve` krev eksakt `If-None-Match` eller `if-none-match`** (`nc_main.no:471`). Andre kasusvariantar gjev 200. Nettlesarar og HTTP/2 sender ein av desse to.
4. **`response_to_http` filtrerer ikkje CR/LF i headerverdiar** (`selfhost/http_response.no`). Vernet ligg i `http_cache.header_trygg`, så svar som ikkje går via `http_cache`, er ikkje verna.
5. **`tekst_til_liten` heng på tekstar over 256 byte på Linux-x86-stage0** (`bootstrap/stage0/norscode-linux-x86_64`, funne i del 1). 256 byte går, 257 heng under `nc run` og gjev signal 139 under `nc test`. macOS-arm64-stage0 er ikkje ramma. Med eittrådig `nc serve` er klientstyrt tekst gjennom `tekst_til_liten` ein DoS-veg: `web.request_header` (A0-skanninga) lowercasa kvart headernamn, og er retta (lowercasar berre namn med rett lengd, test i `test_web_request_header`). `kvikk` lowercasar ikkje `Sec-Purpose`. Anna std-kode som lowercasar klientdata, er ikkje gjennomgått.
6. **Statusteksten** for 303 og 422 er «OK» (`status_tekst` i `http_response.no`). Nettlesarar bryr seg ikkje om han.
7. **`nc serve` tek éin førespurnad om gongen, éin per tilkopling.** Prefetch kjem i same kø som vanlege klikk, og ein open preconnect-sokkel frå nettlesaren kan halde køa (head-of-line). Iver `moderate` gjev éin eller to spekulative førespurnader om gongen. `eager`/`immediate` på lange lister er ikkje tilrådd før serverlykkja er retta.
8. **304 frå `nc serve` sin eigen sjekk har berre ETag**, utan Cache-Control og Speculation-Rules. `kvikk` sitt eige 304 (for `W/`, lister og `*`) har dei med.

## Funn i andre repo (berre lese)

Grep etter `access-control-allow-origin`, `access-control-allow-credentials`, `response_finalize`, `handle_request`, `strict_cors` og `origin` i `~/norscode.no`, `~/NorscodeHelpdeskAI`, `~/Gateway-v2` og `~/Gateway`:

- Ingen app er avhengig av Origin-refleksjonen. Treffa på `response_finalize` i `~/norscode.no` er dokumentasjonstekst i seed-filer.
- `~/NorscodeHelpdeskAI/deployment/production.no` krev sjølv `cors = same_origin_no_reflection`.
- `~/norscode.no`, `~/Gateway-v2` og `~/Gateway` har eigen kasus-uavhengig `gateway_request_header` og merkar ikkje endringa.
- `~/NorscodeHelpdeskAI/src/identity.no` brukar `web.request_header(ctx, "origin")`, `"host"` og `"sec-fetch-site"` i CSRF-sjekken for opphav.
  - Under `nc serve` fann han før ikkje `Origin`/`Sec-Fetch-Site` med stor forbokstav, og sjekken slapp då alt gjennom.
  - Etter A0 blir `Origin` samanlikna med `Host`.
  - **Må stadfestast før synk:** bak ein proxy som skriv om `Host` (til dømes til `127.0.0.1:<port>`) kan legitime POST-ar bli avviste. `Sec-Fetch-Site: same-origin` frå moderne nettlesarar slepp dei likevel gjennom før Origin-sjekken.
