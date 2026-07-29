# Registry quickstart

Denne sida viser den kortaste praktiske vegen for pakker i Norscode.

## 1. Opprett ei pakke

Lag ei mappe med `norcode.toml` og ei entry-fil:

```toml
[project]
name = "mitt_bibliotek"
version = "1.0.0"
entry = "lib.no"

[dependencies]
std_math = "^1.0.0"
```

```text
mitt_bibliotek/
  norcode.toml
  lib.no
  README.md
```

## 2. Lås avhengigheiter

```sh
nc lock
```

Dette lagar eller oppdaterer `norcode.lock`.

## 3. Hent pakkar

```sh
nc fetch
```

Dette lastar ned pakkar til lokal cache frå lockfila.

## 4. Installer alt

```sh
nc install
```

Dette køyrer både låsing og henting i éin operasjon.

## 5. Publiser ei pakke

```sh
nc publish --token <token>
```

Publisering brukar manifestet i gjeldande mappe, pakkar kjeldene deterministisk og sender artefaktet til registry.

## 6. Bruk ei publisert pakke

I eit anna prosjekt:

```toml
[dependencies]
std_math = "^1.0.0"
```

Deretter:

```sh
nc install
```

## Lokal utvikling

Om du arbeider på fleire pakkar samtidig, kan du bruke ei lokal kjelde:

```toml
[dependencies]
std_math = "./packages/std_math"
```

## Vanlege feil

- Manglande token ved publish: bruk `--token <token>`.
- `409 Conflict`: versjonen finst allereie i registryet.
- `410 Gone`: pakka eller versjonen er yanket.
- checksum-mismatch: stop og sjekk kjelde, lockfil og registry-metadata.

## Vidare lesing

- [PACKAGES](PACKAGES.md)
- [REGISTRY_EXAMPLE](REGISTRY_EXAMPLE.md)
- [REGISTRY_TUTORIAL](REGISTRY_TUTORIAL.md)
- [REGISTRY_API](REGISTRY_API.md)
- [REGISTRY_PROTOCOL](REGISTRY_PROTOCOL.md)
