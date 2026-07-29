# Sammenlikning av korte former

Denne sida samanliknar dagens språk- og manifestform med dei foreslåtte kortformene.

Målet er å vurdere kva som gir mest fart med minst risiko.

## Funksjonssyntaks

### Dagens form

```norscode
funksjon start() -> heltall {
    skriv("Hei, Norscode!")
    returner 0
}
```

### Kort form

```norscode
start(): heltall {
    ret 0
}
```

### Enda kortare form

```norscode
start() {
    skriv("Hei, Norscode!")
}
```

## Manifest

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

## Praktisk vurdering

### Mest konservativ

- Behald dagens form.
- Legg berre til `ret` som kortform.
- Bruk snippets og autofullføring.

Fordel:
- Minst risiko.

Ulempe:
- Mindre gevinst i skriving.

### God balanse

- Fjern `->`.
- Gjør `funksjon` valfritt.
- Tillat `:` for returtype.
- Tillat `[package]` og `deps` som alias i manifest.

Fordel:
- Mye raskare å skrive.
- Framleis ganske lesbart.

Ulempe:
- Parseren får fleire lovlege former.

### Mest aggressiv

- Gjer både funksjon og manifest svært korte.
- Bruk standardverdier mykje meir.

Fordel:
- Raskast å skrive.

Ulempe:
- Kan bli vanskeligare å lese for nye brukarar.

## Anbefalt rekkefølge

1. Kort funksjonssyntaks utan `->`.
2. `ret` som kortform for `returner`.
3. `[package]` og `deps` som manifest-alias.
4. Standardverdiar for `entry` og `version`.
5. Snippets og formattering som støttar kortformene.

## Konklusjon

Den beste balansen ser ut til å vere å korte ned det som er mest skriveintensivt, men ikkje gjere alt ultrakompakt med ein gong.

Det betyr:
- kort funksjonssyntaks først
- kort manifest etterpå
- full syntaks held fram som støtta standard

