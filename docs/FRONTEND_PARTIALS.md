# Frontend Layout Reuse and Partials

Layout-gjenbruk og delvise visninger gjør frontend vedlikeholdbar.

## Mål

- gjenbruke layout på tvers av sider
- gjenbruke delvise visninger som kort, heads, footers og nav
- holde sidekode liten og lesbar

## Kontrakt

- layout er en wrapper rundt sideinnhold
- partials er små gjenbrukbare visninger
- sider setter sammen layout + partials + innhold

## Praktisk modell

- `layout_main(innhold)`
- `partial_nav(...)`
- `partial_card(...)`
- `partial_table(...)`

## Regler

- layout skal være stabil og enkel
- partials skal være små og spesifikke
- sider skal ikke duplisere markup som hører hjemme i layout eller partials

## Når dette er ferdig

- en side kan bygges av layout + partials + innhold
- gjenbruk er tydelig og forutsigbar
- markup-duplisering er redusert

Se også:

- [docs/FRONTEND_LAYOUT_CONTRACT.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_LAYOUT_CONTRACT.md)
- [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_COMPONENT_MODEL.md)
