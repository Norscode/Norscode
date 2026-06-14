# Frontend Fragment Patterns

Dette dokumentet samler noen enkle, JS-frie mønstre for server-drevne fragmenter i Norscode.

Bruk dem når du vil la serveren eie delvis rendering uten å innføre en klientruntime først.

## 1. Søk som fragment

Bruk query-parametre for filtrering og returner fragment når det er nyttig.

```no
funksjon sok(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /sok")

    la query = web.request_query_param(ctx, "q")
    la fragment = islands.fragment_shell("Søk", sok_innhold(query))

    returner islands.fragment_or_full_response(
        ctx,
        frontend.layout_app("Norscode Frontend", navigasjon, fragment, "Bygd med component mode."),
        fragment
    )
}
```

## 2. Paginert liste

Bruk `side` eller en tilsvarende query for å holde listen delbar og lesbar.

```no
funksjon poster(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /poster")

    la side = web.request_query_param(ctx, "side")
    la fragment = islands.fragment_shell("Poster", poster_innhold(side, web.request_query_param(ctx, "sort")))

    returner islands.fragment_or_full_response(
        ctx,
        frontend.layout_app("Norscode Frontend", navigasjon, fragment, "Bygd med component mode."),
        fragment
    )
}
```

## 3. Sorterbar visning

La sortering være en del av URL-en, ikke skjult intern state.

```no
la sortering = web.request_query_param(ctx, "sort")
la asc_link = "/poster?side=2&sort=asc"
la desc_link = "/poster?side=2&sort=desc"
```

## 4. Filtrerbar visning

La kategorier eller andre visningsvalg styres av query-parametre på samme måte som side og sortering.

```no
la kategori = web.request_query_param(ctx, "kategori")
la filter_link = "/poster?side=2&sort=asc&kategori=server"
```

## 5. Full side og fragment fra samme komponenter

La samme innhold bygges én gang og brukes i begge svarformer.

```no
la fragment = islands.fragment_shell("Status", status_innhold())

hvis (islands.fragment_request(ctx)) {
    returner islands.fragment_response_ok(fragment)
}

returner web.response_builder(
    200,
    {"content-type": "text/html; charset=utf-8"},
    frontend.layout_app("Norscode Frontend", navigasjon, fragment, "Bygd med component mode.")
)
```

## 6. Path-basert detaljvisning

Bruk path-parametre når identiteten er en del av URL-en og du vil ha en tydelig, delbar detaljside.

Det samme mønsteret kan brukes for flere typer detaljer, for eksempel brukere, artikler eller prosjekter.

Det er ofte nyttig å ha en egen samleside, for eksempel `GET /artikler`, som lenker videre til detaljsidene.

En enklere variant er en rutesamleside, for eksempel `GET /ruter`, som viser de viktigste URL-ene i appen og gjør dem lett å teste og dele.

```no
funksjon bruker(ctx: ordbok_tekst) -> ordbok_tekst {
    web.route("GET /brukere/{id}")

    la id = web.request_param_int(ctx, "id")
    la fragment = islands.fragment_shell("Bruker", bruker_innhold(id, web.request_query_param(ctx, "visning")))

    returner islands.fragment_or_full_response(
        ctx,
        frontend.layout_app("Norscode Frontend", navigasjon, fragment, "Bygd med component mode."),
        fragment
    )
}
```

## Regler

- bruk fragmenter når delvis rendering faktisk gir verdi
- la URL-en forklare hva som vises
- behold samme komponenter i fullside- og fragment-svar
- hold JS-fri mellomvei enkel og forutsigbar

Se også:

- [docs/FRONTEND_FRAGMENT_MODEL.md](./FRONTEND_FRAGMENT_MODEL.md)
- [docs/FRONTEND_FRAGMENT_EXAMPLES.md](./FRONTEND_FRAGMENT_EXAMPLES.md)
- [docs/FRONTEND_FRAGMENT_PLAYBOOK.md](./FRONTEND_FRAGMENT_PLAYBOOK.md)
- [`examples/frontend.no`](../examples/frontend.no)
