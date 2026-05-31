# Frontend Fragment Model

Norscode kan bruke server-drevne fragmenter når en app vil levere deler av en side eksplisitt, uten å introdusere en klientruntime først.

## Mål

- gjøre delvis rendering tydelig
- la serveren eie oppdatering av sidebiter
- beholde full side-rendering som trygg standard
- holde modellen JS-fri

## Modell

- `std.islands.fragment_root(...)` markerer et fragmentområde i markup
- `std.islands.fragment_shell(...)` gir en liten wrapper for delvis innhold
- `std.islands.fragment_response(...)` og `std.islands.fragment_response_ok(...)` returnerer fragment-svar fra serveren
- `std.frontend.fragment_page(...)` er en anbefalt høy-nivå helper når en route skal kunne svare både som full side og fragment

## Praktisk bruk

- en route kan returnere full side ved vanlig navigasjon
- en route kan returnere fragment innhold når du vil isolere en del av siden
- samme komponenter kan brukes i begge tilfeller
- typiske eksempler er søk, paginerte lister og sorterte visninger, der samme URL kan gi både fullside- og fragment-svar

## Eksempel

```no
funksjon status(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /status")

    la fragment = islands.fragment_shell("Status", status_innhold())

    hvis (islands.fragment_request(ctx)) {
        returner islands.fragment_response_ok(fragment)
    }

    returner web.response_builder(
        200,
        {"content-type": "text/html; charset=utf-8"},
        frontend.layout_app("Norscode Frontend", toppnavigasjon, fragment, "Bygd med component mode.")
    )
}
```

Et annet praktisk mønster er en søke- eller liste-route:

```no
funksjon sok(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /sok")

    la query = web.request_query_param(ctx, "q")
    la fragment = islands.fragment_shell("Søk", sok_innhold(query))

    returner islands.fragment_or_full_response(
        ctx,
        frontend.layout_app("Norscode Frontend", toppnavigasjon, fragment, "Bygd med component mode."),
        fragment
    )
}
```

## Regler

- bruk fragmenter når du vil isolere en del av server-renderingen
- ikke bland fragment-kontrakt og fullsidekontrakt uten å navngi forskjellen
- la URL og server være sannheten, ikke skjult klienttilstand

## Når dette er ferdig

- serveren kan returnere både hele sider og tydelige delvis-renderte biter
- route-flyten holder seg stabil uten å kreve JS

## Se også

- [`FRONTEND_SERVER_FALLBACK`](./FRONTEND_SERVER_FALLBACK.md)
- [`FRONTEND_NAVIGATION_MODEL`](./FRONTEND_NAVIGATION_MODEL.md)
- [`FRONTEND_FRAGMENT_EXAMPLES`](./FRONTEND_FRAGMENT_EXAMPLES.md)
- [`FRONTEND_FRAGMENT_PATTERNS`](./FRONTEND_FRAGMENT_PATTERNS.md)
- [`FRONTEND_FRAGMENT_PLAYBOOK`](./FRONTEND_FRAGMENT_PLAYBOOK.md)
- [`FRONTEND_LEARNING_PATH`](./FRONTEND_LEARNING_PATH.md)
- [`std/islands.no`](../std/islands.no)
