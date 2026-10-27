# Norscode .no – eksempler for språkgjenkjenning

Dette er testgrunnlag for framtidig GitHub Linguist-integrasjon, **ikke** en integrasjon med Linguist sin klassifiserer. Kjør positive og negative tilfeller mot faktiske Linguist-heuristikker etter upstream-implementasjon.

## Positive (.no-kildekode)

`compiler/compiler.no`: `pakke compiler`, `bruk std.runtime`, `struktur CompilerRuntimeState {`, `funksjon init_runtime() -> CompilerRuntimeState {`, `returner CompilerRuntimeState {`.

`std/tomllib.no`: `funksjon _trim(s: tekst) -> tekst {`, `la teikn = builtin.split(s, "")`, `mens (start < n og ...)`.

`std/ghash.no`: `funksjon _rshift1(v: liste_heiltall) -> liste_heiltall {`, `la carry = 0`, `returner ut`.

`toolchain/norspkg/cache.no`: `funksjon cache_lag()`, `returner {`, `slutt`.

Alle disse er faktisk observert i de oppgitte kildefilene. Ingen kildekode er kopiert inn som egen `.no`-fixture.

## Negative (vanlige .no-tekstfiler, forslag til testvektorer)

- Norske tekstlinjer: `Dette er en vanlig tekstfil på norsk.`
- Overskrifter og lenker: `# Kontaktinformasjon` og `https://eksempel.no`
- Prosa med isolert ord: `En funksjon brukes for å beskrive formål.`
- Prosa med ordet returner: `Vennligst returner varen innen fristen.`
- Tekst som inneholder `pakke` eller `bruk` uten kodekonstruksjoner.

## Akseptansekriterier

1. Ingen klassifisering som Norscode på negative tilfeller.
2. Positive eksempler klassifiseres som Norscode.
3. Enkeltstående norske ord er ikke tilstrekkelige; krev kombinasjoner av kildekode-signaler som `funksjon <identifikator>(`, `returner` og blokksyntaks.
4. Ingen endring i eksisterende `Text`-deteksjon for vanlige `.no`-filer.
5. Upstream Linguist-testene må passere, og testdekningen må vise både positive og negative tilfeller.

**Status:** Testvektorer og akseptansekriterier dokumentert, **ikke kjørt**.
