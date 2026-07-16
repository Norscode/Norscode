# Frontend HTML Islands

`std.html_islands` gir små, server-renderte widgeter som kan kobles til standard interaksjonsscript fra `std.html`.
Pakken passer når en side fortsatt skal virke som vanlig HTML, men trenger litt interaktiv oppførsel.

## Widgeter

- `teller(...)`
- `toggle(...)`
- `tabs(...)`
- `modal(...)`
- `dropdown(...)`
- `live_search(...)`
- `live_filter(...)`
- `autoresize_tekstområde(...)`
- `autosubmit_search(...)`
- `json_form(...)`

## Handlinger

- `island_action(...)`
- `island_submit(...)`
- `confirm_action(...)`
- `confirm_submit(...)`
- `copy_knapp(...)`
- `modal_open_knapp(...)`
- `modal_close_knapp(...)`

## Standard Script

Komponentene bruker samme `data-nc-*` kontrakt som `std.html`.
Legg `html.script_standard()` nederst på siden når du bruker dropdown, modal, kopiering, confirm, autoresize, autosubmit, JSON-skjema eller live-filter.

Eksempel:

```norscode
bruk std.html som html
bruk std.html_islands som islands

islands.dropdown(
    "statusvalg",
    "Endre status",
    islands.island_action("open", "Åpen")
        + islands.island_action("closed", "Lukket")
)

html.script_standard()
```

Eksempel med kopiering:

```norscode
islands.copy_knapp("Kopier", "kode", "Kopiert")
```
