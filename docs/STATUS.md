# Norscode — Gjeldande status

> Sist oppdatert: juni 2026  
> Kanonisk sannheitsdokument. Erstattar SELFHOST_HANDLINGSPLAN.md, MODENHETSPLAN.md og SELFHOST_STATUS.md der desse er i konflikt.

---

## Kjernetilstand

Norscode er sjølvhosta. Kompilatoren og kjøretida er skrivne i ren Norscode utan C eller Python i normalflata.

```
bash tools/verify_selvstendighet.sh
→ 120/120 testar bestått
→ L1–L6 sjølvstendighetsportar: BESTÅTT
```

| Komponent | Tilstand |
|---|---|
| Lexer (`selfhost/lexer/lexer_m1.no`) | ✅ Stabil |
| Parser (`selfhost/parser.no`) | ✅ Stabil |
| Semantikk (`selfhost/compiler/semantic.no`) | ✅ Stabil |
| IR → Bytecode (`selfhost/compiler/ir_to_bytecode.no`) | ✅ Stabil |
| VM (`selfhost/vm.no`) | ✅ Stabil |
| CLI (`selfhost/main.no`) | ✅ Stabil |
| Maintainer-regen (`tools/maint/regen_native.sh`) | ✅ Grøn |
| Testsuite (native) | ✅ 120/120 |
| Standardbibliotek (`std/`) | 🟡 Breitt, ikkje fullstendig standardisert |
| Typesystem (statisk inferens) | ❌ Ikkje implementert |
| Pakkebehandlar | ❌ Ikkje implementert |
| LSP / editor-støtte | ❌ Ikkje implementert |
| CI (GitHub Actions) | 🟡 Lokal grøn; ekstern CI ikkje bekrefta |

---

## Kva som er avklart

**Resolverfeil i maintainer-regen** — Dette var ein open feil per ein tidlegare versjon av SELFHOST_HANDLINGSPLAN.md. Han er løyst. `bash tools/maint/regen_native.sh --rebuild` køyrer grønt og produserer ein verifisert `dist/norscode_native`.

**Bootstrap og legacy-C** — Bootstrap/C-flata er avvikla frå normalvegen. Ho eksisterer berre som seed-fornyings- og historikklane under `build/maintainer_regen_fixed/`. Ingen aktiv utvikling skjer der.

**54 hoppa testar** — `test_web_*.no` og slow-testar krev live HTTP-server eller lang køyretid. Dei er hoppa over i normal CI-køyring, ikkje feila.

---

## Prioritert veg framover

### 1. Web-testar (54 hoppa)
Startpunkt: legg til ein test-server-wrapper i `tools/nc_test.sh` som startar `norscode_native serve` i bakgrunnen, køyrer web-testane, og stoppar serveren. Ville løfte frå 120/174 til nærare 174/174.

### 2. Standardbibliotek (`std/`)
`std/` er breitt men ikkje standardisert som heilskap:
- Manglande testdekning for fleire modular
- Overlapp og duplikat mellom nokre modular
- Ingen per-modul-dokumentasjon

### 3. CI (GitHub Actions)
Workflow-filene (`ci.yml`) må oppdaterast til å bruke `dist/norscode_native` og `tools/verify_selvstendighet.sh` som einaste port. Ekstern grøn CI er ikkje bekrefta.

### 4. Typesystem
Statisk typesjekking og typeinferens. Blokkerer LSP, betre feilmeldingar og fleire optimeringar.

### 5. LSP
Language Server Protocol for `vscode-norscode/`. Blokkeres av typesystem.

---

## Kva ein skal ignorere

Desse dokumenta inneheld utdatert eller motstridande informasjon og bør ikkje brukast som kjelde:

- `docs/SELFHOST_HANDLINGSPLAN.md` — omtalar ein resolverfeil som er løyst
- `docs/MODENHETSPLAN.md` — delvis utdatert faseinndeling
- `docs/SELFHOST_STATUS.md` — erstatta av dette dokumentet
- `RELEASE_v1.0_SELFHOST.md`, `RELEASE_v1.0_SELFHOST_FINAL.md` — historikk, ikkje status
