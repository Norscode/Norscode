# Registry-format og API

Status: **Specification (Fase 4)**

## Base-URL

```
https://registry.norscode.dev
```

Alle svar er JSON. Alle feilsvar har `"ok": false` og eit `"feil"`-felt.

---

## Endepunkt

### `GET /packages`

List alle pakkar. Støttar valfritt søkefilter.

**Query-parameter:** `?q=søkjetekst`

**Svar:**
```json
{
  "ok": true,
  "data": {
    "pakkar": [
      { "namn": "std_math", "siste_versjon": "1.2.0", "yanket": false }
    ],
    "totalt": 1
  }
}
```

---

### `GET /packages/{name}`

Hent metadata for ein pakke.

**Svar:**
```json
{
  "ok": true,
  "data": {
    "namn": "std_math",
    "siste": "1.2.0",
    "versjonar": [
      {
        "versjon": "1.0.0",
        "checksum": "sha256:abc...",
        "publisert": "2026-01-01T00:00:00Z",
        "avhengigheiter": {}
      }
    ]
  }
}
```

**Feil:**
- `404` – pakke ikkje funnen
- `410` – pakke yanket

---

### `GET /packages/{name}/{version}`

Hent artifact-metadata for ein eksakt versjon.

**Svar:**
```json
{
  "ok": true,
  "data": {
    "namn": "std_math",
    "versjon": "1.0.0",
    "checksum": "sha256:abc...",
    "kjelde_url": "https://registry.norscode.dev/packages/std_math/1.0.0.tar.gz",
    "avhengigheiter": { "core": "^1.0.0" },
    "metadata": { "entry": "math.no", "storleik": 4096 }
  }
}
```

**Feil:**
- `404` – versjon ikkje funnen
- `410` – versjon yanket

---

### `POST /packages`

Publiser ei ny pakke eller versjon.

**Krev:** `Authorization: Bearer <token>` med skop `publish`

**Body:**
```json
{
  "namn": "mitt_bibliotek",
  "versjon": "1.0.0",
  "checksum": "sha256:...",
  "kjelde_url": "...",
  "avhengigheiter": { "core": "^1.0.0" },
  "metadata": { "entry": "lib.no", "storleik": 2048 }
}
```

**Svar (201):**
```json
{ "ok": true, "data": { "namn": "mitt_bibliotek", "versjon": "1.0.0" } }
```

**Feil:**
- `400` – manglande felt
- `401` – ugyldig token
- `409` – versjon finst allereie

---

### `DELETE /packages/{name}/{version}`

Yanke ein versjon. Versjonen vert markert som trekt tilbake og er ikkje lenger tilgjengeleg for nye installasjonar.

**Krev:** `Authorization: Bearer <token>` med skop `yank`

**Svar (200):**
```json
{ "ok": true, "data": { "yanket": "mitt_bibliotek@1.0.0" } }
```

**Feil:**
- `401` – ugyldig token
- `404` – versjon ikkje funnen
- `409` – versjon allereie yanket

---

### `GET /audit-log`

Hent publiseringslogg.

**Krev:** `Authorization: Bearer <token>` med skop `admin`

**Svar:**
```json
{
  "ok": true,
  "data": {
    "log": [
      {
        "handling": "publish",
        "namn": "std_math",
        "versjon": "1.0.0",
        "brukar": "jan",
        "tidspunkt": "2026-01-01T00:00:00Z"
      }
    ]
  }
}
```

---

## Checksum-format

Alle checksummar er på formatet `sha256:<hex>`, t.d.:

```
sha256:3c4f9a1b2e7d...
```

Klienten (`nc`) verifiserer checksum etter nedlasting og avviser ved mismatch.

---

## Lockfile-format (v2)

`norcode.lock` er JSON:

```json
{
  "lock_version": 2,
  "project": {
    "namn": "min_app",
    "versjon": "0.1.0",
    "entry": "main.no"
  },
  "generert": "2026-06-12T10:00:00Z",
  "pakkar": [
    {
      "namn": "std_math",
      "versjon": "1.0.0",
      "kjelde": { "type": "registry", "url": "https://registry.norscode.dev/..." },
      "checksum": "sha256:abc...",
      "avhengigheiter": []
    }
  ]
}
```

---

## Rate limiting

- Maks 100 publish-forsøk per brukar per minutt
- Overskridne forsøk får svar `429 Too Many Requests`

---

## Kjelder

Støtta kjeldetypar i `norcode.toml`:

| Type       | Døme                                          |
|------------|-----------------------------------------------|
| `registry` | `"^1.0.0"`, `"*"`, `">=1.2.0"`               |
| `path`     | `"./packages/lokalpakke"`                     |
| `git`      | `"git+https://github.com/org/repo.git@v1.0.0"` |
| `url`      | `"https://example.com/pakke-1.0.0.tar.gz"`   |

---

## FAQ

### Kvifor får eg `409 Conflict` ved publish?

Versjonen finst allereie i registryet. Auk versjonstalet i `norcode.toml` og prøv på nytt.

### Kvifor får eg `401 Unauthorized`?

Token manglar, er ugyldig, eller har ikkje rett skop for handlinga du prøver på.

### Kvifor får eg `410 Gone`?

Pakka eller versjonen er yanket. Ho er markert som trekt tilbake og skal ikkje brukast i nye installasjonar.

### Kvifor får eg checksum-mismatch?

Den nedlasta artefakten stemmer ikkje med checksumen i lockfila eller metadata frå registryet. Stopp installasjon og undersøk kjelda.
