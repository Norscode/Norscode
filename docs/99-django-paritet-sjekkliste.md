# Django-paritet for Norscode

Denne lista viser kva som allereie finst, kva som berre er delvis på plass, og kva som framleis manglar for å nå ei Django-liknande totalflate.

## Allereie på plass

- [x] Webgrunnlag i `std/web.no`
- [x] ORM-grunnlag i `std/orm.no`
- [x] Migrasjonslag i `std/migrasjon.no`
- [x] Konfigurasjon i `std/innstillingar.no`
- [x] CLI-rammeverk i `std/cli.no`
- [x] Auth-grunnlag i `std/auth.no`
- [x] Session-lag i `std/sesjon.no`
- [x] Admin-CRUD i `std/admin.no`
- [x] Samla web-stack i `std/web_app_stack.no`
- [x] Prosjektgenerator via `nc startproject`
- [x] Appgenerator via `nc startapp`
- [x] Template-start i `src/templates/base.html` i scaffolden

## Delvis på plass

- [x] Eit fullt routing-system med namespacing og tydeleg app registry (startproject-generator, feature-trygging, route-registry)
- [x] Template-arv, include og partials
- [x] Forms-system med binding og validering
- [x] CSRF som standard i form-flyten
- [x] Ferdige login/logout/register-flows med brukaroppleving i scaffolden
- [x] Roller og permissions som meir komplette standardmodular
- [x] Admin med ModelAdmin-konfig, `list_display`, `search_fields`, `list_filter`, `readonly_fields`, widgets, permissions, CSRF, actions, historikk, søk, sortering og paginering
- [x] Query builder med chainable queryset-liknande API
- [x] Migrasjonar med autogenerering frå modellendringar
- [x] Test-runner med testdatabase og fixtures
- [x] Static/media-handsaming
- [x] Klarare settings/profil-mekanisme for dev/test/prod

## Framleis manglar

- [x] Foreign keys og relasjonar i ORM
- [x] Many-to-many og reverse relations
- [x] Joins, aggregat og annotering i ORM
- [x] Transactions med savepoints
- [x] Constraint- og indeksstyring som first-class API
- [x] Messages-system
- [x] Cache-system
- [x] Email/SMTP-hjelparar
- [x] I18n/l10n
- [x] Security middleware-pakke
- [x] Rate limiting og bruteforce-vern
- [x] Full deploy-historikk og lifecycle docs

## Kva eg har starta først

1. Gjere auth og admin meir modulære i scaffolden.
2. Lage ein tydeleg route-dispatch i prosjektmalen.
3. Utvide template- og forms-laget med testbare payload-malar.
4. Leggje til response-shape-validering i API-rutekjedene i både prosjekt- og app-skal.

## Fase 1 – Web API-paritet (I gang, delvis implementert)

- [x] Response-model-endepunkt i prosjektstack (`/api/v1/response-model`) med svar-validering.
- [x] Response-model-endepunkt i app-stack (`/api/v1/${APP_NAME}/response-model`) med svar-validering.
- [x] Ruteopplisting av nye API-ruter i `startproject` og `startapp`.
- [x] Testar for gyldig og ugyldig response-modell både i stack og app.
- [x] OpenAPI-spec-innehald validering for response-model-ruta.
- [x] Dokumentasjon/README i begge generatorar oppdatert med nye API-ruter.
- [x] Eksempelpayloadar for `payload`/`nested` lagt til i generert tests-mappe.

## Prioritert rekkjefølgje

- [x] 1. Fullføre routing og app registry
- [x] 2. Lage forms + template-arv
- [x] 3. Gjere ORM-relasjonar og migrations meir Django-like
- [x] 4. Utvide admin med Django-liknande ModelAdmin, filter, søk, actions, audit og sikker formflyt
- [x] 5. Leggje inn test-runner og fixtures
- [x] 6. Leggje til static/media og security middleware
