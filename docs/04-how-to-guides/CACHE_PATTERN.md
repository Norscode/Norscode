# Cachemønster

Bruk `std.cache` når du trenger et lite in-memory kart for:

- memoisering av dyre funksjoner
- per-request midlertidige verdier
- små oppslagsdata som du vil gjenbruke i en prosess

## Anbefalt bruk

Cache verdier som tekst, tall eller bool, og gjør om til JSON når du trenger strukturerte data.

```norscode
bruk std.cache som cache

funksjon start() -> heltall {
    la c = cache.ny_minne()
    cache.sett(c, "bruker:42", "Ada")
    cache.inkrementer(c, "antall", 3)
    skriv(cache.hent(c, "bruker:42"))
    skriv(tekst_fra_heltall(cache.inkrementer(c, "antall", 0)))
    returner 0
}
```

For strukturert data er det ofte best å kombinere cache med `std.json`:

```norscode
bruk std.cache som cache
bruk std.json som json

funksjon start() -> heltall {
    la c = cache.ny_minne()
    la modell = "{\"navn\":\"Ada\",\"aktiv\":true}"
    cache.sett(c, "profil:42", modell)
    la lastet = json.parse(cache.hent(c, "profil:42"))
    skriv(json.hent_tekst(lastet, "navn"))
    returner 0
}
```
