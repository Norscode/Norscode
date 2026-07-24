# Kort manifest for Norscode

Dette dokumentet skisserer ei kortare form for `norcode.toml` som er raskare å skrive for små og mellomstore prosjekt.

Maalet er å redusere mengda repetisjon utan å miste tydeligheit.

## Mål

- Gjere manifestet kortare i vanleg bruk.
- Behalde full manifestform som støtta standard.
- Gi gode standardverdiar for enkle prosjekt.
- Gjere det lettare å starte nye pakkar og app-prosjekt.

## Foreslått kortform

### Dagens form

```toml
[project]
name = "mitt_bibliotek"
version = "1.0.0"
entry = "lib.no"

[dependencies]
std_math = "^1.0.0"
```

### Kort form

```toml
[package]
name = "mitt_bibliotek"
entry = "lib.no"
deps = { std_math = "^1.0.0" }
```

## Foreslåtte snarvegar

### `project` -> `package`

- Kortare seksjonsnamn for vanleg pakkeoppretting.
- Full form kan fortsatt støttast.

### `dependencies` -> `deps`

- Kort alias for avhengigheiter.
- Passar godt for små manifest.

### Standardverdiar

- `version = "0.1.0"` kan vere standard i nye prosjekt.
- `entry = "main.no"` kan vere standard når ikkje oppgitt.

## Anbefalt kort manifest

```toml
[package]
name = "min_app"
entry = "main.no"
deps = {
  std_math = "^1.0.0",
  std_io = "^1.0.0"
}
```

## Parserreglar

- `package` og `project` kan begge bli akseptert som seksjonsnamn.
- `deps` og `dependencies` kan peike til same internrepresentasjon.
- Full manifestform skal alltid fungere.
- Kortformen bør vere særleg god for nye prosjekt og enkle bibliotek.

## Overgangsplan

### Fase 1: Støtt begge former

- Behald eksisterande `norcode.toml`.
- Legg til støtte for kort form.

### Fase 2: Snippets og malar

- Standardmalar kan bruke kort form.
- Lang form kan bli brukt i dokumentasjon der tyding er viktig.

### Fase 3: Formatter og konvertering

- Verktøy kan eventuelt konvertere lang form til kort form når det er sikkert.

## Praktisk anbefaling

Start med desse tre endringane:

1. Tillat `[package]` som alias for `[project]`.
2. Tillat `deps` som alias for `dependencies`.
3. Gi `version` og `entry` trygge standardverdiar.

Det gir mykje kortare manifest utan å tvinge bort den gamle forma.
