# V7800 Direct list callsite patches

## Formål
Dette steget rydder bort direkte liste-literal som funksjonsargument i kritiske selfhost-filer, sidan aktiv compile-path no er bevist sårbar nettopp der.

## Patchede filer
- `selfhost/nc_main.no`
- `selfhost/lsp/server.no`
- `selfhost/bootstrap_compiler/remove_python_bootstrap.no`

## Kva som er gjort
I staden for mønster som:
- `fn([..])`

bruker kjelda no mellomvariablar bygde via tryggare vegar før funksjonskallet.

## Kvifor
`v7600` viste:
- direkte liste-literal i kallargument gir `Stack underflow`
- liste bygd først i variabel og deretter sendt inn fungerer

Dermed er dette eit naturleg source-side vern før vidare refresh av aktiv bundle/compiler-bane.

## Viktig avgrensing
Dette betyr ikkje at aktiv `dist/norscode_native` alt har plukka opp endringane. Det betyr berre at kjelda no i mindre grad brukar eit mønster vi veit er brote i aktiv compile-path.
