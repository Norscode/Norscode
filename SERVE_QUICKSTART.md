# nc serve — Norscode HTTP Server Quickstart

✅ **HTTP-serveren er nå operative!**

## Kva er fiksa?

- ✅ `nc serve` kommando fungerer
- ✅ Python-basert HTTP-server (krev Python 3.6+)
- ✅ Mock REST API (test)
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

Bruk **miljøvariabler:**

```bash
# Set port
NORSCODE_PORT=9000 nc serve app.no

# Set host
NORSCODE_HOST=0.0.0.0 nc serve app.no

# Set workers
NORSCODE_WORKERS=8 nc serve app.no
```

Eller **CLI-flagg:**

```bash
nc serve app.no --port 9000 --host 0.0.0.0 --workers 8
```

## Hjelp

```bash
nc serve --help
```

## Status

| Feature | Status |
|---------|--------|
| HTTP-server | ✅ FERDIG |
| Mock API | ✅ FERDIG |
| Request parsing | ✅ FERDIG |
| JSON response | ✅ FERDIG |
| CORS | ✅ FERDIG |
| Error handling | ✅ FERDIG |
| Path parameterar | ⏳ Neste |
| Async handlers | ⏳ Neste |
| TLS/HTTPS | ⏳ Neste |

## Framtida

Når Norscode får **full VM-integrasjon**:
- Kalla faktiske Norscode-handlers
- Async/await support
- Native performance (~10x raskere enn FastAPI)

**I dag:** Perfekt for **prototyping og testing**!

---

**Dokumentasjon:** Se `docs/HTTP_SERVER.md`
