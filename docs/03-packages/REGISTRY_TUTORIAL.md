# Registry tutorial

Denne tutorialen viser heile flyten steg for steg, frå å lage ei pakke til å bruke henne i eit anna prosjekt.

## Steg 1: Lag pakka

Opprett mappa `std_math/` og legg inn desse filene:

```text
std_math/
  norcode.toml
  math.no
  README.md
```

### `norcode.toml`

```toml
[project]
name = "std_math"
version = "1.0.0"
entry = "math.no"

[dependencies]
```

### `math.no`

```norscode
funksjon addere(a, b)
    returner a + b
slutt
```

## Steg 2: Lås pakken

Kjør:

```sh
nc lock
```

Det lagar `norcode.lock` frå manifestet og avhengigheiter.

## Steg 3: Hent og verifiser

Kjør:

```sh
nc fetch
```

Dette lastar ned pakkar til cache og verifiserer checksum.

## Steg 4: Publiser

Når pakka er klar:

```sh
nc publish --token <token>
```

Publisering krev gyldig token og vil avvise same versjon om ho allereie finst i registryet.

## Steg 5: Bruk pakka i eit anna prosjekt

Opprett `min_app/`:

```text
min_app/
  norcode.toml
  main.no
```

### `norcode.toml`

```toml
[project]
name = "min_app"
version = "0.1.0"
entry = "main.no"

[dependencies]
std_math = "^1.0.0"
```

### `main.no`

```norscode
funksjon start() -> heltall {
    verdi = addere(2, 3)
    skriv(verdi)
    returner 0
}
```

### Installer

```sh
nc install
```

Dette løyser avhengigheiter, skriv lockfila og hentar pakkar.

## Steg 6: Lokal utvikling

Om du vil jobbe på begge pakkar samtidig:

```toml
[dependencies]
std_math = "./packages/std_math"
```

Det gjer at `min_app` brukar ei lokal kopling i staden for registry-versjonen.

## Vanlege problem

- Manglande token: bruk `--token <token>`.
- `409 Conflict`: auk versjonsnummeret.
- `410 Gone`: pakka er yanket.
- checksum-mismatch: sjekk lockfila og registry-metadata.

## Vidare lesing

- [REGISTRY_QUICKSTART](REGISTRY_QUICKSTART.md)
- [REGISTRY_EXAMPLE](REGISTRY_EXAMPLE.md)
- [PACKAGES](PACKAGES.md)
