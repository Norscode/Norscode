# WebSocket i apptenaren (`std.apptenar_ws`)

WebSocket-tenar (RFC 6455) for appar som køyrer under `nc run` med `std.apptenar`.
Protokollmotoren ligg i `std/apptenar_ws.no`, og tenarløkka i `std/apptenar.no`
handterer oppgraderinga og socketen. Alt er skrive i Norscode.

## Rute og handlar

Ei WebSocket-rute er ei vanleg rute med metoden `WS`. Vakter (`web.use_guard`),
mellomvare, TrustedHost, rate-grense og avhengnader køyrer på same måte som for andre
ruter, og dei køyrer **før** oppgraderinga. Ei vakt som seier nei, gjev 401 eller 403, og
handlaren blir aldri kalla.

```norscode
bruk std.web som web
bruk std.apptenar som app
bruk std.apptenar_ws som ws

funksjon innlogga(ctx: ordbok) -> bool {
    web.guard()
    returner app.cookie(ctx, "sesjon") != ""
}

funksjon chat(t: ordbok) -> heltall {
    web.use_guard("innlogga")
    web.route("WS /chat/{rom}")
    la rom = ws.param(t, "rom")
    la h = ws.hending(t)
    hvis h == "open" { ws.bli_med(t, rom) }
    hvis h == "melding" { ws.rom_send(rom, ws.motta_tekst(t)) }
    hvis h == "lukk" { skriv("lukka med " + tekst(ws.lukkekode(t)) + "\n") }
    returner 0
}

funksjon start() -> heltall {
    returner app.lytt({"port": 8080})
}
```

Apptenaren køyrer i éin tråd, så handlarane er hendingsbaserte og blokkerer aldri.
Handlaren blir kalla:

| Hending | Når | I handlaren |
|---|---|---|
| `"open"` | rett etter 101-svaret | `ws.send_tekst`, `ws.bli_med` osb. |
| `"melding"` | éin gong per heile melding (fragment er sette saman) | `ws.motta(t)` gjev `{"type": "tekst"\|"binær", "data": tekst}` |
| `"lukk"` | éin gong når tilkoplinga er over | `ws.lukkekode(t)`, `ws.lukkegrunn(t)` (1006 når TCP vart broten) |

Rute-avhengnader (`web.use_dependency`) blir sende med som ekstra argument etter `t`,
som for vanlege handlarar. Eit unntak i handlaren lukkar tilkoplinga med 1011.

## API

| Funksjon | Forklaring |
|---|---|
| `hending(t)` | `"open"`, `"melding"` eller `"lukk"` |
| `motta(t)` / `motta_tekst(t)` | meldinga som utløyste hendinga (null eller `""` elles) |
| `send_tekst(t, s)` | tekstmelding; gjev 0 når tilkoplinga er på veg ned |
| `send_bytes(t, data)` | binærmelding; tek imot byte-tekst, `bytes` eller liste av heiltal |
| `ping(t, data)` | ping (tenaren sender òg heartbeat-ping sjølv) |
| `lukk(t, kode, grunn)` | startar close-handtrykket (1000, 1001, 1003, 1007–1014, 3000–4999) |
| `er_open(t)`, `id(t)`, `ctx(t)`, `param(t, namn)`, `header(t, namn)` | tilstand og førespurnaden frå handtrykket |
| `bli_med(t, rom)`, `forlat(t, rom)` | rom i same prosess |
| `rom_send(rom, s)`, `rom_send_bytes(rom, data)`, `rom_send_unntatt(rom, s, t)` | kringkasting; gjev talet på mottakarar |
| `tal_i_rom(rom)`, `tal_opne()` | teljarar |
| `aksept(nøkkel)`, `gyldig_utf8(s)` | hjelparar (Sec-WebSocket-Accept, streng UTF-8) |

Binære data er tekst der kvart teikn er éin byte (`builtin.chr`/`builtin.char_code`).
Tekst i Norscode er byte-trygg, òg for NUL.

## Protokoll

- **Handtrykk:** GET over HTTP/1.1 med `Upgrade: websocket`, `Connection: Upgrade`,
  ein gyldig `Sec-WebSocket-Key` og `Sec-WebSocket-Version: 13` gjev 101 med
  `Sec-WebSocket-Accept`. Utan Upgrade, eller med ein annan versjon, blir svaret 426 (med
  `Sec-WebSocket-Version: 13`). Ein ugyldig nøkkel, manglande `Connection: Upgrade` eller
  HTTP/1.0 gjev 400. Andre metodar gjev 405, og ein Origin utanfor `ws_origins` gjev 403.
- **Rammer:** klientrammer må vere maskerte. Ei umaskert ramme, RSV-bit, ukjende
  opkodar, fragmenterte eller for lange (> 125 byte) kontrollrammer og feil
  fragmentrekkjefølgje gjev alle 1002.
- **Ping og close:** ping får pong automatisk. Close blir svara med same statuskode, og
  ein ugyldig lukkekode gjev 1002. Etter close blir TCP lukka.
- **Grenser:** ei ramme over `ws_maks_ramme` eller ei melding over `ws_maks_melding`
  gjev 1009. Tenaren avviser rammer på hovudet åleine, før nyttelasta er komen.
- **UTF-8:** tekstmeldingar og lukkegrunnar blir validerte strengt (ingen overlange
  former, surrogatar eller kodepunkt over U+10FFFF). Ugyldig UTF-8 gjev 1007.

## Konfigurasjon (`app.lytt`)

| Nøkkel | Standard | |
|---|---|---|
| `ws_maks_ramme` | 1048576 | største ramme (byte) |
| `ws_maks_melding` | 4194304 | største samansette melding |
| `ws_ping_ms` | 30000 | heartbeat-ping når tilkoplinga er open |
| `ws_lukk_ms` | 5000 | ventetid på klienten sin close etter `ws.lukk` |
| `ws_origins` | `[]` | tillatne Origin (tom liste = alle) |
| `ws_maks_utbuffer` | 4194304 | ein treg mottakar med meir i kø enn dette blir kopla frå |

`app.status()["ws"]` gjev talet på opne WebSocket-tilkoplingar.

## Testing utan nettverk

`std.apptest` (app-testklienten) opnar tilkoplingar i same prosess: handtrykket går gjennom same
ruting, vakter og validering som på ein socket, og rammene går gjennom protokollmotoren.

```norscode
bruk std.apptest som at

funksjon start() -> heltall {
    la kl = at.klient({"logg": "ingen"})
    at.sett_header(kl, "Cookie", "sesjon=abc")
    la k = at.ws_kople(kl, "/chat/lobby", {})
    assert_eq(k["status"], 101)
    at.ws_send_tekst(k, "hei")
    assert_eq(at.ws_motta_tekst(k), "hei")
    at.ws_lukk(k, 1000, "")
    la r = at.ws_motta(k)          # {"type": "lukk", "kode": 1000, ...}
    returner 0
}
```

Andre klientfunksjonar er `ws_send_bytes`, `ws_ping`, `ws_send_ramme(k, fin, op, data)`
(for fragment), `ws_ramme` og `ws_send_rå` (for rå og ugyldige rammer) og
`ws_er_lukka`.

Testar: `tests/test_apptenar_websocket.no` (in-process) og
`tests/test_apptenar_websocket_socket.no` (ekte socket mot ein barne-`nc run`).

## Avgrensingar

- Ruta blir deklarert med `web.route("WS /sti")`. Kompilatoren kjenner berre att
  `web.route`-markøren, så ein eigen `web.websocket("/sti")`-markør krev ein ny seed.
- Ingen utvidingar (permessage-deflate) og ingen subprotokollar
  (`Sec-WebSocket-Protocol`) blir forhandla.
- Yield-avhengnader blir rydda etter handtrykket, ikkje når tilkoplinga blir lukka.
- Avmaskering og UTF-8-validering går byte for byte i VM-en. Det held for chat og
  hendingar, men store binærmeldingar (fleire hundre KiB) er trege.
- Rom gjeld berre i same prosess.
