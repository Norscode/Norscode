# Frontend Layout Contract

Dette er den enkle kontrakten for sider og layout i Norscode-frontenden.

## Mål

- gjøre det tydelig hva en side er
- gjøre det tydelig hva en layout er
- holde layout og sideinnhold adskilt
- gjøre det lett å bygge gjenbrukbare sider

## Kontrakt

### Side

En side er en funksjon eller modul som:

- tar inn app-kontekst eller side-kontekst
- bygger innhold for én visning
- returnerer HTML eller en side-struktur som kan renderes til HTML

### Layout

En layout er en funksjon eller modul som:

- tar inn sideinnhold
- legger til felles struktur som header, footer, navigasjon og metadata
- kan gjenbrukes av flere sider

## Anbefalt form

- `side(...)` bygger innholdet
- `layout(...)` pakker inn innholdet
- `render(...)` eller `html(...)` gjør sluttkonvertering til respons

## Praktisk eksempel

- `page_home()` lager hovedinnholdet
- `layout_main(content)` legger rundt felles ramme
- `render_page(...)` sender responsen tilbake til `std.web`

## Regler

- En side bør ikke eie hele appens struktur alene.
- Layout bør ikke kjenne detaljer om hver enkelt side.
- Gjenbruk bør skje via små, rene funksjoner.
- Alle sider bør kunne rendres uten skjult global tilstand.

## Hva dette betyr for roadmapen

Se [docs/FRONTEND_ROADMAP.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_ROADMAP.md).
Med denne kontrakten er frontend-etappe 1 litt nærmere ferdig.
