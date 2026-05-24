# Frontend Layout Reuse and Partials

Layout-gjenbruk og delvise visninger gjør frontend vedlikeholdbar.

De mest lesbare full-eksemplene ligger i [`docs/FRONTEND_GOLDEN_EXAMPLES.md`](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_GOLDEN_EXAMPLES.md).

## Mål

- gjenbruke layout på tvers av sider
- gjenbruke delvise visninger som kort, heads, footers og nav
- holde sidekode liten og lesbar

## Kontrakt

- layout er en wrapper rundt sideinnhold
- partials er små gjenbrukbare visninger
- sider setter sammen layout + partials + innhold

## Praktisk modell

- `std.frontend.layout_main(innhold)`
- `std.frontend.layout_app(...)`
- `std.frontend.slot(...)`
- `std.frontend.nav_link(...)`
- `std.frontend.komponent_nav(...)`
- `std.frontend.komponent_kort(...)`
- `std.frontend.komponent_tabell(...)`
- `std.frontend.komponent_liste(...)`
- server-drevne fragmenter og delvis rendering kan bygges med `std.islands.fragment_root(...)`, `std.islands.fragment_shell(...)` og `std.islands.fragment_or_full_response(...)`
- lavnivå partials kan fortsatt komme fra `std.html`

## Regler

- layout skal være stabil og enkel
- partials skal være små og spesifikke
- sider skal ikke duplisere markup som hører hjemme i layout eller partials

## Når dette er ferdig

- en side kan bygges av layout + partials + innhold
- gjenbruk er tydelig og forutsigbar
- markup-duplisering er redusert

Se også:

- [docs/FRONTEND_LAYOUT_CONTRACT.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_LAYOUT_CONTRACT.md)
- [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_COMPONENT_MODEL.md)
- [docs/FRONTEND_FRAGMENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_FRAGMENT_MODEL.md)
