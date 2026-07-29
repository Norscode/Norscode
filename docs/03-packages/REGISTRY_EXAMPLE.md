# Registry example

Dette er eit konkret eksempel på korleis pakke-flyten kan sjå ut i praksis.

## Prosjekt A: `std_math`

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

funksjon subtrahere(a, b)
    returner a - b
slutt
```

### Flyt

```sh
nc lock
nc fetch
nc publish --token <token>
```

## Prosjekt B: `min_app`

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

### Flyt

```sh
nc lock
nc install
```

## Lokal utvikling

Under aktiv utvikling kan `min_app` peike på ei lokal kopi av `std_math`:

```toml
[dependencies]
std_math = "./packages/std_math"
```

Det er nyttig når du vil teste endringar før du publiserer ny versjon.

## Resultat

- `std_math` er publisert som `1.0.0`
- `min_app` låser avhengigheita til `std_math`
- `nc install` hentar og verifiserer pakken frå registry eller cache

## Vidare lesing

- [REGISTRY_QUICKSTART](REGISTRY_QUICKSTART.md)
- [PACKAGES](PACKAGES.md)
- [REGISTRY_API](REGISTRY_API.md)
