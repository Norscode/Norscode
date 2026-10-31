# ARM64-seedar etter Windows-sikkerheitsvakta

PR #275 si CI `41218875322` bygde ferske ARM64-kandidatar, men stoppa fordi dei
committa seedane ikkje var byte-like kjelda. Kandidatane frå denne køyringa er
testa før promotering på den same eksisterande PR-branchen.

| Plattform | SHA-256 | Seed-port |
|---|---|---|
| Linux ARM64 | `9e46b12a01000de74f3ebf7ac5d65f16300a89b2fcf8aca29b96dc4fc0cfd8aa` | 116 bestått, 2 hoppa, 0 feil |
| macOS ARM64 | `e1b5fb8607abe4a7ce57135edf887004d404e126d4af1f83925a0f8728916b2a` | 117 bestått, 1 hoppa, 0 feil |

Begge køyrde heile den eksplisitte `tools/seed_gate_tests.txt` (118 oppføringar)
gjennom kandidaten som `dist/norscode_native`, med `NC_TEST_ALL=1`,
`NC_SLOW_TESTS=1`, `NORSCODE_VM_FAST=1`, `NC_TEST_VM_FAST=1`, `TEST_TIMEOUT=600`
og `SLOW_TEST_TIMEOUT=1200`. macOS gjekk på ekte ARM64-vert, Linux i Ubuntu 24.04
ARM64-container med eigen checkout. Hoppa testar er plattformklassifiseringa i
løparen, ikkje full støtte for andre plattformer.

Begge kandidatane bestod `selftest`. Stage0-manifestet er regenerert gjennom
Norscode og verifisert mot alle fire stage0-filer. Binærpinnane er oppdaterte i
same commit. Feature-check for `std/native_gap.no` og diff-kontroll bestod.

Kandidatane vart bygde frå PR-tip `4efbe909c2c1b32b6164b0aa4babef9fd6801cab`.
Berre seedar, manifest, pinnar og dette testbeviset er endra etterpå. Ny CI må
stadfeste byte-likskap mot eit nytt kjeldebygg og køyre portane på ekte CI-vertar.
Windows-stage0 er ikkje endra; Windows-smoke og negativ sandbox-regresjon bestod
allereie i `41218875157`. AppContainer er framleis ikkje implementert.
