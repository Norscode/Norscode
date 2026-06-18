# Selfhost Fase 4 - Breiare standardbibliotek v1

Dette dokumentet samlar fase-4-planen for å utvide standardbiblioteket utover den smale fase-2-løypa.

## Formål

- Gjere standardbiblioteket meir nyttig i normal utvikling
- Utvide frå dei prioriterte fase-2-modulane til eit breiare dekningsbilete
- Halde same lesereglar: klar API-flate, tydeleg status og praktiske brukscase

## Tre første utvidingsområde

### 1. Web og request/response

Byggjer på:

- `std/web.no`
- `std/web_app_stack.no`
- `std/http.no`
- `std/wsgiref.no`

Praktisk bruk:

- rutehandtering
- request/response-form
- samla web-stack for appar

### 2. Data, ORM og migrasjonar

Byggjer på:

- `std/db.no`
- `std/orm.no`
- `std/migrasjon.no`

Praktisk bruk:

- modell- og query-arbeid
- migrasjonsflyt
- databasetilgang med tydeleg status

### 3. Auth, session og admin

Byggjer på:

- `std/auth.no`
- `std/sesjon.no`
- `std/admin.no`

Praktisk bruk:

- innlogging og tilgangsstyring
- session-handsaming
- admin- og CRUD-flate

## Kva vi ser etter

- kvar modul skal ha klar API-flate
- kvar modul skal ha tydeleg null-/feilkontrakt
- kvar modul skal ha praktiske brukscase
- kvar modul skal vere lesbar som normal utviklingsstøtte

## Kva som er klart frå fase 2

- standardbibliotek-løypa er allereie etablert
- statusmatrise og brukscase er allereie dokumenterte
- fase 2-modulane er allereie modne som referanse

## Vidare arbeid

- byggje fleire modulstatusar når nye område er klare
- utvide brukscase-oversikta
- halde same statusmønster som i fase 2
