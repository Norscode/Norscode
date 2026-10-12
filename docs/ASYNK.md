# Samtidigheit: trådar og asynk (R5 / F35 og F36)

Gjeld VM-vegen (`nc run`, `nc test`, programkøyring via `vm.køyr_ncb`). AOT (x86/arm64) er
ikkje med, sjå "AOT" nedst. Testar: `tests/test_r5_f35_trad.no`, `tests/test_r5_f36_asynk.no`
(program i `tests/fixtures/r5_f35_trad.no` og `r5_f36_asynk.no`).

## Motoren: grøne trådar (finst frå før)

VM-en køyrer brukarfunksjonar som grøne trådar (M:1, éin OS-tråd): kvar tråd er ein
suspendert continuation-tilstand (eigen rammeliste, same format som await-suspend).
Planleggaren (`vm_green_runde` i `selfhost/vm.no`) gjev kvar køyrbar tråd ein slice.
Ingen omskriving av køyremotoren var nødvendig: VM-en køyrer brukar-til-brukar-kall som
continuation frames på ei eksplisitt rammeliste, so parkering er å lagre rammelista.

R5 legg til to ting på toppen:

* **Kooperative oppgåver** (`koop = 1`): grøne trådar som aldri blir preempta på
  instruksjonsbudsjett. Dei gjev berre frå seg kontrollen ved ventepunkt (`vent`,
  `sov`/`sov_asynk`, join, kanal-/lås-venting). Hovudtråden køyrer dei først når han sjølv
  ventar. Kode mellom to ventepunkt er difor atomisk i forhold til andre oppgåver.
* **Avbrotsflagg** (`avbryt = 1`) per tråd, sjekka ved blokkerande operasjonar.

## F35: trådar

Modul `std.samtid` (`bruk std.samtid som sm`):

| Kall | Verknad |
|---|---|
| `sm.tråd(f)` / `sm.tråd_start(f)` | start tråd med lambdakropp `f` (utan parameter, t.d. `fun() -> arbeid(x)`) |
| `t.join()` / `sm.tråd_join(t)` | vent; gjev returverdien (typa, ikkje tekst); kastar tråden sitt unntak på nytt |
| `t.join_innan(ms)` | sann når ferdig innan fristen; hent verdi med `t.resultat()` |
| `t.avbryt()` / `sm.tråd_avbryt(t)` | set avbrotsflagget |
| `t.avbrote()` / `sm.avbrote()` | er tråden (den som kallar) avbroten? |
| `t.ferdig()`, `t.resultat()` | status og verdi |
| `sm.ny_lås()`, `lås_skaff`, `lås_slepp` | re-entrant lås (teljar; `lås_djupn`) |

* **Closure-fangst:** lambdaen fangar variablar ved oppretting (verdien då), ei fanga liste/
  ordbok er delt med tråden. Etter `tråd(fun() -> n * 6)` påverkar ikkje `n = 100` resultatet.
* **Typa resultat:** `join` gjev same verdi som lambdaen (heiltal, liste, ordbok, ingenting).
* **Unntak gjennom join:** unntaket (tekst, ordbok eller struktur) blir kasta att uendra, og
  stakksporet er frå TRÅDEN (`unntak_pakk(e)["stakk"]` / `e.stakk` har trådramma øvst,
  ikkje join-staden). Tråden lagrar sporet i boksen sin; `builtin.unntak_sett_stakk(e, stakk)`
  gjenoppretter det før kastet.
* **Avbrot:** `avbryt()` set flagget. Neste blokkerande operasjon i tråden kastar
  `AvbruttFeil` (under `FutureFeil`, `fang (e: AvbruttFeil)` fungerer) og nullstiller flagget
  (som `Thread.interrupted()`): `sov`, `thread_join`, `vent` på ventande future, og all venting
  på monitor-primitiva (kanal `send_vent`/`motta_vent`, lås, semafor, nedteljar, barriere,
  rw-lås). Ein søvande tråd vaknar med ein gong. Mutexen til primitivet er teken att og blir
  frigjord av std-koden før unntaket forplantar seg (ingen leke lås). Ein tråd som ikkje
  blokkerer ser flagget via `avbrote()`. `avbryt()` på ein ferdig tråd er ufarleg.
* **Re-entrant lås:** same tråd kan ta `ny_lås()` fleire gonger; andre trådar blokkerer til
  siste `lås_slepp`. Fanst frå før i `std.samtid`; testa no òg saman med avbrot.

## F36: asynk

Eit kall til ein `asynk funksjon` (programkøyring) startar ei **oppgåve** (kooperativ grøn
tråd) og gjev ei **future**: ordbok `{__future__, status, verdi, feil, __task__}`.

```
asynk funksjon hent(x: heiltall) -> heiltall { … }

la f = hent(1)          # kroppen har ikkje starta; berre ei future
la v = vent f           # parkerer kallaren, planleggaren køyrer oppgåvene
```

* **Start:** kroppen startar først når kallaren gjev frå seg kontrollen (`vent` på ei
  ventande future, `sov`/`sov_asynk`, join, eller programslutt). Ikkje-venta oppgåver blir
  køyrde ferdig før programmet sluttar (som ikkje-daemon trådar).
* **`vent`:** i hovudtråden driv `vent` planleggaren til futuren er ferdig; i ei oppgåve
  parkerer `vent` oppgåva (same suspender som await-resume), og andre oppgåver køyrer. `vent`
  på ein verdi som ikkje er ei future er identitet. `vent` på ei future som returnerte ei
  future (`returner andre()` i ein asynk funksjon) flatar ut.
* **Feil:** kasta oppgåva eit unntak, kastar `vent` det same (med stakkspor frå oppgåva).
  Ei avbroten oppgåve gjev `AvbruttFeil` (status `kansellert`). Ei feila oppgåve som ingen
  ventar på velter ikkje programmet.
* **Ventepunkt:** berre `vent`, `sov_asynk`/`builtin.vent.sov`, join og monitor-venting
  parkerer. **Blokkerande I/O (sokkel, fil, prosess) parkerer ikkje**: ho blokkerer heile VM-en.
* **Kooperativt:** hovudkoden blir ikkje avbroten av oppgåver mellom ventepunkt (ei lang
  lykkje i `start` køyrer ferdig før nokon oppgåve startar).

`std.oppgaver` (`bruk std.oppgaver som oppg`):

| Kall | Verknad |
|---|---|
| `oppg.vent_alle([f1, f2, …])` | liste med resultata i same rekkjefølgje; første feil (i listerekkjefølgje) blir kasta |
| `oppg.vent_første([…])`, `vent_første_indeks([…])` | resultat/indeks til den fyrste som blir ferdig |
| `oppg.vent_innan(f, ms)` | som `vent f`, men `TimeoutFeil` (og avbrot av oppgåva) etter `ms` |
| `vent(f, ms)` | same som `vent_innan` som kallform (innebygd i VM-en; ei eiga brukarfunksjon `vent` vinn) |
| `oppg.sov_asynk(ms)` | søv utan å blokkere andre oppgåver |
| `oppg.avbryt(f)` | avbryt oppgåva: ikkje starta → startar aldri; elles `AvbruttFeil` ved neste ventepunkt |
| `oppg.er_ferdig(f)`, `er_kansellert(f)`, `er_future(v)` | statusspørsmål |

`vent.timeout(f, ms)` og `vent.kanseller(f)` (frå før) verkar som før; `vent.kanseller` avbryt
no òg oppgåva bak futuren.

### Endra semantikk og bakoverkompatibilitet

Før R5 var `asynk` berre eit merke: kallet køyrde kroppen synkront og gav verdien direkte.
No gjev kallet ei future i **programkøyring** (`vm.køyr_ncb`: `nc run`, `nc test`). Kode som
brukar `vent f(x)` (alle eksisterande testar og std-bruk) gjev same resultat. Kode som brukte
resultatet av eit asynk-kall utan `vent` (`la x = f(1)` og så `x + 1`) må no skrive
`vent f(1)`. Innbyggings-API-et (`vm.køyr_funksjon` utan programkøyring, serve-vegen) beheld
synkron køyring av asynk-funksjonar, so handlarar og eksisterande verter er uendra. Utypa/
uendra kode gjev uendra NCB (endringa er berre i VM-en).

`TimeoutFeil`/`AvbruttFeil` frå `vent` på ei `timeout`/`kansellert` future har no formen
`«Type: melding»`, so `fang (e: TimeoutFeil)` og `fang (e: FutureFeil)` matchar (før: bar tekst
utan kolon som berre ein utypa `fang` såg).

### Avgrensingar

* Ingen ekte parallellisme (éin OS-tråd).
* Blokkerande I/O parkerer ikkje; bruk `sov_asynk`/poll-varianter.
* Deadlock (to oppgåver som ventar på kvarandre) blir ikkje oppdaga; programmet heng.
* Avbrot slår berre inn ved ventepunkt i den ytste aktiveringa (ikkje inne i ein builtin-
  callback). `mutex_lock` (monitor-lås, korte kritiske seksjonar) er ikkje avbrytbar.
* Feilmeldinga for ei feila oppgåve sin stakk kjem frå `__vm_stakk__` ved fangst; det er eitt
  globalt spor, so to trådar som kastar i same slice-runde kan i prinsippet bytte spor.

## AOT (x86/arm64)

Ikkje del av R5. AOT har ingen planleggar: `asynk funksjon` er ein vanleg synkron funksjon og
`vent` er identitet (eller `native_gap.await_value` for futures), so `std.oppgaver`-
funksjonane gjev sekvensielt same resultat for kode som ikkje avheng av samtidigheit. Det
som krev trådar feilar høgt: `sm.tråd_start` kastar `SamtidFeil` (ingen trådar), `unntak_*`-
builtins kastar `NATIV-GAP` ved køyring, og ein ventande future i AOT gjev `FutureFeil`.
`std/native_gap.no` har fått same `«Type: melding»`-form for `TimeoutFeil`/`AvbruttFeil` (krev
reseed for å slå inn i `./bin/nc`).
