# Norscode Treningsguide

Strukturerte treningsmoduler for å lære Norscode systematisk, fra nybegynner til ekspert.

## 📚 Tre treningsnivåer

### 🟢 Level 1: Nybegynnerkurs (6-8 timer)
For alle som er nye til Norscode.

#### Modul 1.1: Grunnleggende (1-2 timer)
- **Installasjon & Setup** → [START_HER.md](./00-getting-started/START_HER.md)
- **Første program** → [STARTAPP.md](./00-getting-started/STARTAPP.md)
- **Kjør eksempler** → [EXAMPLES.md](./00-getting-started/EXAMPLES.md)

**Hands-on:** Skriv et "Hello, World!" program og kjør det.

#### Modul 1.2: Syntaks & Type System (1.5-2 timer)
- **Type System** → [TYPE_SYSTEM.md](./01-language-guide/TYPE_SYSTEM.md)
- **Variables & Functions** → Se EXAMPLES.md
- **Control Flow** → Se COOKBOOK.md

**Hands-on:** Skriv en funksjon som tar inputs og returnerer resultater.

#### Modul 1.3: Collections & Loops (1-1.5 timer)
- **Lists, Dicts, Sets** → Se STDLIB_STATUS.md
- **Loops & Comprehensions** → [COMPREHENSIONS_DESIGN.md](./01-language-guide/COMPREHENSIONS_DESIGN.md)
- **String handling** → Se EXAMPLES.md

**Hands-on:** Lag et program som manipulerer lister og dicts.

#### Modul 1.4: Praktisk programmering (1.5-2 timer)
- **Error handling** → Se QUALITY.md
- **File I/O** → Se COOKBOOK.md
- **Debugging** → Se IDE_AND_LSP.md

**Hands-on:** Lag et program som leser fil og prosesserer data.

#### Modul 1.5: Introduksjon til Packages (1 time)
- **Package Manager** → [REGISTRY_QUICKSTART.md](./03-packages/REGISTRY_QUICKSTART.md)
- **Installer pakker** → [PACKAGE_MANAGER.md](./03-packages/PACKAGE_MANAGER.md)
- **Bruk eksterne pakker** → Se EXAMPLES.md

**Hands-on:** Installer en pakke og bruk den i ditt program.

**Avsluttende prosjekt:** Lag en CLI-app som bruker pakker og filer.

---

### 🟡 Level 2: Intermediate Kurs (10-12 timer)
For de som behersker basics.

#### Modul 2.1: Advanced Type System (1.5-2 timer)
- **Generics** → [GENERICS_DESIGN.md](./01-language-guide/GENERICS_DESIGN.md)
- **Type constraints** → Se TYPE_SYSTEM.md
- **Custom types** → Se EXAMPLES.md

**Hands-on:** Lag en generic data struktur.

#### Modul 2.2: Pattern Matching & Destructuring (1.5-2 timer)
- **Pattern Matching** → [PATTERN_MATCHING_DESIGN.md](./01-language-guide/PATTERN_MATCHING_DESIGN.md)
- **Destructuring** → Se COMPREHENSIONS_DESIGN.md
- **Advanced patterns** → Se COOKBOOK.md

**Hands-on:** Skriv kode som bruker pattern matching intensivt.

#### Modul 2.3: Async & Concurrent Programming (2-2.5 timer)
- **Async/Await** → [ASYNC_DESIGN.md](./01-language-guide/ASYNC_DESIGN.md)
- **Tasks & Futures** → Se ASYNC_DESIGN.md
- **Error handling i async** → Se QUALITY.md

**Hands-on:** Lag et async program som gjør flere ting samtidig.

#### Modul 2.4: Lambda Functions & Functional Programming (1.5 timer)
- **Lambda Design** → [LAMBDA_DESIGN.md](./01-language-guide/LAMBDA_DESIGN.md)
- **map(), filter(), reduce()** → Se EXAMPLES.md
- **Function composition** → Se COOKBOOK.md

**Hands-on:** Skriv funksjonell stil kode med lambdas.

#### Modul 2.5: Design Patterns (2-2.5 timer)
- **Common patterns** → Se [04-how-to-guides](./04-how-to-guides/)
- **Choose 3-4 patterns** basert på dine interesser
- **Audit, Cache, Observable** er gode starter-patterns

**Hands-on:** Implementer 1-2 patterns i et program.

#### Modul 2.6: Publishing Packages (1-1.5 timer)
- **Package structure** → [PACKAGES.md](./03-packages/PACKAGES.md)
- **Publishing** → [PUBLISHING.md](./03-packages/PUBLISHING.md)
- **Registry API** → [REGISTRY_API.md](./03-packages/REGISTRY_API.md)

**Hands-on:** Publiser en enkel pakke til registry.

**Avsluttende prosjekt:** Lag en med-kompleks applikasjon som bruker advanced features, design patterns og egne pakker.

---

### 🔴 Level 3: Expert Kurs (20+ timer)
For de som vil dybde-dybde kunnskap.

#### Modul 3.1: Norscode vs Python (1-1.5 timer)
- **Komplett sammenligning** → [NORSCODE_VS_PYTHON.md](./01-language-guide/NORSCODE_VS_PYTHON.md)
- **Design philosophi** → Se ARCHITECTURE_V2.md
- **Performance características** → Se COMPILER_PIPELINE_ARCHITECTURE.md

**Diskusjon:** Hvilke Norscode features brukes bedre enn Python? Omvendt?

#### Modul 3.2: Architecture & Design (2-2.5 timer)
- **System Architecture** → [NORSCODE_SYSTEMS_ARCHITECTURE.md](./05-development/NORSCODE_SYSTEMS_ARCHITECTURE.md)
- **Runtime** → [RUNTIME_ARCHITECTURE.md](./05-development/RUNTIME_ARCHITECTURE.md)
- **Compiler Architecture** → [COMPILER_PIPELINE_ARCHITECTURE.md](./05-development/COMPILER_PIPELINE_ARCHITECTURE.md)

**Hands-on:** Tegn en arkitektur-diagram av Norscode.

#### Modul 3.3: Compiler Internals (3-4 timer)
- **Compiler Pipeline** → [COMPILER_STRUCTURE.md](./05-development/COMPILER_STRUCTURE.md)
- **Lexer & Parser** → Se TOKEN_FORMAT_V1.md, AST_FORMAT_V1.md
- **IR & Codegen** → [IR_CONTRACT.md](./02-standard-library/IR_CONTRACT.md), [NATIVE_CODEGEN_V2_ABI.md](./05-development/NATIVE_CODEGEN_V2_ABI.md)

**Hands-on:** Skriv et script som analyser Norscode AST.

#### Modul 3.4: Selfhosting Deep Dive (3-4 timer)
- **Selfhost Chain** → [SELFHOST_COMPILER_CHAIN.md](./05-development/SELFHOST_COMPILER_CHAIN.md)
- **Dependency Map** → [SELFHOST_DEPENDENCY_MAP.md](./05-development/SELFHOST_DEPENDENCY_MAP.md)
- **Release Process** → [SELFHOST_RELEASE_CHECKLIST.md](./05-development/SELFHOST_RELEASE_CHECKLIST.md)

**Hands-on:** Bygg Norscode kompilator fra kilde.

#### Modul 3.5: Advanced Performance (2-3 timer)
- **Deterministic Compilation** → [DETERMINISTIC_COMPILER_RULES.md](./05-development/DETERMINISTIC_COMPILER_RULES.md)
- **Code Generation** → [NATIVE_CODEGEN_V2_ABI.md](./05-development/NATIVE_CODEGEN_V2_ABI.md)
- **Profiling & Optimization** → Se QUALITY.md

**Hands-on:** Profil og optimiser et eksisterende program.

#### Modul 3.6: Contributing to Norscode (2-3 timer)
- **Development Setup** → [05-development](./05-development/)
- **Code Quality** → [QUALITY.md](./04-how-to-guides/QUALITY.md)
- **Testing** → Se QUALITY.md
- **Maintenance** → [MAINTENANCE_POLICY.md](./04-how-to-guides/MAINTENANCE_POLICY.md)

**Hands-on:** Send en pull request med en forbedring.

#### Modul 3.7: Advanced Deployment (2-2.5 timer)
- **Deployment Strategies** → [DEPLOYMENT_PLAYBOOK.md](./04-how-to-guides/DEPLOYMENT_PLAYBOOK.md)
- **Observability** → [OBSERVABILITY_PATTERN.md](./04-how-to-guides/OBSERVABILITY_PATTERN.md)
- **Maintenance** → [MAINTENANCE_POLICY.md](./04-how-to-guides/MAINTENANCE_POLICY.md)

**Hands-on:** Sett opp en produksjon Norscode applikasjon.

#### Modul 3.8: LSP & Tooling (1.5-2 timer)
- **IDE Integration** → [IDE_AND_LSP.md](./04-how-to-guides/IDE_AND_LSP.md)
- **Language Server Protocol** → Se IDE_AND_LSP.md
- **Build custom tools** → Se CLI_CONTRACT.md

**Hands-on:** Lag en custom tool som integrerer med Norscode.

**Avsluttende prosjekt:** Bidra med en major feature eller forbedring til Norscode.

---

## 🎓 Treningsstier basert på interesser

### Interessert i WebDevelopment?
- Level 1: Alle moduler
- Level 2: 2.3 (Async), 2.4 (Lambda), 2.5 (Patterns)
- Level 3: 3.1, 3.7

### Interessert i Data Science?
- Level 1: Alle moduler
- Level 2: 2.1 (Generics), 2.4 (Functional), 2.5 (Patterns)
- Level 3: 3.1, 3.5 (Performance)

### Interessert i Compiler/Language Design?
- Level 1: Alle moduler
- Level 2: 2.1 (Types), 2.2 (Pattern matching)
- Level 3: 3.2, 3.3, 3.4, 3.6

### Interessert i DevOps/Infrastructure?
- Level 1: Alle moduler
- Level 2: 2.3 (Async), 2.5 (Patterns), 2.6 (Packages)
- Level 3: 3.7, 3.8

---

## 📊 Estimerte timesummer

| Nivå | Timer | Varighet |
|-----|-------|----------|
| Level 1 (Nybegynner) | 6-8 | 2-3 uker (deltid) |
| Level 2 (Intermediate) | 10-12 | 3-4 uker (deltid) |
| Level 3 (Expert) | 20+ | 6-8 uker (deltid) |
| **Total (Full progression)** | **36-40** | **3-4 måneder** |

---

## ✅ Sjekkliste for leting

### Level 1 Completion
- [ ] Installert Norscode og kjørt første program
- [ ] Forståelse av type system
- [ ] Kan skrive funksjoner og loops
- [ ] Kan arbeide med filer
- [ ] Har installert og brukt en pakke

### Level 2 Completion
- [ ] Forståelse av generics og advanced types
- [ ] Kan bruke pattern matching
- [ ] Kan skrive async kode
- [ ] Kan implementere design patterns
- [ ] Har publisert en pakke

### Level 3 Completion
- [ ] Dybde forståelse av arkitektur
- [ ] Kan bygge Norscode fra kilde
- [ ] Forståelse av compiler pipeline
- [ ] Har bidratt til Norscode prosjektet
- [ ] Kan sette opp produksjon Norscode systemer

---

## 🚀 Neste steg etter training

1. **Lag et reelt prosjekt** - Anvend det du har lært
2. **Bidra til Norscode** - Hjelp med forbedringer
3. **Del kunnskapen** - Skriv blog posts, tutorials
4. **Spesialisering** - Dyk dypere inn i områder som interesserer deg

---

**Sist oppdatert:** 2026-06-14
