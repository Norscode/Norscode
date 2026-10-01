# nc serve — Norscode HTTP Server Quickstart

✅ **HTTP-serveren køyrer.**

> **Merk (fase 0, 2026-10).** Tenaren er **rein Norscode** med native socket — det er
> **ingen** Python-avhengnad (eldre påstand om «Python 3.6+» stemmer ikkje). Miljøvariablane
> `NORSCODE_PORT`/`NORSCODE_HOST`/`NORSCODE_WORKERS` blir **ikkje** lesne; bruk flagga
> `--host`/`--port` (`--workers` blir ignorert). Døme med `std.httpserver` blir aldri servert
> (stubb). Sjå `notat/fastapi_analyse.md` for full, etterprøvd status.

## Kva er fiksa?

- ✅ `nc serve` kommando fungerer
- ✅ HTTP-server i rein Norscode (ingen Python)
- ✅ Request-logging
- ✅ CORS-support

## Kommando

```bash
nc serve [fil.no] [--port 8000] [--workers 4] [--host 127.0.0.1]
```

## Kjør Eksempel-API

```bash
nc serve examples/api_server.no --port 8000
```

**Output:**
```
═══════════════════════════════════════════════════════════════
Norscode HTTP Server v1.0
═══════════════════════════════════════════════════════════════
  App:        examples/api_server.no
  Host:       127.0.0.1
  Port:       8000
  Workers:    4
  URL:        http://127.0.0.1:8000

Endpoints:
  GET  /               - Velkomen
  GET  /api/helse      - Health check
  GET  /api/*          - API endpoints

Press Ctrl+C to stop
─────────────────────────────────────────────────────────────
```

## Test i annan terminal

```bash
# Få velkomen-melding
curl http://localhost:8000/

# Health check
curl http://localhost:8000/api/helse

# JSON-respons
curl http://localhost:8000/api/data

# Med query-parameter
curl "http://localhost:8000/api/search?q=test"
```

## Ditt eige Norscode-app

Opprett `app.no`:

```norscode
bruk std.httpserver som http

funksjon start() -> heltall {
    la server = http.ny_server(8000)
    http.route(server, "GET", "/", hello)
    returner http.lytt(server)
}

funksjon hello(ctx: ordbok_tekst) -> tekst {
    returner "{\"melding\": \"Hei frå Norscode!\"}"
}
```

Kjør:

```bash
nc serve app.no --port 3000
```

Test:

```bash
curl http://localhost:3000/
```

## Konfigurering

Bruk **CLI-flagg** (miljøvariablane `NORSCODE_PORT`/`NORSCODE_HOST`/`NORSCODE_WORKERS`
blir **ikkje** lesne):

```bash
nc serve app.no --host 0.0.0.0 --port 9000
```

`--workers` blir i dag ignorert (éin prosess, éin tråd).

## Hjelp

```bash
nc serve --help
```

## Status

| Feature | Status |
|---------|--------|
| HTTP-server | ✅ Fungerer (`nc serve`, rein Norscode) |
| Request parsing | ✅ Fungerer |
| CORS | ⚠️ Delvis (preflight manglar ACAO; sjå analysen) |
| Error handling | ⚠️ Delvis (500/404 er `text/plain`, ikkje JSON) |
| Path parameterar | ⚠️ Delvis (`{id:int}` verkar; utan type 404, ingen 422) |
| Async handlers | ⚠️ Delvis (kompilerer, men køyrer synkront) |
| TLS/HTTPS | ❌ Ikkje i `nc serve` |

## Framtida

Når Norscode får **full VM-integrasjon**:
- Kalla faktiske Norscode-handlers
- Async/await support
- Native performance (~10x raskere enn FastAPI)

**I dag:** Perfekt for **prototyping og testing**!

---

**Dokumentasjon:** Se `docs/HTTP_SERVER.md`
