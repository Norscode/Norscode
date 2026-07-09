# Frontend Forms

`std.html_forms` gir høyere nivå skjema-komponenter oppå `std.html`.
Pakken bør brukes når en app trenger vanlige felt med label, hjelpetekst, feilmelding og tilgjengelige attributter.

## Felt

- `tekstfelt(...)`
- `epostfelt(...)`
- `passordfelt(...)`
- `tekstområde(...)`
- `velg(...)`
- `avkryssing(...)`
- `søkefelt(...)`
- `feltgruppe(...)`

Felt med feilmelding får `aria-invalid="true"` og kobler både hjelp og feil til samme kontroll via `aria-describedby`.

## Feil Og Handlinger

- `feilsammendrag_rad(...)`
- `feilsammendrag(...)`
- `submit_knapp(...)`
- `submit_knapp_busy(...)`
- `avbryt_lenke(...)`
- `handlinger_standard(...)`

`feilsammendrag(...)` returnerer tom tekst når listen er tom, og ellers en tilgjengelig `error-summary` med `role="alert"`.
`feltgruppe(...)` pakker relaterte felt i `fieldset` og `legend`.

## Tilstander

Bruk attributt-hjelperne sammen med `*_med_attrs(...)`-variantene når feltet trenger ekstra tilstand:

- `påkrevd_attrs()`
- `deaktivert_attrs()`
- `skrivebeskyttet_attrs()`
- `autocomplete_attr(...)`
- `submit_busy_attrs(...)`

Eksempel:

```norscode
bruk std.html_forms som forms

forms.epostfelt_med_attrs(
    "epost",
    "E-post",
    "",
    "navn@domene.no",
    "Bruk arbeidsadresse",
    "",
    forms.påkrevd_attrs() + forms.autocomplete_attr("email")
)
```

Eksempel med feilsammendrag:

```norscode
forms.feilsammendrag(
    "Rett feilene før du fortsetter",
    forms.feilsammendrag_rad("#felt-epost", "E-post mangler")
)
```
