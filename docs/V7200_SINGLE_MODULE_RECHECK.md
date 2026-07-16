# V7200 Single-module recheck

## Resultat
Recheck vart køyrd på to måtar etter `v7000`-patchen:

1. `tests/fixtures/v6800/bundler_single_module_probe_v6800.no`
2. direkte `run selfhost/bundler.no` med éin modul i `NORSCODE_BUNDLE_ARGS`

Begge gav framleis:
- `norscode: Stack underflow`

## Tolkning
Dette betyr at `v7000`-patchen i kjelde åleine ikkje er nok til å endre aktiv åtferd i `dist/norscode_native`-bana.

Den mest sannsynlege forklaringa er no:
- aktiv runtime køyrer framleis ein bundle/compile-løype som ikkje har plukka opp den nye kjeldeendringa i `selfhost/bundler.no`
- eller stack-underflow skjer tidlegare enn den patched delen rekk å få effekt i aktiv bane

## Konklusjon
Feilen er framleis reproduserbar i aktiv bane.
Vi må derfor gå eitt nivå djupare enn kjeldepatch i `bundler.no` og sjå på korleis aktiv `dist` faktisk køyrer bundle-sporet.
