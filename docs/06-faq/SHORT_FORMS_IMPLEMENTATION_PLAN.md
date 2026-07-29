# Implementasjonsplan for korte former

Denne planen beskriv korleis Norscode kan få raskare skriving gjennom korte former for funksjonar og manifest, utan å bryte eksisterande flyt.

## Mål

- Gjere funksjonssyntaks raskare aa skrive.
- Gjere `norcode.toml` mindre repetitiv.
- Behalde full syntaks som støtta standard.
- Innføre endringar i små steg med låg risiko.

## Prinsipp

1. Støtt begge former i ein overgangsperiode.
2. Gjer kortforma til anbefalt stil i docs og snippets.
3. La formattering og verktøy normalisere uttrykk.
4. Unngå å gjere språkreglane uleselege.

## Fase 1: Kort funksjonssyntaks

### Endringar

- Fjern `->` frå ny funksjonssyntaks.
- Gjør `funksjon` valfritt når parseren ser `namn(...)`.
- Tillat `:` for returtype.
- Tillat `ret` som kortform for `returner`.
- Tillat `= uttrykk` for korte éinlinje-funksjonar.

### Målbilete

```norscode
start() {
    skriv("Hei, Norscode!")
}
```

```norscode
tell(): heltall {
    ret 0
}
```

### Filer som treng arbeid

- parser
- AST-/funksjonsnode
- formatter
- dokumentasjon og eksempel

## Fase 2: Kort manifest

### Endringar

- Tillat `[package]` som alias for `[project]`.
- Tillat `deps` som alias for `dependencies`.
- Gi `version` og `entry` standardverdiar der det er trygt.
- Hald full manifestform fullt støtta.

### Målbilete

```toml
[package]
name = "min_app"
entry = "main.no"
deps = { std_math = "^1.0.0" }
```

### Filer som treng arbeid

- manifest-parser
- manifest-validering
- lockfile-/resolver-flyt
- docs og snippets

## Fase 3: Verktøy og migrering

### Endringar

- Formatter som kan skrive om trygg lang form til kort form.
- Snippets for app, bibliotek og testpakke.
- Autofullføring for nye korte former.
- Dokumentasjon som viser anbefalt stil først.

### Filer som treng arbeid

- formatter
- CLI-hjelp
- docs/quickstart/tutorial
- editor-snippets

## Anbefalt rekkefølge

1. Parser for kort funksjonssyntaks.
2. Formatter og docs for funksjonssyntaks.
3. Manifest-alias og standardverdiar.
4. Snippets og autofullføring.
5. Eventuell gradvis deprecate av lang form i docs.

## Sikkerheitsnett

- Aldri fjern lang form før kort form er verifisert og brukt i docs/testar.
- Hald parseren tolerant for begge former.
- Ver konservativ med standardverdiar, særleg der dei kan endre semantikk.

## Kva som gir mest verdi først

- `->` bort frå funksjonar.
- `funksjon` valfritt i enkle definisjonar.
- `ret` som kortform.
- Deretter kort manifest med `[package]` og `deps`.

## Konklusjon

Dette er den mest praktiske vegen til raskare skriving:

1. Gjer funksjonar kortare først.
2. Gjer manifest kortare etterpå.
3. La verktøy og docs hjelpe til med overgangen.
