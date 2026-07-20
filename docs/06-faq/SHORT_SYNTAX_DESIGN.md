# Kortsyntaks for Norscode

Dette dokumentet skisserer ein kortare og raskare syntaks for vanlege funksjonar i Norscode.

Maalet er aa gjere koden raskare aa skrive utan aa miste lesbarheit.

## Mål

- Fjerne `->` frå vanleg funksjonssyntaks.
- Gjere `funksjon` valfritt i enkle definisjonar.
- Bruke returtype berre når ho trengst.
- La kort form og lang form leve side om side i ein overgangsperiode.

## Foreslått kort syntaks

### Funksjon utan nøkkelord

```norscode
start() {
    skriv("Hei, Norscode!")
}
```

Regel:
- Dersom ei linje startar med `namn(...)`, skal parseren tolke det som ei funksjonsdefinisjon.
- `funksjon` blir då valfritt.

### Returtype berre når nødvendig

```norscode
tell(): heltall {
    ret 0
}
```

Regel:
- `:` etter parameterlista introduserer returtype.
- Om returtype ikkje er oppgitt, er funksjonen implisitt `ingenting`/void.

### Kort retur

```norscode
tell(): heltall {
    ret 0
}
```

Regel:
- `ret` er kortform for `returner`.
- Begge former kan støttast.

### Énlinje-funksjon

```norscode
doble(x): heltall = x * 2
```

Regel:
- `= uttrykk` etter funksjonshovud tyder kort funksjon med implisitt retur.

## Anbefalt stil

For vanleg bruk bør dette vere standard:

```norscode
start() {
    skriv("Hei, Norscode!")
}
```

Og når returtype er viktig:

```norscode
tell(): heltall {
    ret 0
}
```

## Overgangsplan

### Fase 1: Støtt begge syntaksar

- Behald dagens form:

```norscode
funksjon start() -> heltall {
    returner 0
}
```

- Legg til ny form:

```norscode
start(): heltall {
    ret 0
}
```

### Fase 2: Formatter til kort stil

- Formattereren kan skrive om trygg lang syntaks til kort syntaks.
- Dette bør berre skje når tydinga er eintydig.

### Fase 3: Dokumenter kort stil som standard

- Nye eksempel, tutorials og quickstarts bør bruke kortsyntaksen.

### Fase 4: Eventuell deprecate av lang form

- Den lange forma kan halde fram som støtta, men markerast som mindre anbefalt.

## Parserreglar

- `funksjon` er valfritt i enkle definisjonar.
- `->` blir ikkje brukt i ny kortsyntaks.
- `:` etter parameterlista betyr returtype.
- `= uttrykk` betyr kort funksjon med implisitt retur.
- Parseren bør prioritere funksjonstolking når mønsteret er:
  - `namn(...)`
  - eventuelt `: type`
  - deretter blokk eller `= uttrykk`

## Praktisk anbefaling

Start med desse tre reglane:

1. Gjør `funksjon` valfritt.
2. Fjern `->` frå ny syntaks.
3. La `ret` vere kortform for `returner`.

Det gir mykje raskare skriving utan å gjere språket rotete.
