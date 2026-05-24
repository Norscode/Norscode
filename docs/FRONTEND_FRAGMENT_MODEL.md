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

## Praktisk bruk

- en route kan returnere full side ved vanlig navigasjon
- en route kan returnere fragment innhold når du vil isolere en del av siden
- samme komponenter kan brukes i begge tilfeller

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

## Regler

- bruk fragmenter når du vil isolere en del av server-renderingen
- ikke bland fragment-kontrakt og fullsidekontrakt uten å navngi forskjellen
- la URL og server være sannheten, ikke skjult klienttilstand

## Når dette er ferdig

- serveren kan returnere både hele sider og tydelige delvis-renderte biter
- route-flyten holder seg stabil uten å kreve JS

Se også:

- [docs/FRONTEND_SERVER_FALLBACK.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_SERVER_FALLBACK.md)
- [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_NAVIGATION_MODEL.md)
- [`std/islands.no`](/Users/jansteinar/Projects/Norscode/std/islands.no)
