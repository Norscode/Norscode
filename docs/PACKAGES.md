# Opprette og bruke pakkar i Norscode

## Kva er ei Norscode-pakke?

Ei pakke er ei mappe med:
- `norcode.toml` – prosjektmanifest
- éi eller fleire `.no`-kjeldefiler
- eventuelt `norcode.lock` – låst avhengigheitsgraf

## Status

Pakkeflyten er delvis implementert i repoet, med manifest, resolver, lockfile, cache, publisering og registry-API dokumentert som eigne modular og kontraktar.

For protokoll og API, sjå [REGISTRY_API](REGISTRY_API.md) og [REGISTRY_PROTOCOL](REGISTRY_PROTOCOL.md).
For ein kort praktisk flyt, sjå [REGISTRY_QUICKSTART](REGISTRY_QUICKSTART.md).

## Opprett ei pakke

Lag ei ny mappe og opprett `norcode.toml`:

```toml
[project]
name    = "mitt_bibliotek"
version = "1.0.0"
entry   = "lib.no"

[dependencies]
std_math = "^1.0.0"
```

## Kom i gang

1. Opprett `norcode.toml` med `[project]` og eventuelle `[dependencies]`.
2. Kjør `nc lock` for å generere `norcode.lock`.
3. Kjør `nc fetch` for å hente pakkar til lokal cache.
4. Kjør `nc install` for å gjere begge delar i éin operasjon.

For publisering:

1. Sørg for at manifestet er gyldig.
2. Kjør `nc publish --token <token>`.
3. Sjekk at registryet returnerer riktig metadata med `nc info <pakke>`.

## Eksempel: `std_math`

Her er eit lite, realistisk eksempel på korleis ei pakke kan sjå ut i Norscode:

```toml
[project]
name = "std_math"
version = "1.0.0"
entry = "math.no"

[dependencies]
```

Filstruktur:

```text
std_math/
  norcode.toml
  math.no
  README.md
```

Typisk flyt:

```sh
nc lock
nc fetch
nc publish --token <token>
```

Når pakka er publisert, kan andre prosjekt bruke henne slik:

```toml
[dependencies]
std_math = "^1.0.0"
```

Og så:

```sh
nc install
```

## Eksempel: lokal utvikling med `path`

Når du jobbar på to pakkar samtidig, kan du peike på ei lokal mappe:

```toml
[project]
name = "min_app"
version = "0.1.0"
entry = "main.no"

[dependencies]
std_math = "./packages/std_math"
```

Dette er nyttig når du vil teste endringar i ei lokal pakke før du publiserer henne til registry.

Feltforklaring:

| Felt        | Påkravd | Beskriving                        |
|-------------|---------|-----------------------------------|
| `name`      | Ja      | Pakkenamn (bokstavar, tal, `-`, `_`) |
| `version`   | Ja      | Semantisk versjon (`major.minor.patch`) |
| `entry`     | Ja      | Hovudfil i pakka                  |

## Avhengigheitsspesifikatorar

| Format               | Meining                              |
|----------------------|--------------------------------------|
| `"1.0.0"`            | Eksakt versjon                       |
| `"^1.0.0"`           | Same major, `>= minor.patch`         |
| `"*"`                | Kva som helst versjon                |
| `">=1.2.0"`          | Minimum versjon                      |
| `"<=2.0.0"`          | Maksimum versjon                     |
| `"./lokal/sti"`      | Lokal path-kjelde                    |
| `"git+https://...@v1.0.0"` | Git-kjelde med ref             |
| `"https://...tar.gz"`| URL-kjelde                           |

## Installer avhengigheitar

```sh
nc install
```

Dette køyrer resolver, skriv `norcode.lock` og lastar ned alle pakkar.

Viss du alt har ein gyldig lockfil, kan du også køyre `nc fetch` åleine for å berre hente frå lockfila utan å generere ny.

## Legg til og fjern avhengigheitar

```sh
nc add std_tekst "^1.0.0"
nc remove std_tekst
```

Hugs å køyre `nc lock` etterpå for å oppdatere lockfila.

## Oppdater pakkar

```sh
nc update            # oppdater alle
nc update std_math   # oppdater éin
```

## Publiser pakke

```sh
nc publish --token <token>
```

Publisering brukar `norcode.toml` i gjeldande mappe, pakkar kjeldefiler deterministisk og sender artefaktet til registry.

### Før du publiserer

- Sjekk at `norcode.toml` er gyldig.
- Sørg for at `version` er oppdatert.
- Kjør `nc lock` og kontroller at lockfila er oppdatert.
- Ver sikker på at entry-fila finst.
- Pass på at token har publish-tilgang.
- Ver merksam på at same versjon ikkje kan publiserast to gonger.

## Pakkestruktur (anbefalt)

```
mitt_bibliotek/
  norcode.toml
  norcode.lock
  lib.no
  utils.no
  README.md
```

## Versjonering

Norscode følgjer semantisk versjonering:

- **Major**: brotande endringar
- **Minor**: bakoverkompatible tillegg
- **Patch**: feilretting

Når du publiserer ei ny oppdatering, auk rett felt i `norcode.toml` og publiser på nytt.

## FAQ

### Kvifor feiler `nc publish` med manglande token?

Du må sende inn `--token <token>` for operasjonar som publiserer eller yankar.

### Kvifor feiler `nc install`?

Vanlege årsaker er:
- ugyldig `norcode.toml`
- manglande `norcode.lock`
- checksum-mismatch
- ei pakke som ikkje finst i registry

### Kvifor kan eg ikkje publisere same versjon to gonger?

Registryet avviser duplikatversjonar for å halde publiserte pakkar deterministiske og sporbare.

### Kvifor bruker eg `path`-kjelder lokalt?

`path` er nyttig under utvikling når du vil jobbe på fleire pakkar i same workspace utan å publisere først.
