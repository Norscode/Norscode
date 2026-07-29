# Publisere og installere pakkar

## Publiser til registry

### Krav

- Gyldig `norcode.toml` med `name`, `version` og `entry`
- Entry-fila må finst
- Auth-token (sjå nedanfor)

### Publiser

```sh
nc publish --token <ditt_token>
```

`nc` vil:
1. Validere manifestet
2. Sjekke at versjonen ikkje finst i registry frå før
3. Pakke kjeldefilene deterministisk til ein `.tar.gz`
4. Rekne ut `sha256`-checksum
5. Sende til registry med auth-token

Eksempel på output:

```
Publisert: mitt_bibliotek @ 1.0.0
  sha256:3c4f9a1b...
```

### Versjonen finst allereie

Registry avviser publisering av ein versjon som allereie er registrert. Du må aukje versjonsnummeret i `norcode.toml`.

### Yank ein versjon

Yanking markerer ein versjon som trekt tilbake. Nye installasjonar vil ikkje bruke denne versjonen:

```sh
nc yank mitt_bibliotek 1.0.0 --token <ditt_token>
```

---

## Installer pakkar

### Frå registry (standard)

```sh
nc install
```

Løyser avhengigheitar, skriv `norcode.lock`, lastar ned og installerer.

### Berre oppdater lockfil

```sh
nc lock
```

### Berre last ned (utan re-resolving)

```sh
nc fetch
```

### Offline-modus

```sh
nc install --offline
```

Brukar berre lokalt cache. Feiler om ei pakke manglar.

---

## Auth-token

Tokenet er eit tilfeldig 32-teikns hex-streng knytt til brukarkontoen din.

For å generere eit token, bruk registry-administrasjonsverktøyet eller `nc token create` (kjem i neste versjon).

Tokenet kan ha skop:
- `publish` – løyve til å publisere pakkar
- `yank` – løyve til å yanke versjonar
- `admin` – alle løyve

---

## Sjekkliste for publisering

- [ ] `name` og `version` er riktig i `norcode.toml`
- [ ] Entry-fila kompilerer utan feil
- [ ] Alle avhengigheitar er spesifiserte
- [ ] Versjonen er ny (ikkje allereie publisert)
- [ ] `nc lock` er køyrt og `norcode.lock` er committa
