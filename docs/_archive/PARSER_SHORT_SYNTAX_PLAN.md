# Plan for kortfunksjonssyntaks i Norscode

## Status no

1. `selfhost/parser.no`
   - `parse_setning` har eksplisitt kortfunksjon-sjekk for mønsteret `IDENT (...)` etterfølgt av `LBRACE/COLON/ASSIGN/ARROW`.
   - `parse_program` er oppdatert til å prøve `parse_kort_funksjon` før generisk `parse_setning`.
   - Ny hjelpefunksjon for kortfunksjon (`er_kort_funksjon`) er lagt inn.

2. Artefakt-synk
   - `selfhost/parser.ncb.json` er regenerert frå oppdatert parser-kjelde.
   - `bootstrap/precompiled/parser.ncb.json` og dei relevante `bootstrap/precompiled_fragments*`-filene er oppdatert/synkroniserte.

3. Observasjon
   - Køyretest med `./bin/nc run /tmp/check_short.nor_tmp.no` og `./bin/nc test selfhost/tests/parser_tests.no` feilar på parsertrinnet (`Parserfeil ... - forventet uttrykk` ved EOF).
   - `./bin/nc check archive/legacy_c_backend/ncb_to_c.no` feilar på same mønster (`Parserfeil 493:1`), så parserfeilen kjem før generatorfasen blir verifisert i fullt omfang.

## Hva som manglar

1. `dist/norscode_native` nyttar framleis ein eldre innebygd parser-kjelde i runtime.
2. Forsøk på rebuild:
   - `tools/maint/regen_native.sh --rebuild` startar, men bryt på bygg-steg:
     - parser/kompilering av `archive/legacy_c_backend/ncb_to_c.no` feilar.
     - påfølgjande steg (clang) møter problem i generert C dersom ncb_to_c kjem heilt gjennom.
3. Difor får vi enno ikkje parserendringa heilt ut i aktiva `./bin/nc`-køyring og testløp.

## Neste steg (konkret)

1. Reparér parserfeilen i `archive/legacy_c_backend/ncb_to_c.no` (line 493) som gjer at runtime parser stoppar ved EOF i siste funksjonsblokk.
2. Re-test at minimal parsing (`./bin/nc check` av små funksjonsfiler) går gjennom med no parser-implementasjon.
3. Rebase/rebuild `dist/norscode_native` via `tools/maint/regen_native.sh --rebuild` når parseren er stabil.
4. Kjør `./bin/nc run /tmp/check_short.nor_tmp.no` og `./bin/nc test selfhost/tests/parser_tests.no` igjen.
5. Når alt er grønt, rydd opp i midlertidige filer og skriv eit kort statusavsnitt i hovuddokumentasjonen (`docs/SELFHOST_STATUS.md`/`docs/SELFHOST_HANDLINGSPLAN.md`) om kva som er ferdig og evt. gjenstår.
