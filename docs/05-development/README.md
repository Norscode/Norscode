# Development & Architecture

For personer som bidrar til Norscode eller arbeider med kompilator og internals.

## 🏗️ Arkitektur

Forstå designet av Norscode:

| Dokument | Innhold |
|----------|---------|
| **ARCHITECTURE_V2.md** | Overordnet arkitektur |
| **NORSCODE_SYSTEMS_ARCHITECTURE.md** | System design |
| **RUNTIME_ARCHITECTURE.md** | Runtime design |

## 🔨 Kompilator & Build System

Hvordan Norscode bygger seg selv og oversetter kode:

| Dokument | Innhold |
|----------|---------|
| **COMPILER_PIPELINE_ARCHITECTURE.md** | Pipeline design |
| **COMPILER_STRUCTURE.md** | Komponentoversikt |
| **COMPILER_PIPELINE.md** | Steg-for-steg kompilering |
| **COMPILER_LAYER_RULES.md** | Lag-ordening |
| **COMPILER_FREEZE_POLICY.md** | Stabilitetspolicy |
| **DETERMINISTIC_COMPILER_RULES.md** | Deterministisk kompilering |
| **NATIVE_CODEGEN_V2_ABI.md** | Native code generation |

## 🔄 Selfhosting

Norscode som selv skriver sin egen kompilator:

| Dokument | Innhold |
|----------|---------|
| **SELFHOST_COMPILER_CHAIN.md** | Selfhost pipeline |
| **SELFHOST_DEPENDENCY_MAP.md** | Dependency graph |
| **SELFHOST_CI_GATES.md** | Continuous Integration |
| **SELFHOST_RELEASE_CHECKLIST.md** | Release prosess |
| **SELFHOST_MIGRATION_AND_DEPRECATIONS.md** | Migrasjonsguide |
| **SELFHOST_FALLBACK_CONTRACT.md** | Fallback mekanisme |
| **SELFHOST_LEXER_RUNTIME_ABI.md** | ABI spesifikasjoner |
| **SELFHOST_PARITY_GATES.md** | Parity testing |

## 🔌 API & CLI

Grensesnitt for verktøy og integrasjoner:

| Dokument | Innhold |
|----------|---------|
| **CLI_CONTRACT.md** | Command line interface |
| **API_VERSIONING.md** | API versioning |
| **CONTAINER.md** | Container runtime |

## 🎯 Bidragssti

### Få kompilatoren til å kjøre
1. Clone repo
2. Se ARCHITECTURE_V2.md
3. Les COMPILER_PIPELINE_ARCHITECTURE.md
4. Build og test

### Legg til ny feature
1. Design dokument
2. Implementering
3. Tests
4. Update dokumentasjon

### Bidra til selfhosting
1. Se SELFHOST_COMPILER_CHAIN.md
2. Forstå SELFHOST_DEPENDENCY_MAP.md
3. Kjør SELFHOST_CI_GATES.md

## 📊 Dokumentasjonsstatistikk

- 18 architecture/compiler dokumenter
- Dekker alle lag fra AST til bytecode
- Selfhost prosess fullt dokumentert

## 📞 Kontakt

Spørsmål om internals? Se development guides eller check GitHub issues.
