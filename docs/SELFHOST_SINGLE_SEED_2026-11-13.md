# Kaldt selvbygg fra én Norscode-seed

## Mål og godkjenningskrav

Dette arbeidet gjelder kompilatoren, kodegeneratorene og runtime i
`Norscode/Norscode`. Den aktive språkkjeden følger `AGENTS.md`:
Norscode-kilde → NCB → Norscode-eid native runtime/VM. Bruk av Norscodes egen
VM er ikke en avhengighet av Python eller en fremmed kompilator.

Et fullført selvbygg skal dokumentere følgende:

1. Kaldt utgangspunkt med bare `.no`-kilder og én hashverifisert native seed.
2. Første kodegenerator bygges ved å kjøre kodegeneratorkilden i denne seeden.
   Ingen forhåndsbygd kodegenerator, bytekodebunt eller fragmentcache kopieres inn.
3. Tre generasjoner bygger både kodegenerator og full compiler/runtime fra
   kilde. Hver generasjon bruker forrige generasjons binærer.
4. Gen2 og Gen3 er like for kodegenerator-NCB, kodegenerator-binær,
   runtime-NCB og runtime-binær. Alle byggetrinn krever vellykket prosess,
   null uløste/uventede builtins og en faktisk runtime-selvtest.
5. Kildeinventar, kildehash og opprinnelig seed er uendret gjennom hele løpet.
6. Ingen kompilert bytekode er sporet i Git. CI håndhever dette med
   `NORSCODE_BYTECODE_GATE=hard` og et bytekodetak på null.
7. De fire plattformene har signert, kildebundet runtime-verifikasjon:
   Linux x86-64, Linux ARM64, macOS ARM64 og Windows x86-64.

Punkt 1–5 prøves direkte av `tools/selfcompile_toolchain.no` på Linux x86-64.
Dette er et helt verktøykjedebygg; det supplerer eksisterende L5/L5b- og
ELF-driverkontroller. Plattformbygg og kjøring på ekte maskiner er egne
krav og blir ikke erstattet av det lokale fikspunktet.

## Kjøring

Etter bootstrap-materialiseringen i `INSTALL.md`:

```text
./bin/nc run tools/selfcompile_toolchain.no
```

Hver kjøring får en ny mappe under `build/toolchain-rebuild/`.
`latest.txt` peker til den aktuelle mappen. `report.json` inneholder status,
kildeinventarets SHA-256, seed-identiteten og alle generasjonenes inn- og
utdatahash. Hver generasjon har komplette bygglogger og `sources.manifest`.
Fasefeltet oppdateres før kildekopiering, kodegeneratorbygg, runtimebygg,
validering, kildekontroll og fikspunktsammenlikning. Rapporten viser også
starttid, siste fremdrift og brukt tid i millisekunder. En avbrutt prosess
med `status=running` er ikke et bestått resultat.

Hver generasjon beholder SHA-256 fra den foregående generasjonens validerte
utdata. Endres en binær før neste steg, avvises den før kopiering. Både
runtimebinæren og runtime-NCB kontrolleres mot byggrapporten. CI-revisjonen
videresendes eksplisitt i det lukkede miljøet og må samsvare i alle
runtime-byggrapportene. Lokale kjøringer uten GitHub-revisjon bindes fortsatt
til det komplette kildeinventarets SHA-256.

Barneprosessene har et lukket miljø, `PATH=/nonexistent`, lokale kilder og
avslått gjenbruk av precompiled-/verktøycacher. Første kodegenerator kjøres
fra kilde i VM og kan derfor ta vesentlig lengre tid enn senere native
generasjoner. En manglende fil, prosessfeil, kildeendring eller ulikt
fikspunkt stopper løpet. Verktøyet endrer ikke repoets stage0 eller dist.
Kjør derfor hele verifikasjonen mot et uendret tre; samtidig redigering av
kilder skal gjøre kildekontrollen rød.

## Rettelser som følger med

- Linux ARM64 kjører samme komplette runtime-probe som Linux x86-64 på en
  ekte ARM64-runner. Rapportmodus uten kjøring og kjøring på driveren i
  stedet for kandidaten er fjernet.
- ARM64-kodegeneratoren ruter Argon2id til den inkluderte
  `std.argon2id_pure.hash_raw_hex`. Den tidligere wrapper-ruten manglet i
  fullhost-bunten og ga en native stub. Den fulle 64 MiB-testen er beholdt.
- Utgivelsens brukerreise krever både et vellykket native-bygg og faktisk
  kjøring av et generert program med bestemt utdata og exitkode. En fil som
  ligger igjen etter et mislykket bygg kan ikke godkjennes.
- Den siste sporede v9400-bytekodebunten er fjernet. Fire ubrukte shellmaler
  for prosjektstarteren er også fjernet; prosjektgeneratoren materialiserer
  allerede en ekte native-starter.
- Både kjedeorkestratoren og runtimebyggeren hasher store filer i kortlivede
  barn av den kjørende Norscode-binæren. Det hindrer at boksede byte-lister fra
  flere generasjoner samles i byggerens heap. Filtype, størrelse, full digest
  og alle eksisterende kilde- og rapportkrav kontrolleres fortsatt. Manglende
  filer kan ikke få samme godkjenning som en ekte tom fil.
- Windows-portene bevarer syscall-registrene gjennom Win32-kall, også på
  feilveier. Den uavhengige maskinkodeprøven ødelegger bevisst Win32s volatile
  registre og shadow space. Den dekker filoperasjoner, søking, venting,
  katalogspørring, minneallokering og tidlig feil ved prosessopprettelse.
  Linux-kjøring av denne prøven erstatter ikke verifikasjon på Windows.
- Hele den signerte CRL-/OCSP-matrisen kjøres som et native Norscode-program.
  De opprinnelige vektorene og alle byteavkortingskontrollene er bevart i
  `tests/fixtures/x509_revocation_native_probe.no`. Byggeren må rapportere
  null uløste/uventede builtins, og proben må faktisk kjøres til sluttmarkøren.

## Verifikasjonsstatus

Et fullt lokalt kaldbygg har bestått på revisjon
`bb2998b438d8795dac241eb327a173b335e05741`.
[Den komplette lokale rapporten](#komplett-lokal-rapport)
viser én opprinnelig binær, null forhåndsbygde kodegeneratorer, null kopierte
bytekodebunter, 1047 kildefiler og uendret kildeinventar gjennom hele løpet.
Alle tre generasjoner fullførte L5-byteparitet, runtimebygg, selvtest og
sluttvalidering. Gen2 og Gen3 er byte- og hashidentiske for kodegenerator-NCB,
kodegenerator-binær, runtime-NCB og runtime-binær. Prosessen avsluttet med
status 0 etter 1162682 ms.

Etter denne revisjonen er ARM64-ruten for `builtin.kompiler_fil` rettet til
den eksisterende, bundla Norscode-kompilatoren. Kodegen- og VM-regresjonene
består lokalt, og både Linux ARM64- og macOS ARM64-kandidatene er bygd på nytt.
Deres rapporter viser null uløste builtins og tre gjenværende tråd-/kontekst-
stubber; `kompiler_fil` er ikke lenger en stub. Vanlig og aktiv kompilator-ABI
holdes adskilt. Den lokale kaldbyggrapporten over gjelder uttrykkelig
`bb2998b`, før denne ARM64-endringen.

Lokalt er kildebygd Linux x86-64-runtime, full x86-runtime-probe, faktisk
native-bygg/kjøring i installert mappestruktur, kald bootstrap-materialisering,
prosjektgenerering og de nye negative regresjonene verifisert.

macOS-kodegeneratorene er deretter regenerert på ekte macOS ARM64 i
[native-codegen-refresh 43529255905](https://github.com/Norscode/Norscode/actions/runs/43529255905)
fra PR-revisjon `689fae4f69dd7e8d3bc272b731a1e3fec80e1e52`. Runnerens
merge-revisjon har samme kildetre `c12a2e88c9949c4e55449992f63c59117e52b5d6`.
Alle tre vertsgenerasjonene er byteidentiske og har null uløste builtins.
De tre importerte Mach-O-filene beholder tilhørende kilde-, avhengighets-
og produsentbindinger fra det verifiserte artefaktet; bare deres tre binærpinner
er oppdatert i aktivflate-allowlisten. Dette dokumenterer kodegeneratorbygget,
mens full runtime-attestasjon fortsatt er et separat krav.

Fullt kaldt fikspunkt på den endelige revisjonen, ekte ARM64-/macOS-kjøringer,
CI med oppdaterte macOS-kodegeneratorer og kontrollert stage0-promotering
må være bestått før arbeidet merkes ferdig. En grønn ordinær CI-kjøring alene
er ikke dokumentasjon på at disse nye kravene er oppfylt.

## Komplett lokal rapport

Rapporten er bevart uendret som dokumentasjon her. Genererte rapporter hører
til byggartefaktene; denne kopien legger ikke til en ny JSON-fil i den aktive
repositoryflaten eller øker JSON-taket i selvstendighetskontrollen.

```json
{"schema":"norscode-toolchain-fixed-point-v1","status":"ok","fixed_point":true,"initial_binary_count":1,"prebuilt_codegen_input_count":0,"copied_bytecode_count":0,"promoted":false,"started_at_ms":1794611191956,"step":"complete","last_progress_at_ms":1794612354638,"elapsed_ms":1162682,"bootstrap_sha256":"1f7b588917c47e5c68dce3000df6f9bc7d8cde8e5e90e669a5fb1c7a0f62de48","source_file_count":1047,"source_commit":"bb2998b438d8795dac241eb327a173b335e05741","source_manifest_sha256":"43ed8ea745e0a2b169768609610978e11c56ebbedd37dea6f09576c8a6e44b23","initial_codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","initial_codegen_from_source_vm":true,"generations":[{"generation":1,"workspace":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen1","input_runtime_sha256":"1f7b588917c47e5c68dce3000df6f9bc7d8cde8e5e90e669a5fb1c7a0f62de48","input_codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_ncb_sha256":"a49567052fcead9f5b8061f2cf6299096543b6e79a04885f2724c657287db78d","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","runtime_ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","runtime_build":{"schema":"norscode-ci-runtime-v1","status":"ok","source_commit":"bb2998b438d8795dac241eb327a173b335e05741","bootstrap_sha256":"1f7b588917c47e5c68dce3000df6f9bc7d8cde8e5e90e669a5fb1c7a0f62de48","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_source_sha256":"f9196baf234e7292518c23760785ee09ffde2483867ca3f4f00a1fc75045c384","codegen_dependencies_sha256":"5e1dbfd420875a88f6b771ba6003ca28c54e79ef2ee089f072fccafc41d60d84","runtime_sources_sha256":"ee41a4b7c8f540ea3be2c32dcdbd6f45f7165cac761b73d5a7294a2f76a397d0","runtime_source_files":64,"compiler_driver_functions":440,"l5_byte_parity":true,"ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","candidate_path":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen1/build/ci-runtime/norscode_native","candidate_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","unresolved_builtins":0,"unexpected_builtins":0,"selftest":"passed","promoted":false}},{"generation":2,"workspace":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen2","input_runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","input_codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_ncb_sha256":"a49567052fcead9f5b8061f2cf6299096543b6e79a04885f2724c657287db78d","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","runtime_ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","runtime_build":{"schema":"norscode-ci-runtime-v1","status":"ok","source_commit":"bb2998b438d8795dac241eb327a173b335e05741","bootstrap_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_source_sha256":"f9196baf234e7292518c23760785ee09ffde2483867ca3f4f00a1fc75045c384","codegen_dependencies_sha256":"5e1dbfd420875a88f6b771ba6003ca28c54e79ef2ee089f072fccafc41d60d84","runtime_sources_sha256":"ee41a4b7c8f540ea3be2c32dcdbd6f45f7165cac761b73d5a7294a2f76a397d0","runtime_source_files":64,"compiler_driver_functions":440,"l5_byte_parity":true,"ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","candidate_path":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen2/build/ci-runtime/norscode_native","candidate_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","unresolved_builtins":0,"unexpected_builtins":0,"selftest":"passed","promoted":false}},{"generation":3,"workspace":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen3","input_runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","input_codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_ncb_sha256":"a49567052fcead9f5b8061f2cf6299096543b6e79a04885f2724c657287db78d","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","runtime_ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","runtime_build":{"schema":"norscode-ci-runtime-v1","status":"ok","source_commit":"bb2998b438d8795dac241eb327a173b335e05741","bootstrap_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","codegen_source_sha256":"f9196baf234e7292518c23760785ee09ffde2483867ca3f4f00a1fc75045c384","codegen_dependencies_sha256":"5e1dbfd420875a88f6b771ba6003ca28c54e79ef2ee089f072fccafc41d60d84","runtime_sources_sha256":"ee41a4b7c8f540ea3be2c32dcdbd6f45f7165cac761b73d5a7294a2f76a397d0","runtime_source_files":64,"compiler_driver_functions":440,"l5_byte_parity":true,"ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","candidate_path":"/workspace/scratch/ed54926806f1/Norscode/build/toolchain-rebuild/8546013f8189e41e/gen3/build/ci-runtime/norscode_native","candidate_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","unresolved_builtins":0,"unexpected_builtins":0,"selftest":"passed","promoted":false}}],"codegen_ncb_sha256":"a49567052fcead9f5b8061f2cf6299096543b6e79a04885f2724c657287db78d","codegen_sha256":"0c3c9e0241fa7619d3dcbcd3db1ff1409ae6869f43281d130197f9d31aeef4f2","runtime_ncb_sha256":"4f5dab6c0c9218e4df04b1f94f108978561e96cc0f655516dbe77a5eea23a5d8","runtime_sha256":"81904e15c8bde074b2863e3ad89c78e4a15f629aa7b2d4712a4923779090d51e","source_tree_unchanged":true}
```
