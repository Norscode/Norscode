# Frontend Router

`std.html_router` gir en lett rute- og navigasjonsflate for server-renderte HTML-sider.
Pakken er tekstbasert og passer til dagens Norscode runtime.

## Navigasjon

- `route_link(...)`
- `route_link_active(...)`
- `route_link_for(...)`
- `route_link_for_url(...)`
- `route_nav(...)`
- `route_item(...)`
- `route_item_for(...)`
- `route_list(...)`

Aktive lenker får `aria-current="page"` slik at både bruker og hjelpeteknologi ser hvilken side som er valgt.
`route_link_for_url(...)` sammenligner base-path og ignorerer query-parametre når aktiv lenke skal velges.

## URL Og Parametre

- `route_base_path(...)`
- `route_query(...)`
- `route_with_query(...)`
- `route_path_param(...)`
- `route_path_params(...)`

`route_query(...)` og `route_with_query(...)` bruker `std.url` for trygg query-encoding.
`route_path_param(...)` erstatter `{id}`-liknende segmenter med URL-enkodet verdi.

## Brødsmuler

- `route_breadcrumb(...)`
- `route_crumb(...)`
- `route_crumb_current(...)`

## Sider

- `route_page(...)`
- `route_page_med_nav(...)`
- `route_index(...)`
- `simple_site(...)`

`route_page(...)` og `route_page_med_nav(...)` legger inn canonical metadata for ruten.

Eksempel:

```norscode
bruk std.html_router som router

router.route_page_med_nav(
    "/tickets",
    "Saker",
    "Saksliste",
    router.route_link_for("/tickets", "/", "Hjem")
        + router.route_link_for("/tickets", "/tickets", "Saker"),
    "<p>Innhold</p>",
    "/app.css"
)
```
