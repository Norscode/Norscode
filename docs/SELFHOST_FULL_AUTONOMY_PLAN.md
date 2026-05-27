# Norscode — plan for full selvstendighet

Mål:
Gjøre Norscode helt selvstendig i normal utvikling, bygging, testing, kjøring og publisering.

Med "helt selvstendig" mener vi at vanlig bruk ikke er avhengig av Python eller C-backend, at selfhost-kjeden er den vanlige veien, og at all legacy-støtte er eksplisitt, liten og lett å slå av.

Denne planen er en samlet masterplan. Den samler arbeid som allerede er fordelt på fase- og roadmap-dokumenter, og viser hvilken rekkefølge som gir minst risiko.

## Hva som regnes som ferdig

Norscode er helt selvstendig når:

- `./bin/nc run`, `test`, `check`, `build` og release-flyt bruker native/selfhost som normalvei
- Python finnes kun som tydelig, eksplisitt legacy- eller bootstrap-støtte
- C-backend finnes ikke lenger som nødvendig del av normal produktflyt
- CI verifiserer bootstrap, selfhost, release og regresjoner uten skjulte bakdører
- nye bidragsytere kan se normalvei, fallback og migreringshistorikk uten å lese kildekoden først

## Hovedprinsipp

Vi fjerner ikke alt på én gang.
Vi bygger en stabil selvstendig kjede ved siden av den gamle, låser den med test og CI, og faser deretter ut gamle veier når de ikke lenger trengs.

Ny produktlogikk skal i størst mulig grad skrives direkte i Norscode, ikke i Python.
Python er kun lov som tynn bootstrap, eksplisitt legacy-støtte eller midlertidig verifiseringslag mens selfhost-kjeden fortsatt fullføres.
Alt som fortsatt må ligge i Python skal være tydelig merket som overgangskode og ha en konkret plan for utfasing.

## Omgangsregel

Én omgang skal være én sammenhengende og verifiserbar batch som kan fullføres i samme svar.

Det betyr i praksis:

- én tydelig kommando-familie eller én liten, sammenhengende kodeflatenhet per omgang
- kodeflytt, importopprydding og planoppdatering i samme batch
- lokal verifisering i samme omgang, helst med `py_compile` eller tilsvarende
- ingen blanding av flere store arkitekturspor hvis de ikke kan stå ferdig sammen

Hvis en endring krever ny design, ny hjelpeflate og flere uavhengige filer, er den for stor for én omgang og skal deles opp.

## Plan i omganger

### Omgang 0 — Lås fase 0

Mål:
Gjøre CI, bootstrap og dokumentasjon konsistente nok til at resten av reisen kan foregå uten uklare blokker.

Leveranser:

- [x] `bin/nc` og `bin/bootstrap` peker tydelig mot native-first flyt
- [x] Legacy Python-fallback er eksplisitt navngitt
- [x] CI har egen `phase0-verify`-gate
- [x] Fase-0-status er dokumentert som lokalt verifisert
- [ ] Ekstern grønn-bekreftelse i GitHub Actions er observert og notert

Ferdig når:

- [ ] Fase 0 kan regnes som faktisk ferdig, ikke bare lokalt verifisert

### Omgang 1 — Skill runtime og CLI fra bootstrap

Mål:
Fjerne den verste blandingen av produktlogikk og bootstrap-logikk.

Leveranser:

- [x] Første high-traffic kommandoer (`ci` og `selfhost-bootstrap-gate`) er flyttet til modulær CLI-dispatch
- [x] Selfhost parity-familien (`selfhost-parity*` og fixture-kommandoer) er flyttet til modulær CLI-dispatch
- [x] Standard `norcode`-inngang bruker modulær CLI-dispatch først
- [x] Legacy bootstrap-flaten sender modulære kommandoer rett til samme dispatcher
- [x] Legacy duplicate branches for de migrerte kommandoene er fjernet
- [x] Legacy duplicate branches for `diagnose`, `doctor` og `serve` er fjernet
- [x] Redundant fallback-dispatch i `legacy_main.py` er fjernet
- [x] Package-registry-flaten (`add`, `lock`, `update`, `registry-*`) er flyttet til modulær CLI-dispatch
- [x] `bytecode-build` er flyttet til modulær CLI-dispatch
- [x] Legacy duplicate branches for package-registry og `bytecode-build` er fjernet
- [x] Inspeksjonskommandoene `debug`, `disasm`, `ir-disasm` og `update-snapshots` er flyttet til modulær CLI-dispatch
- [x] Selfhost-bridge og NCB-kommandoene `selfhost-ast-export`, `ast-export`, `selfhost-chain-export`, `selfhost-chain-check`, `selfhost-ncb-export`, `selfhost-ncb-run` og `selfhost-ncb-build-cache` er flyttet til modulær CLI-dispatch
- [x] Legacy duplicate branches for inspeksjons- og selfhost-bridge-kommandoene er fjernet
- [x] Bootstrap/runtime-kommandoene `bootstrap-compiler-verify`, `registry-host`, `selfhost-chain-run` og `selfhost-compile-all` er flyttet til modulær CLI-dispatch
- [x] Byggekommandoene `build`, `native-build` og `native-run` er flyttet til modulær CLI-dispatch
- [x] Meta-kommandoene `repl` og `commands` er flyttet til modulær CLI-dispatch
- [x] Legacy parseroppsett er forenklet til automatisk registry-basert registrering av modulære kommandoer
- [x] `legacy_main.py` og `cli.py` deler nå samme parserbygger i `cli_parser.py`
- [x] Normal `cli.py` bruker nå registry-dispatch direkte uten bootstrap-fallback
- [x] Stale bootstrap-era docstrings i `command_dispatch.py`, `commands/base.py`, `commands/registry.py` og `bootstrap/python_entry.py` er oppdatert til modulær CLI-virkelighet
- [x] `CLI_CONTRACT.md` og `PYTHON_UTFASING.md` er oppdatert til å beskrive den modulære CLI-en og den eksplisitte Python-kompatibiliteten
- [x] `SELFHOST_BOOTSTRAP_INVENTORY.md` er oppdatert til å skille klarere mellom modulær CLI, `main.py` og den eksplisitte Python-bootstrapen
- [x] `SELFHOST_BOOTSTRAP_INVENTORY.md` er oppdatert til å nevne `norcode/cli.py` og `norcode/bootstrap/python_entry.py` som del av den eksplisitte bootstrap-flaten
- [x] `PHASE8_PYTHON_FALLBACK.md` er oppdatert til å bruke `--legacy-python-fallback` som eksplisitt fallback-vei i eksempler og regler
- [x] `SELFHOST_FALLBACK_CONTRACT.md` og `SELFHOST_MIGRATION_AND_DEPRECATIONS.md` er oppdatert til å beskrive `norcode/cli.py` som normal vei og `legacy_main.py` / `bootstrap/python_entry.py` som eksplisitt kompatibilitet
- [x] `SELFSTENDIG_NORSCODE_ROADMAP.md` er oppdatert til å omtale den gjenværende Python-bootstrapen i stedet for bare `main.py`
- [x] README-filstrukturen er oppdatert til å vise repo-rot uten `main.py` som normal strukturdel
- [x] `SELFHOST_STATUS.md` beskriver nå den gjenværende bootstrap-kompatibiliteten eksplisitt i `legacy_main.py` og `bootstrap/python_entry.py`
- [x] `SELFHOST_BOOTSTRAP_INVENTORY.md` er oppdatert til å markere `norcode/cli.py` som normal vei og `main.py` som historisk wrapper
- [x] `SELFHOST_REMAINING_ROADMAP.md` er oppdatert til å beskrive `norcode/cli.py` som normal modulær vei og Python-flaten som eksplisitt kompatibilitet
- [x] `SELFHOST_DEPENDENCY_MAP.md` er oppdatert til å si at `norcode/cli.py` er normal CLI-vei og at Python-flaten er eksplisitt bootstrap-kompatibilitet
- [x] `SELFSTENDIG_NORSCODE_ROADMAP.md` er oppdatert til å si at den normale CLI-veien er modulær i `norcode/cli.py`
- [x] README fallback-avsnittet er oppdatert til å peke eksplisitt på `legacy_main.py` og `bootstrap/python_entry.py`
- [x] `SELFHOST_STATUS.md` er oppdatert til å si at `norcode/cli.py` er normal modulær vei og at `legacy_main.py` / `bootstrap/python_entry.py` er eksplisitt kompatibilitet
- [x] `SELFHOST_CI_GATES.md` og `SELFHOST_RELEASE_CHECKLIST.md` er oppdatert til å si at fallback-lane og release-smoke går via den gjenværende Python-kompatibiliteten
- [x] `SELFHOST_DIAGNOSTICS.md` er oppdatert til å si at `norcode diagnose` er normal modulær CLI og at `doctor` er eksplisitt verifikasjonsflate
- [x] README-åpningen er oppdatert til å beskrive native backend, modulær CLI og eksplisitt bootstrap-kompatibilitet
- [x] `SELFHOST_MIGRATION_AND_DEPRECATIONS.md` er oppdatert til å liste `legacy_main.py` og `bootstrap/python_entry.py` som del av bootstrap-kompatibiliteten
- [x] Aktive wrappers og installasjons-/release-helpers er flyttet fra direkte `main.py`-kall til `python -m norcode.legacy_main`
- Splitt `main.py`/legacy-entrypoints i tynn bootstrap og tydelige moduler
- Gjør CLI-kommandoer testbare uten å starte hele bootstrap-laget
- Flytt build-, test-, serve- og release-orchestration til egne moduler
- Behold Python kun som overgangsbane, ikke som skjult standard

Ferdig når:

- CLI og runtime kan testes uavhengig av bootstrap
- nye kommandoer ikke må legges inn i et monolittisk bootstrap-lag

### Omgang 2 — Fullfør selfhost-kjeden

Mål:
La Norscode-kompilatoren bygge Norscode-kode uten å være avhengig av Python-orakler i normalflyt.

Leveranser:

- Stabil parser- og AST-kontrakt
- Stabil semantic- og IR-kontrakt
- Stabil bytecode-/VM-kontrakt
- Selfhost parity-tester for kjerneeksempler
- Kommandoer som verifiserer selfhost ende-til-ende i CI

Ferdig når:

- `selfhost/main.no` kan kompilere kjernepipelinen deterministisk
- selfhost og bootstrap følger samme kontrakter for de viktige tilfellene

### Omgang 3 — Få Python helt ut av normal flyt

Mål:
Gå fra "Python som eksplisitt fallback" til "Python som ren legacy".

Leveranser:

- Normal `run`, `test`, `check` og `build` bruker ikke Python-path
- CI stopper om normalvei forsøker å bruke Python-fallback
- Alle brukerrettede docs beskriver native/selfhost først
- Legacy-stier er tydelig merket og samlet

Ferdig når:

- Vanlig utvikling kan foregå uten at Python trengs
- Python er kun nødvendig for uttrykkelig legacy eller historisk reproduksjon

### Omgang 4 — Fjern C-backend fra normal flyt

Mål:
Gjøre C til noe som eventuelt kan ligge igjen som legacy eller eksperiment, men ikke som krav.

Leveranser:

- Bytecode/VM er normal kjørevei
- C-backend blir flyttet ut av normal installasjon og normal docs
- Release og installasjon krever ikke C-toolchain
- Eventuelle C-spor får en egen legacy-policy

Ferdig når:

- nye brukere kan bygge og kjøre Norscode uten C-verktøykjede

### Omgang 5 — Rydd historikk og deprecations

Mål:
Fjerne resten av ballast som bare finnes for å forklare en tidligere verden.

Leveranser:

- Historiske dokumenter er tydelig arkiv og ikke normal referanse
- Gjenstående aliaser, kompatibilitetsveier og gamle navn er samlet på ett sted
- README, START_HER, HANDOFF, CLI_CONTRACT og roadmap peker alle samme vei
- Migrering og deprecation er dokumentert med tydelige sluttpunkter

Ferdig når:

- nye bidragsytere møter en enkel og sannferdig dokumentasjonsflate

### Omgang 6 — Lås vedlikehold og drift

Mål:
Sørge for at selvstendigheten holder seg selv ved like.

Leveranser:

- Verifikatorer for bootstrap, selfhost og release
- Regresjonsvern i CI
- Diagnostikk og feilmeldinger som peker mot riktig lag
- Enkle rutiner for å oppdage når legacy begynner å vokse igjen

Ferdig når:

- endringer som gjeninnfører skjult avhengighet blir stoppet tidlig

## Foreslått rekkefølge

1. Lås fase 0 fullt ut
2. Splitt bootstrap og produktlogikk
3. Fullfør selfhost-kjeden
4. Fjern Python fra normal flyt
5. Fjern C-backend fra normal flyt
6. Rydd historikk, docs og drift

## Sannhetsregel

Hvis en endring gjør at en ny bidragsyter må vite "hvilken skjult vei er riktig i dag", er vi ikke ferdige ennå.
Hvis en endring gjør normal vei kortere og legacy tydeligere, går vi i riktig retning.

## Les videre

- [ROADMAP.md](../ROADMAP.md)
- [docs/SELFHOST_PHASE0_REMAINING_ROUNDS.md](SELFHOST_PHASE0_REMAINING_ROUNDS.md)
- [docs/SELFHOST_NO_PYTHON_NO_C_PLAN.md](SELFHOST_NO_PYTHON_NO_C_PLAN.md)
- [docs/SELFHOST_REMAINING_ROADMAP.md](SELFHOST_REMAINING_ROADMAP.md)
- [docs/SELFHOST_STATUS.md](SELFHOST_STATUS.md)
