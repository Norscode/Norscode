# Norscode

Norscode er eit norsk språk- og verktøysett med native-first CLI, selfhost-løype og ei aktiv flate utan Python eller C.

![Norscode-oversikt](docs/assets/norscode-overview.svg)

![Norscode-logo](frontend/assets/icons/norscode-logo-dark.svg)

## Kort fortalt

- Norsk syntaks for funksjonar, kontrollflyt og uttrykk
- Statisk typing for heiltal, tekst, bool, lister og ordbøker
- Modul- og pakke-system
- Standardbiblioteket `std`
- `std.ai` for lokal AI-kontrakt: chat, embeddings, moderering, verktøy og agent-plan
- `std.runtime_status` for testbar runtime v1-status og prioriterte sjølvstendigheitsblokkerar
- Feilhandtering med `kast`, `prøv` og `fang`
- Normal kjede: `.no` -> NCB JSON -> `selfhost/vm.no`

## Kom i gang

1. Les [dokumentasjonsinngangen](docs/INDEX.md)
2. Installer med [installasjonsguiden](INSTALL.md)
3. Bruk [brukarmanualen](docs/USER_MANUAL.md) som praktisk manual
4. Følg [opplæringsguiden](docs/LEARNING_GUIDE.md) for opplæring
5. Bruk [løypekartet](docs/LANE_MAP.md) for rett arbeidsløype
6. Les [selfhost-handlingsplanen](docs/SELFHOST_HANDLINGSPLAN.md) for selfhost-løypa
7. Sjekk [statussida](docs/STATUS.md)
8. Vedlikehald: `./bin/nc maintenance verify`

Snarvegar:

```bash
./bin/nc --help
./bin/nc run app.no
./bin/nc check app.no
./bin/nc feature-check app.no
./bin/nc maintenance maturity
./bin/nc test
```

## Normal bruk

- `./bin/nc` og `dist/norscode_native` er normal CLI og runtime
- `./bin/nc run <fil.no>` køyrer også vanlege `bruk`/`importer`-program med bundla imports
- `./bin/nc feature-check [fil.no ...]` er standard gate for å byggje nye funksjonar direkte i Norscode
- `./bin/nc repl [uttrykk]` evaluerer små uttrykk direkte eller startar interaktiv REPL
  - Døme: `./bin/nc repl '1 + 2'`
- `./bin/nc release-preflight` sjekkar release-/GitHub-klargjering lokalt utan å publisere
- `./bin/nc release-preflight --strict` er siste lokale port før GitHub/release og feilar på uspora nøkkelfiler
  - Når denne porten er grøn, er regelen at nye nøkkelfiler skal med til GitHub.
- `./bin/nc local-green` køyrer lokal release-preflight, aktiv-flate-sjekk, fase-0, L1-L6-sjølvstendighet og full testflate utan å publisere
- `./bin/nc local-green --strict` køyrer same grønnliste med streng release-preflight som fyrste steg
- `./bin/nc local-green --strict --list` eller `./bin/nc local-green --strict -l` viser streng grønnliste med steg og kommandoar utan å køyre henne
- `./bin/nc local-green --list` viser grønnliste-stega med kommandoar utan å køyre dei
- `./bin/nc local-green -l` er kortform for `--list`
- `./bin/nc local-green --help` viser kort brukshjelp utan å køyre grønnlista
- `./bin/nc local-green -h` er kortform for `--help`
- samla lokal grønnliste: release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate
- `./bin/nc stage0-release-assets --platform <plattform>` byggjer stage-0 release-ELF og flyttbar `.sha256` under `release-artifacts/stage0/`
- `./bin/nc maintenance verify` gir vedlikehaldssamandrag
- `./bin/nc maintenance maturity` sjekkar 10/10-bevisflata
- `./bin/nc maintenance status`, `lane`, `seed`, `seed-status` viser Norscode-vedlikehald og stage-0-status
- `./bin/nc maintenance report-json` gir maskinlesbar statusrapport (inkl. `stage0_seed_ok`)
- `./bin/nc selvstendighet` verifiserer normalflata utan C-regen eller stage-0 rebuild
- `./bin/nc verify-selvstendighet` verifiserer eigarskap, bootstrap, L5/L5b og full testflate utan C-regen eller stage-0 rebuild

## Dokumentasjon

- [Installasjon](INSTALL.md)
- [Dokumentasjonsinngang](docs/INDEX.md)
- [Brukarmanual](docs/USER_MANUAL.md)
- [Opplæringsguide](docs/LEARNING_GUIDE.md)
- [Dokumentasjonsindeks for vedlikehald](docs/DOCUMENTATION_INDEX.md)
- [Løypekart](docs/LANE_MAP.md)
- [Merkevare og ikon](docs/BRAND.md)
- [Modenheit 10/10](docs/MODENHET_10_10.md)
- [Selfhost-handlingsplan](docs/SELFHOST_HANDLINGSPLAN.md)
- [Status](docs/STATUS.md)

## Verifisering

- `./bin/nc test` går grønt
- `./bin/nc release-preflight` går grønt før tag/release og publiserer ingenting
- `./bin/nc release-preflight --strict` skal vere grøn før push/tag når nye nøkkelfiler skal med til GitHub
- `./bin/nc local-green` er samla lokal grønnliste før tag, release eller større rydding
- `./bin/nc local-green --strict` er samla streng grønnliste før push/tag
- L1-L6/selfhost-verifisering går grønt via `./bin/nc selvstendighet`
- normal verifisering går grønt via `./bin/nc verify-selvstendighet`
- aktiv verktøyflate er fri for Python og C; historisk C ligg berre under `archive/`
- `std.dns`, `std.tls_acme`, `std.mail_server`, `std.domenehost` og `std.brannmur` gir framtidsgrunnmur for mailserver, domenehosting, samla infrastrukturplan, DNSSEC-plan, ACME-auto-renew, atomic mail-spool, retry/dead-letter, mail-karantene, backup/restore og default-drop brannmur med egress, nftables-reglar, dry-run/check, rollback-plan, lockdown, risikorapport og runtime-security policy
- `std.runtime_status` viser at runtime v1 framleis har reelle blokkerarar, men alle v1-rader har no minst ei delvis Norscode-standardflate. Minnehåndtering/heap/GC ligg i `std.runtime_memory` og er kopla til pure-VM med aktiv rammeskanning, parallell rot-stack, automatisk mark/sweep, syklushandtering, byte-regnskap, young/old-promotering og flyttande arenakomprimering. Native `NcVal` brukar NCG1-header, runtime-eigde arenasider, automatisk terskelstyrt minor/major-GC og fysisk relokering av levande objekt med oppdatering av frame-røter, containarar, edge-graf og host-kontekst. Native compiler-regionar med rå C-peikarar blir pinna. Call stack ligg i `std.runtime_stack`, type-runtime i `std.runtime_type`, exception-unwind i `std.runtime_exception`, sikkerheit i `std.runtime_security`, scheduler/trådar i `std.sched`/`std.tråd`, dynamisk lasting i `std.runtime_loader`, prosess-API i `std.prosess`, refleksjon/profilering i `std.inspect`, `std.timeit` og `std.trace`, baseline-JIT i `std.runtime_jit`, og GUI-kontrakt i `std.runtime_gui`.
- Pure-VM handhevar deny-by-default capabilities før sensitive builtins: `disk.read`, `disk.write`, `env.read`, `env.write`, `process.exec`, `net.tcp`, `net.http` og `net.dns`. Disk-, nettverks- og prosess-scopes blir handheva, native filhandtak avviser traversal og symlink-følging, plugin-capabilities er isolerte, og prosessar kan bruke lukka miljø, rlimits, Linux seccomp-profilar og macOS Seatbelt. Windows AppContainer/Job Object og hard Darwin-minnegrense står att.
- `std.runtime.allocator` er no køyrbar Norscode i gjeldande syntaks og gir ei avgrensa 64 MB arena med 8-byte alignment, first-fit-gjenbruk, free/dobbel-free-vern, OOM, flyttande komprimering, relokasjonstabell og statistikk. `std.runtime_memory` brukar arenaadressene, mark/sweep frigjer blokkene og compact oppdaterer objektadressene. Direkte memmove-binding til `native_codegen_v2` sitt fysiske ELF RW-segment står framleis att.
- Pure-VM kan laste og hot-reloade NCB-modular utan restart. Loaderen validerer ABI og modul-ID, verifiserer innhaldet med rein Norscode SHA-256 mot trust store, skriv `__main__` og interne kall om til modulnavnerom, stoppar kollisjonar, tel generasjonar, handhevar dependency-rekkefølgje og isolerer capabilities per plugin. Dynamisk loader er stabil for runtime v1.

## Sjølvstendighet akkurat no

Når `./bin/nc local-green --strict` er grøn, er normalflata lokalt kontrollert: release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate køyrer utan Python/C som aktiv arbeidsveg. Shell-wrapperar finst framleis for operativsystemgrensene og for fallback når native runtime manglar `exec_prosess`, men eigarlogikken ligg i `.no`-filer.

## Lisens

Apache-2.0. Sjå [LICENSE](LICENSE).

## Forfattar

Jan Steinar Sætre
