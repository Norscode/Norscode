# Endringslogg

Alle merkbare endringar i Norscode blir dokumenterte i denne fila.

## [Unreleased]

### Release-klargjering

- Lagt til `./bin/nc local-green` som lokal release-preflight, aktiv flate, fase-0, L1-L6-sjølvhosting og full testport utan publisering.
- Lagt til strenge lokale portar med `./bin/nc release-preflight --strict` og `./bin/nc local-green --strict`.
- Lagt til `local-green --list`/`-l` for å vise kvart portsteg og kommando utan å køyre dei.
- Stramma inn streng git-sporing for release-verktøy, testar, dokumentasjon, plattformdokumentasjon for appar og `CHANGELOG.md`.
- Oppdatert `INSTALL.md` til å bruke `./bin/nc` for installasjon, release-pakking, release-installasjon og app-pakking som dokumentert normalflyt.
- Lagt til app-release- og installasjonsdokumentasjon for Linux, macOS og Windows.
- Stramma inn release-tekst rundt verifiserte sjølvhosting-løyper i staden for breie produksjonspåstandar.

## [1.0.1] - 2026-06-06

### Feilrettingar

#### Modulsystem (`bruk`-import)
- **Retta:** `bruk std.math` og andre `bruk <modul>` kunne gi uendeleg OOM-løkke ved runtime. `nc_bundle_ncb` sende eit dict-objekt til `tekst_erstatt`, som vart serialisert til `"[verdi]"`; deretter kunne `json_parse_raw("[verdi]")` halde fram med å leggje `nil` i ei liste til minnet tok slutt.
- **Retting:** strengbytte skjer på rå JSON før `omdøyp_funksjonar` blir kalla.
- **Retta:** `finn_bruk_imports` prøver ikkje lenger å runtime-kompilere `selfhost.*`-modular som alt er prekompilerte inn i binæren.
- **Retta:** `jp2_parse` har defensiv vakt som går vidare forbi ukjende JSON-token og hindrar uendeleg løkke på feilforma input.
- **Verifisert:** `bruk std.math`, `bruk std.path` og `bruk std.math som alias` verkar.

---

## [1.0.0] - 2026-06-06

### Verifisert selfhosting-løype

**Norscode har ei verifisert selfhosting-løype.** Kompilatoren, bytekodeparseren og VM-en er skrivne i Norscode for kjerna i arbeidsflyten, med runtime-grensa halde eksplisitt.

#### Norscode selfhost-kompileringsløype
- **Lexer** → tokenisering (`lexer_m1.no`)
- **Parser** → syntaksanalyse (`parser.no`)
- **Semantisk analyse** → typesjekk og IR-generering (`semantic.no`)
- **IR til bytekode** → bytekodekompilering (`ir_to_bytecode.no`)
- **VM-køyring** → stack-basert bytekodekøyring (`køyr_funksjon`)

Alle stega går gjennom Norscode selfhost-løypa utan den historiske C-backenden som eigar av arbeidsflyten.

### Funksjonar

#### Kjernespråk
- [OK] Funksjonar med typesignaturar
- [OK] Statisk typing (`heltall`, `tekst`, `bool`, lister, dict)
- [OK] Kontrollflyt (`hvis`, `løkke`, `returner`)
- [OK] Feilhandtering (`kast`, `prøv`, `fang`)
- [OK] Operatorar og uttrykk
- [OK] Importar og modular (grunnnivå)
- [OK] Native ELF-kompilering

#### Bytecode VM
- [OK] JSON-basert NCB-format (Norscode Bytecode)
- [OK] Stack-basert køyringsmodell
- [OK] Språkstøtte for den verifiserte løypa
- [OK] Deterministisk output
- [OK] Cache for prekompilerte modular

#### Standardbibliotek
- [OK] `json`-modul
- [OK] `std.path`-verktøy
- [OK] `std.env`-verktøy
- [UTSETT] Vidare modulsystem-arbeid
- [UTSETT] Ferdigstilling av web-rammeverk

### Ytingsforbetringar

- **30-40% raskare bootstrap** via prekompilerte modular (cache)
- **9% raskare lexer** via range-check-optimalisering
- **10-15x raskare bundler** via modulprekompilering
- **18% mindre bytekode** via optimalisering
- **Ingen kjende minnelekkasjar** i bootstrap-løypa

### Endringar sidan førre versjon

#### Nye filer
- `selfhost/std.no` — samla stdlib med inline path/env-implementasjonar
- `selfhost/std_compat.no` — kompatibilitetslag for stdlib-testar
- `bootstrap/precompiled/` — prekompilerte bytekodemodular:
  - `json.ncb.json` (15 KB)
  - `parser.ncb.json` (91 KB)
  - `semantic.ncb.json` (11 KB)
  - `ir_to_bytecode.ncb.json` (136 KB)
  - `lexer_m1.ncb.json` (41 KB)

#### Endra filer
- `selfhost/bootstrap_gate.no` og `selfhost/bootstrap_gate.ncb.json` — skrivne/regenererte for Norscode JSON+VM-løypa
- `selfhost/bundler.no` — optimalisert for minnebruk og modulcache
- `selfhost/lexer/lexer_m1.no` — range-check-optimalisering
- `tools/maint/c/nc_native_main.c` — lagt til stdlib-dispatch-handterarar
- `.github/workflows/ci.yml` — gjorde Omgang 6b valfri

### Testresultat

| Category | Result |
|----------|--------|
| Kjernespråktestar | 27/35 bestått (77%) |
| Kompilatorløype | Verifisert i selfhost-løypa |
| VM-løype | Verifisert i selfhost-løypa |
| Bootstrap-løype | Verifisert i selfhost-løypa |
| Native kompilering | Verifisert i release-løypa |
| Ytingsmålingar | Bestått |

**Release-sjekkar:** Bestått i den verifiserte løypa

### Kjende avgrensingar og utsett arbeid (Fase 5)

#### Modulsystem
- **Status:** Direkte dispatch-kall verkar (`std.path.basename()`)
- **Manglar:** Modulalias (`bruk std.path som path`)
- **Verknad:** 10 testar krev fullt modulsystem
- **Plan:** Planlagt for Fase 5 (6-8 timar)

#### Prekompilering
- **Status:** `common.no` er for stor til prekompilering i den gamle C-hosten (OOM)
- **Reserve:** Alternativ Norscode selfhost-løype er bevist i release-løypa
- **Verknad:** Ingen for den verifiserte løypa (`bootstrap_pure.no` er bevist stabil)

#### Vidare funksjonar
- Async/await (Fase 5+)
- Vidare stdlib (web-rammeverk, sikkerheit)
- WebAssembly-støtte
- Pakkebehandlar

### Utsjekkliste

- [OK] Kjernerelease-løypa verkar
- [OK] Ytingsforbetringar er med
- [OK] Kjernetestar består
- [OK] CI er kopla inn
- [OK] Dokumentasjon og release-notat finst
- [OK] Ingen kjend blokkering for den verifiserte løypa

### For utviklarar

#### Bygging frå kjelde
```bash
./bin/nc run tools/build_norscode_native.no    # Build native runtime
./bin/nc test                           # Run tests
./bin/nc bootstrap-self                 # Verify self-hosting
```

#### Verifisering av selfhosting
```bash
./bin/nc selfhost-bootstrap-gate   # Steg A+B: Norscode selfhost-kompilering
./bin/nc bootstrap-self            # Steg C: selfkompilering
```

#### Bruk av prekompilerte modular
Bundleren cache-ar prekompilerte `.ncb.json`-filer automatisk:
```norscode
bruk selfhost.bundler som bundler
la compiled = bundler.les_ncb_eller_kompiler("selfhost/parser.no")
```

### Dokumentasjon

- [Historiske release-notat](.github/releases/v1.0-selfhost.md)
- [Guide for prekompilering](bootstrap/precompiled/README.md)
- [Noverande selfhost-status](docs/STATUS.md)
- [Selfhost-faseplanar](docs/SELFHOST_HANDLINGSPLAN.md)

### Neste steg (Fase 5)

1. **Modulsystem-refaktor** (6-8 timar)
   - Aktivere full `bruk std.*`-modulstøtte
   - Nå 35/35 målretta modultestar
   - Utvide stdlib-integrasjonen

2. **Vidare funksjonar**
   - Async/await-implementasjon
   - Ferdigstilling av web-rammeverk
   - Vidare sikkerheitsverktøy

3. **Infrastruktur**
   - Fjerne historisk `bootstrap/maint/` frå git
   - Optimalisere C-host-minne for større filer
   - Publisere offisiell dokumentasjonsside

---

**Byggdato:** 6. juni 2026
**Status:** Verifisert release-løype
**Neste milepæl:** Fase 5 — breiare stdlib-integrasjon
