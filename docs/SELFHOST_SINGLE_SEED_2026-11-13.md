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
[Den komplette lokale rapporten](proofs/toolchain-fixed-point-bb2998b.json)
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

Fullt kaldt fikspunkt på den endelige revisjonen, ekte ARM64-/macOS-kjøringer,
oppdaterte macOS-kodegeneratorer og eventuell kontrollert stage0-promotering
må være bestått før arbeidet merkes ferdig. En grønn ordinær CI-kjøring alene
er ikke dokumentasjon på at disse nye kravene er oppfylt.
