# NorsDB: kontrollert gjenoppretting frå .cp2

Når kompaktering blir avbroten, kan både `.cp` og `.cp2` finnast.
Tidlegare vart alle `.cp2` med ein vilkårleg Z-markør kopierte til `.cp`,
sjølv om `.cp` inneheldt nyare fullførte transaksjonar. Det kunne føre til
tilbakerulling og datatap ved neste opning.

## Val av checkpoint

- Finn siste fullførte S/Z-segment og epoken for begge filer.
- Ei `.cp2` må ha avsluttande komplett segment og samsvarande S/Z-epoke.
- Gjenopprett frå `.cp2` berre om epoken ikkje er eldre enn den siste
  fullførte epoken i `.cp`.
- Ei eldre eller ufullstendig `.cp2` blir kasta, og `.cp` blir brukt.
- Den eksisterande WAL-replayen og handtering av trunkerte `.cp`-halar er
  uendra.

## Regresjon

`./bin/nc run tests/test_norsdb_checkpoint.no` prøver både eldre `.cp2`
med ein nyare `.cp` og ei ufullstendig `.cp2`, og kontrollerer at rader
ikkje forsvinn. CI og manuelt testresultat må stadfestast før merge.

## Avgrensingar

Dette er ei avgrensa recovery-retting, ikkje fullstendig durabilitetsgaranti.
Diskfeil, delvis skrift, korrupt data og fsync/atomisk rename krev vidare arbeid.
