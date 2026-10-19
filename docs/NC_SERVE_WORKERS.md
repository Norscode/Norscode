# nc serve --workers N og HTTPFeil-hierarkiet (rC-serve: 5a og 4.1)

## 5a. `nc serve --workers N`

```
nc serve app.no --port 8000 --workers 4        # eller NORSCODE_WORKERS=4
```

Utan `--workers` (eller med 1) er åtferda uendra: éin prosess med hendingslykkja
(keep-alive, tidsgrenser, tak, SIGTERM-drenering; sjå kommentaren i `selfhost/nc_main.no`).
Standarden var før `4` i hjelpeteksten men vart aldri brukt; han er no `1`.

Med N > 1 (maks 64) er prosessen ein **leiar**: han bind ikkje sokkel sjølv, men startar N
barneprosessar av same binær (`std.native_gap.prosess_start_miljo`, miljøkontrakten
`NORSCODE_CMD=serve`, `NORSCODE_FILE`, `NC_SERVE_WORKER=1`, `NC_SERVE_WORKER_ID=k`). Kvar
arbeidar kompilerer appen, set `SO_REUSEPORT` før `bind` og lyttar sjølv på same port;
kjernen fordeler tilkoplingane. Alle arbeidarar er vanlege `nc serve`-prosessar, so
keep-alive, tidsgrenser og drenering gjeld per arbeidar. Tilstand i minnet er per arbeidar
(ikkje delt): bruk database/fil for delt tilstand.

Leiaren:
- overvakar barna (`wait4` WNOHANG, ~50 ms) og **startar døde arbeidarar på nytt**
  (200 ms × tal raske krasj som pause). Ein arbeidar som dør innan 3 s etter start tel som
  rask krasj; meir enn 5 på rad i same slot → leiaren gjev opp, stoppar alt og sluttar
  med exit 1 (tak mot krasjløkke, t.d. når porten er oppteken eller appen ikkje kompilerer);
- sender **SIGTERM/SIGINT** vidare til alle arbeidarar, som drenerer (`NC_SERVE_DRAIN_MS`,
  10 s); står ein att etter drenering + 3 s, får han SIGKILL. Leiaren sluttar med exit 0;
- sender SIGHUP vidare (arbeidarane loggar og held fram);
- loggar `[serve] Leiar: ...` og kvar arbeidar `Arbeidar k klar (pid N)`. Barnet startar med
  tom signalmaske (`prosess_start_miljo` set ho tilbake når leiaren har blokkert signal).

Støtteendringar: `std/native_gap.no` (`prosess_start_miljo` med miljø-overstyringar fordi
`miljo_sett` berre er eit overlay og ikkje når barnet, `prosess_signal`, `serve_reuseport`,
`serve_pid`), `std/native_sys.no` (`SO_REUSEPORT`: Linux 15, macOS 0x200).

Ikkje med (valfritt i oppdraget): per-worker metrikk-label og `/_workers`-status.
Arbeidar-id ligg i `NC_SERVE_WORKER_ID` (krev `env.read`) og pid i loggen.

**Plattformar.** Linux x86_64 verifisert (Docker linux/amd64). Linux arm64 delar kodevegen
(same `SO_REUSEPORT=15`) men er ikkje køyrt. macOS: `SO_REUSEPORT` (0x200) er lagt i
flagg-tabellen og `prosess_start_miljo` brukar fork/execve, men dette er **ikkje testa**;
på BSD/macOS fordeler kjernen tilkoplingar mindre jamt enn på Linux.

**Avgrensing.** Barnet får berre `NORSCODE_CMD/FILE/MODULE/ROOT`, `NC_NATIVE`, port, vert
og arva miljø (leiarens `miljo_sett`-overlay, t.d. `NORSCODE_VM_TARGET_*` frå direkte
invokasjon, vert rekna ut på nytt i barnet). Arbeidarane bind etter at dei har kompilert
appen; leiaren venter ikkje på det (første tilkopling kan gå til ein arbeidar som er klar).
Krev ein `nc` med denne endringa (reseed).

**Test.** `tests/test_serve_arbeidarar.no` (Linux): 3 arbeidarar, 40 samtidige tilkoplingar
fordelt på ≥ 2 arbeidarar, SIGKILL på éin → omstart med ny starttid, SIGTERM → exit 0.
Fiksturen `tests/fixtures/serve_arbeidarar_app.no`. `NC_SERVE_TEST_BIN` overstyrer kva
binær testen startar (til dømes ein lokalt bygd `nc` før reseed).

## 4.1. HTTPFeil-hierarki

```
bruk std.httpfeil                       # utan alias: namna er synlege i fang
bruk std.httpfeil_handterar som hf

kast IkkjeFunnet("Fann ikkje")          # 404  {"detail":"Fann ikkje"}
kast HTTPFeil(418, "tekanne")           # generell
kast IkkjeAutorisert("logg inn", {"WWW-Authenticate": "Bearer"})
```

Bygd på R4 (`docs/UNNTAK.md`, ingen arv): rota er **grensesnittet** `HTTPFeil`;
`HTTPFeilGenerell` (felt `kode`, `detalj`, `headers`; laga med funksjonen `HTTPFeil(kode,
detalj)` eller `HTTPFeil_med_headers`) og undertypane er eigne strukturar som implementerer
det: `UgyldigInput` 400, `IkkjeAutorisert` 401, `Forbode` 403, `IkkjeFunnet` 404,
`Konflikt` 409, `ServerFeil` 500 (felt `detalj`, `headers`; `e.status()`). Kvart kast
gjev statuskoden og JSON `{"detail": detalj}` (detalj kan vere tekst, liste eller ordbok).
`fang (e: HTTPFeil)` fangar alle, `fang (e: IkkjeFunnet)` berre éin type. Namn utanfor
`std.httpfeil` må importerast utan alias (parseren tek ikkje `fang (e: m.Type)`).

**Eigne feilhandterarar** per type, registrert éin gong ved oppstart (startup-hook under
nc serve, `start()` før `app.lytt` i apptenar):

```
hf.feilhandterar("IkkjeFunnet", fun(req, feil) -> {"status": 404, "body": "..."})
app.feilhandterar("HTTPFeil", fun(req, feil) -> {...})      # rota: alle andre HTTPFeil
```

Typen vert gitt som tekst (strukturnamn er ikkje verdiar i språket). Oppslag: eksakt
`typenamn(feil)`, deretter rota `"HTTPFeil"`, elles standardsvaret. Handlaren returnerer
ei respons-ordbok (`status`, `body`, `headers`, `content_type`); ei ordbok utan
respons-nøklar blir JSON med feilens statuskode. Kastar handlaren, vert det 500.

Eksisterande åtferd er uendra: `app.http_feil(...)`/`__httpfeil__`-ordbøker, `ValideringsFeil`
(422), vanlege unntak (500 utan lekkasje), 404/405/422 frå rutinga.

Filer: `std/httpfeil.no`, `std/httpfeil_handterar.no` (eiga modul fordi `struktur` +
toppnivå-`la` i same modul gjev «Stack underflow» i stage0-kompilatoren),
`std/apptenar.no` (`_feil_frå_unntak`, `feilhandterar`), `selfhost/nc_main.no`
(`feil_frå_unntak_ncb`: standardsvar duck-typa på `__type__`, handterarar via
`std.httpfeil_handterar.til_respons` når appen importerer modulen).
Testar: `tests/test_apptenar_httpfeil.no`, `tests/test_serve_httpfeil.no` (fiksturen
`tests/fixtures/httpfeil_app.no`; sistnemnde krev ein `nc` med endringa i nc_main).
AOT/WASM: ikkje dekt (grensesnitt-fang er VM-berre, sjå UNNTAK.md §5).
