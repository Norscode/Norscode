# Self-host plan: Norscode uten Python og C

Mål:
Gjøre Norscode gradvis selvstendig slik at normal utvikling, bygging og kjøring ikke er avhengig av Python eller C-backend.

Dette dokumentet er omgang 1: analyse og migreringsplan. Det endrer ikke compiler-koden ennå.

## Nåværende situasjon

Repoet har allerede en sterk binary-first retning:

- `norcode` er primær CLI.
- `nc`, `nl`, `nor` og `norsklang` er legacy-/kompatibilitetsalias.
- Python er dokumentert som eksplisitt bootstrap-/utviklerverktøy.
- C-backend finnes fortsatt som viktig compiler-retning.
- Det ligger fortsatt igjen bootstrap- og orchestration-logikk i Python-laget, men mye av CLI-en er allerede flyttet til modulære kommandoer.
- `norcode/cli.py` bruker nå modulær registry-dispatch, mens `norcode/legacy_main.py` og `norcode/bootstrap/python_entry.py` bærer den gjenværende Python-kompatibiliteten.

Det betyr at prosjektet er selvstendig i brukeropplevelse, men ikke fullt selvstendig i intern compiler/runtime-arkitektur.

## Endelig mål

Ønsket sluttstatus:

```text
Norscode-kilde (.no)
        ↓
Norscode lexer
        ↓
Norscode parser
        ↓
Norscode semantic checker
        ↓
Norscode bytecode generator
        ↓
Norscode VM/runtime
        ↓
Program kjører uten Python og uten C-backend som normalvei
```

Python skal bare beholdes som historisk bootstrap til den kan fjernes helt.
C-backend kan beholdes midlertidig som legacy/eksperimentell backend, men ikke som normal kjørevei.

## Viktig prinsipp

Vi fjerner ikke Python og C brått.
Først lager vi en ny selvstendig kjede ved siden av den gamle.
Når den nye kjeden består samme tester, bytter vi normalvei.
Deretter kan gammel Python-/C-avhengighet fases ut.

## Fase 1 — Kartlegging og grenseflater

Leveranse:

- [ ] Dokumenter hvilke deler som i dag er Python-avhengige.
- [ ] Dokumenter hvilke deler som i dag er C-backend-avhengige.
- [ ] Definer stabilt bytecode-format.
- [ ] Definer minimum AST-format som Norscode-compiler må kunne produsere.
- [ ] Definer minste selfhost-test: compiler kan kompilere et lite `.no`-program.

Ferdig når:

- [ ] Vi vet nøyaktig hvilke filer/moduler som må flyttes først.
- [ ] Det finnes en tydelig test som sier om selfhost-kjeden virker.

## Fase 2 — Skill ut compiler-kjernen fra `main.py`

Leveranse:

- [ ] Flytt CLI-kommandoer ut av `main.py`.
- [ ] Flytt build/release-verktøy ut av `main.py`.
- [ ] Flytt serverkommandoer ut av `main.py`.
- [ ] Behold bare den nødvendige bootstrap-kompatibiliteten i Python-laget.
- [ ] Gjør modulær CLI-dispatch til standardvei for nye kommandoer.

Ferdig når:

- [ ] Compiler-kjernen kan importeres uten CLI-sideeffekter.
- [ ] CLI kan testes separat fra compiler/runtime.

## Fase 3 — Bytecode som hovedmål

Leveranse:

- [ ] Gjør bytecode-backend til primær normalvei.
- [ ] Dokumenter bytecode-instruksjoner og stack-regler.
- [ ] Legg til snapshot-tester for bytecode.
- [ ] Sørg for at C-backend ikke kreves for vanlige `run`, `test` og `check`.

Ferdig når:

- [ ] Små programmer kjører via bytecode/VM uten C-kompilering.
- [ ] CI kan validere bytecode-output deterministisk.

## Fase 4 — Lexer i Norscode

Leveranse:

- [ ] Implementer første Norscode-lexer i Norscode.
- [ ] Den må støtte kommentarer, tekst, tall, navn, operatorer og norske nøkkelord.
- [ ] Lag parity-test mot eksisterende Python-lexer.

Ferdig når:

- [ ] Samme input gir samme tokenliste i Python-lexer og Norscode-lexer for kjerneeksempler.

## Fase 5 — Parser i Norscode

Leveranse:

- [ ] Implementer parser for minimum syntax-sett.
- [ ] Start med funksjoner, variabler, uttrykk, `hvis`, `mens`, `returner` og kall.
- [ ] Lag AST-json eller tilsvarende stabil representasjon.
- [ ] Lag parity-test mot eksisterende Python-parser.

Ferdig når:

- [ ] Norscode-parser kan parse små programmer og produsere forventet AST.

## Fase 6 — Semantic checker i Norscode

Leveranse:

- [ ] Typekontroll for heltall, tekst, bool og enkle lister.
- [ ] Funksjonssignaturer og returtyper.
- [ ] Navneoppslag og lokale scopes.
- [ ] Feilmeldinger som matcher dagens hovedmønster.

Ferdig når:

- [ ] Ugyldige programmer feiler kontrollert før runtime.

## Fase 7 — Bytecode generator i Norscode

Leveranse:

- [ ] Generer bytecode fra AST.
- [ ] Start med aritmetikk, variabler, if/while, funksjonskall og return.
- [ ] Snapshot-test bytecode-output.

Ferdig når:

- [ ] Norscode-compiler skrevet i Norscode kan produsere kjørbar bytecode for små programmer.

## Fase 8 — Selfhost-kjede

Leveranse:

- [ ] Gammel compiler bygger ny compiler.
- [ ] Ny compiler bygger testprogrammer.
- [ ] Ny compiler bygger deler av seg selv.
- [ ] Legg til `norcode selfhost-check` som fast kommando.

Ferdig når:

- [ ] Selfhost-kjeden kan kjøres i CI.
- [ ] Resultatet er deterministisk.

## Fase 9 — Fjern Python fra normal intern flyt

Leveranse:

- [ ] Normal `norcode run` bruker ikke Python compiler-path.
- [ ] Normal `norcode test` bruker ikke Python compiler-path.
- [ ] Python compiler beholdes kun som `bootstrap` eller `legacy`.

Ferdig når:

- [ ] CI feiler hvis normal CLI bruker Python-fallback.

## Fase 10 — Fjern C-backend fra normal flyt

Leveranse:

- [ ] C-backend flyttes til legacy/experimental.
- [ ] Bytecode/VM er default.
- [ ] Dokumentasjon oppdateres fra “kompilerer til C” til “kjører på Norscode VM/bytecode”.

Ferdig når:

- [ ] Nye brukere kan bygge og kjøre Norscode uten C-toolchain.

## Fase 11 — Full opprydding

Leveranse:

- [ ] Fjern døde Python-stier.
- [ ] Fjern ubrukte C-backend-krav fra normal installasjon.
- [ ] Oppdater README, START_HER, CLI_CONTRACT, QUALITY og HANDOFF.
- [ ] Lag migreringsnotat for brukere.

Ferdig når:

- [ ] Prosjektet er self-hosted i normal utvikling.
- [ ] Python og C er ikke nødvendige for vanlig bruk eller vanlig bygging.

## Første konkrete kodeoppgave etter denne planen

Neste omgang bør være:

```text
Omgang 2: Fortsette å flytte gjenværende bootstrap- og runtime-ansvar ut av Python-laget.
```

Foreslått ny struktur:

```text
norcode/
  cli.py
  commands/
    run.py
    check.py
    test.py
    build.py
    serve.py
    release.py
    selfhost.py
  bootstrap/
    python_entry.py
compiler/
  lexer.py
  parser.py
  semantic.py
  bytecode_backend.py
  cgen.py
runtime/
  vm.py
  web.py
  stdlib.py
```

Dette må gjøres videre før vi flytter lexer/parser-arbeidet enda lenger over i Norscode, ellers blir selfhost-arbeidet vanskelig å teste trygt.

## Risikoer

- For tidlig fjerning av Python kan ødelegge bootstrap.
- For tidlig fjerning av C kan ødelegge eksisterende brukerflyt.
- Selfhost kan bli ustabil hvis bytecode-formatet ikke låses først.
- Den gjenværende Python-bootstrapen er et sentralt risikopunkt fordi for mye ansvar fortsatt kan samle seg der.

## Beslutning

Vi velger trygg migrering:

1. Rydd arkitektur.
2. Gjør bytecode/VM til hovedmål.
3. Flytt compiler-delene én og én til Norscode.
4. Kjør parity-tester hele veien.
5. Bytt default først når selfhost-kjeden er stabil.
