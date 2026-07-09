# Frontend Router

`std.html_router` gir en lett rute- og navigasjonsflate for server-renderte HTML-sider.
Pakken er tekstbasert og passer til dagens Norscode runtime.

## Navigasjon

- `route_link(...)`
- `route_link_active(...)`
- `route_link_for(...)`
- `route_nav(...)`
- `route_item(...)`
- `route_item_for(...)`
- `route_list(...)`

Aktive lenker får `aria-current="page"` slik at både bruker og hjelpeteknologi ser hvilken side som er valgt.

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

