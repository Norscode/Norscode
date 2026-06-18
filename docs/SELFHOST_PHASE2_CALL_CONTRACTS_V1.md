# Selfhost Fase 2 - Call Contracts v1

Dette dokumentet skisserer dei første kall-kontraktane for builtin og extern-modular.

## Mål

- Gjere kall mellom modulane føreseielege
- Gjere det tydeleg kva som er stabilt
- Gjere det lettare å skrive test og FFI-kontrollar

## Builtin-kontraktar

Builtin-funksjonar er interne runtime-hjelparar som skal kunne kallast direkte frå standardbibliotek og selfhost-kode.

### Krav

- Stabile namn i `builtin.*`
- Stabile parameterrekkjer
- Klå feilmelding når funksjon manglar eller får ugyldig input
- Returntype skal vere dokumentert og ikkje skifte stille

### Døme på builtin-område

- `builtin.lengde`
- `builtin.json_parse`
- `builtin.json_stringify`
- `builtin.fil_les`
- `builtin.fil_skriv`
- `builtin.miljo_hent`
- `builtin.miljo_finnes`
- `builtin.skriv`
- `builtin.assert`
- `builtin.assert_eq`

## Extern-kontraktar

Extern-modular er offentlege modulgrensesnitt som skal oppføre seg som stabile API frå andre delar av systemet.

### Krav

- Modulnamn skal vere eksplisitte og dokumenterte
- Eksporterte funksjonar skal ha stabil signatur
- Ein modul kan leggje til nye funksjonar utan å bryte gamle
- Fjerning eller omtolking av funksjonar krev ny versjon eller ny modulflate

### Døme på extern-modular

- `std.log`
- `std.fil`
- `std.cache`
- `std.lagring`
- `std.innstillingar`
- `std.sched`
- `std.tråd`

## Kallflyt

- `std`-modular kan kalle `builtin.*`
- runtime-lag kan kalle `builtin.*` og interne hjelpefunksjonar
- extern-modular skal ikkje stole på skjulte hjelpefunksjonar utanfor dokumentert flate

## Feilreglar

- Manglande builtin skal gje eksplisitt feil
- Ugyldig modul eller ugyldig kall skal feile deterministisk
- Feil skal vere mogleg å sjå i test og CI

## Stabilitet

- Kall-kontraktar skal ikkje endre tyding stille
- Eit nytt valfritt argument krev dokumentasjon
- Eit nytt kall skal innførast som ny signatur, ikkje ved å endre gammal
