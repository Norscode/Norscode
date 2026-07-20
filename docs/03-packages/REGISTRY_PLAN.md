# Plan for aa fullfoere registryet

Maalet er aa gjere Norscode sitt pakke- og registry-oppsett til ein reell, brukbar og self-host-vennleg plattform, utan aa basere normal utvikling og drift paa Python eller C.

Status: **Delvis implementert (June 2026)**

## Maal

- Gjere registry-opplegget til ein faktisk pakkeplattform.
- Halde flyten i Norscode: `norcode.toml` -> resolver -> lockfile -> fetch -> build -> publish -> registry.
- Sikre at normal bruk, bygg og CI kan skje utan Python og C.

## Fase 1: Spesifiser kontrakten

- Lås ned kva ein pakke er: namn, versjon, checksum, source, dependencies og metadata.
- Definer minimumsfelta i `norcode.toml`.
- Definer formatet for `norcode.lock`.
- Bestem kva kilder som skal støttast:
  - `registry`
  - `git`
  - `url`
  - `path`

## Fase 2: Ferdigstill manifest-parseren

- Gjer `norcode.toml` til den stabile kjelda for prosjekt og pakker.
- Legg inn validering for namn, versjon og avhengigheiter.
- Gi tydelege feil ved ugyldige manifest.
- Ver robust mot manglande eller konfliktande felt.

## Fase 3: Bygg ein deterministisk resolver

- Sørg for at same input alltid gir same resultat.
- Støtt versjonsreglar utover eksakt match, til dømes `^`, `*` og intervall.
- Handter transitive dependencies.
- Oppdag og rapporter sirkulære avhengigheiter.
- Lag konfliktmeldingar som er lette aa feilsoke.

## Fase 4: Stabiliser lockfile-flyten

- Generer lockfile frå resolver, ikkje manuelt.
- Inkluder checksum, kjelde og eksakt versjon.
- Gjer lockfile lesbar og maskinell.
- Verifiser lockfile ved installasjon og build.

## Fase 5: Gjer registry-protokollen ekte

- Spesifiser HTTP-endepunkt for:
  - liste pakker
  - hent metadata
  - hent artifact
  - publish
  - yank eller deprecate
- Legg til auth for publisering.
- Legg til checksum-verifisering ved nedlasting.
- Beskriv responsformatet tydeleg.

## Fase 6: Implementer klientflyten i `nc`

- Lag kommandoar som:
  - `nc add`
  - `nc remove`
  - `nc update`
  - `nc lock`
  - `nc fetch`
  - `nc publish`
  - `nc install`
- Sørg for at kommandoane brukar Norscode-kode, ikkje eksterne scripts.
- Koble kommandoane til manifest, resolver og lockfile.

## Fase 7: Fullfør lokal cache og installasjon

- Last ned pakker til cache først.
- Installer frå cache når det er mogleg.
- Verifiser checksum før bruk.
- Gjer installasjon repeterbar og mest mogleg offline-vennleg.

## Fase 8: Bygg publiseringsflyten

- Valider pakka før publish.
- Sjekk at versjon ikkje allereie finst.
- Pakk kjeldekode eller artifact deterministisk.
- Signer eller checksum-beskyt publiserte artefaktar.
- Logg publiseringar i registry.

## Fase 9: Legg til tryggleik og kontroll

- Auth for publish.
- Rate limiting på registry.
- Yank av sårbare eller feilaktige versjonar.
- Audit log over publiseringar.
- Tydlege reglar for private vs offentlege pakkar.

## Fase 10: Test alt ende-til-ende

- Manifest parsing.
- Resolver med enkle og transitive dependencies.
- Lockfile-generering.
- Registry publish og fetch.
- Installasjon frå registry.
- Offline installasjon frå cache.
- Feiltestar for konflikt, manglande pakke, checksum-mismatch og syklisk dependency.

## Fase 11: Dokumenter brukarreisen

- Éi side for korleis ein brukar opprettar pakke.
- Éi side for korleis ein publiserer.
- Éi side for korleis ein installerer og oppdaterer.
- Éi side for registry-format og API.

## Fase 12: Gjer det self-host-ready

- Flytt all kritisk pakke-logikk inn i Norscode-runtime eller selfhost-lag.
- Unngaa at produksjonsflyten er avhengig av legacy-verktøy.
- Hald legacy som fallback og merk det tydeleg.

## Rekkefolge eg ville tatt

1. Manifest og lockfile-kontrakt.
2. Resolver og versjonsreglar.
3. Lokal installasjons- og fetch-flyt.
4. Registry API.
5. Publish-flyt.
6. Tester og dokumentasjon.
7. Self-hosting og hardening.

## Noverande status

Statusen har kome vidare sidan den første planen blei skrive. Repoet viser no ein meir konkret pakke-linje med eigne modulflatar og dokumentert API.

Det som no ser ut til aa vere på plass:

- ein delt `norspkg`-struktur med eigne moduler for manifest, resolver, lockfile, cache, auth, publisher og registry-protokoll
- ein CLI-overflate i `toolchain/norspkg/nc.no` med kommandoar som `add`, `remove`, `update`, `install`, `publish`, `info`, `search` og `yank`
- eit dokumentert HTTP-registry-API i `docs/REGISTRY_API.md`
- ein tydeleg protokollspec i `docs/REGISTRY_PROTOCOL.md`
- brukarrettleiing for pakker i `docs/PACKAGES.md`
- registry-/pakkeeksempel i `packages/registry.toml` og `packages/remote_registry_example.json`
- ende-til-ende-flytar i modulane:
  - manifestvalidering
  - deterministisk resolver
  - lockfile-generering og -verifisering
  - cache og installasjon
  - publisering og yank
  - checksum-verifisering
  - audit-logg
- semver- og konfliktreglar for `^`, `*`, `>=` og `<=` i resolveren
- auth- og tokenflyt for publish/yank/admin i registry-protokollen

Det som framleis ser ut til aa mangle før dette kan kallast heilt ferdig:

- ein driftsett, offentleg registry-instans med permanent livsløp og release-/operasjonsansvar
- full testdekning som køyrer automatisk mot alle hovudflyter i CI
- eventuelle vidare skjerpingar rundt signaturar og distribuerte mirror-løysingar

## Kva som no kan reknast som fullført i repoet

- Manifestet er spesifisert og validert.
- Resolveren støttar deterministisk val av pakkeversjonar.
- Lockfile-flyten er definert og maskinlesbar.
- Registry-API-et er dokumentert med publish, yank, list, metadata og audit-logg.
- Cache- og installasjonsflyten er implementert som eiga modul.
- CLI-en har dei viktigaste pakkeoperasjonane definert.
- Publisering har validering, checksum og duplikatvern.

## Kva som er att som operasjonell finish

- sette opp ein stabil registry-teneste i produksjon
- kople modulane til ein fullt verifisert runtime-/CI-løype
- eventuelt legge til signaturverifisering og mirror-støtte dersom det blir krav
