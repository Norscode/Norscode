# Selfhost Fase 4 - Modulsystem og importflyt v1

Dette dokumentet samlar den første fase-4-flata for modulsystem og importflyt.

## Formål

- Gjere modullasting meir tydeleg
- Skilje intern og offentleg modulfløy
- Gjere importflyten enkel å lese for utviklarar
- Byggje vidare på eksisterande `bruk`- og `importer`-støtte

## Byggjer på

- `selfhost/nc_main.no`
- `selfhost/compiler/ir_to_bytecode.no`
- `selfhost/std_compat.no`
- [docs/SELFHOST_PHASE3_INTEGRATION_MAP.md](/Users/jansteinar/Projects/Norscode1/docs/SELFHOST_PHASE3_INTEGRATION_MAP.md)

## Eksisterande flyt

### Modulnamn til filsti

- `selfhost/nc_main.no` bruker `_modul_filsti_fra_namn(...)`
- `selfhost/compiler/ir_to_bytecode.no` bruker `finn_modul_filsti(...)` og `modul_til_filsti(...)`
- `selfhost/std_compat.no` viser modul-liknande wrapperstruktur for testsystem

### Importskann

- `finn_bruk_imports(...)` skannar kjeldetekst for `bruk` og `importer`
- importar blir normaliserte til alias + filsti
- transitive importar blir lagde i kø i import-bundling

### Bundle og last

- `nc_bundle_ncb(...)` lastar importerte modular før hovudfila
- importert funksjonsnamn blir skrivne om med modulalias
- `registrer_importerte_funksjonar(...)` kartlegg funksjonar frå modulfilene

## Praktisk kontrakt

- `bruk modul.namn` og `importer modul.namn` skal haldast lesbare
- alias skal vere siste segment når det ikkje er eksplisitt alias
- modulstiar skal kunne finnast deterministisk frå modulnamn

## Kva som er klart

- importflyten er allereie implementert nok til å dokumenterast
- modulnamn blir no normaliserte på ein kjent måte
- bundling av importerte modular er allereie ein del av hovudløypa

## Vidare arbeid

- tydelegare modullastingsstatus i eigen statusflate
- eventuelt meir presis namespace-kontroll
- meir dokumentasjon for transitive imports når nye modulområde kjem
