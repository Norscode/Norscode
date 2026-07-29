# V8200 Bundle args materialization

## Formål
Dette steget testar om problemet sit i sjølve oppbygginga av `reine_modular` i `bundler.start`, eller om det berre sit i miljøvariabelvegen.

## Prober
- `empty_list_append_probe_v8200.no`
- `bundler_start_emulated_v8200.no`

## Kva dei måler
- om `la liste = []` + `legg_til(...)` + funksjonskall fungerer
- om ein handskriven kopi av `bundler.start`-logikken fungerer når han får fast `args_str`

## Tolkning
- dersom begge er grøne, er miljø-/inngangsvegen den sterkaste attverande mistenkte
- dersom emulert `bundler.start` er raud, sit feilen i logikken til `bundler.start` før eller ved kall til `bygg_bundle`
