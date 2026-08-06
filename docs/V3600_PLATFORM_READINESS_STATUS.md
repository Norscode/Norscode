# v3600 Platform readiness status

Status 2026-08-06: denne fila dokumenterer historisk v3600-evidens. Ho er
ikkje ein attestasjon av den gjeldande fixed14-kandidaten. Fixed14 har lokal
macOS-`local-green --strict`, men manglar nye, signerte rapportar frå macOS,
Linux x86-64, Linux ARM64 og ekte Windows bundne til same kjeldegenerasjon.

## macOS

```text
dist=norscode_native
stage0=bootstrap/stage0/norscode-macos-arm64
runtime_gap=green
production_ready_macos=false
reason=current_fixed14_generation_not_remotely_attested
```

Aktuelle SHA256:

```text
dist/norscode_native: aef58c7894925b53c67f350992ffcb9df56849f352c18829aa17ca50a1917717
bootstrap/stage0/norscode-macos-arm64: dacbcd3edfc3447dbbf729f1097a05840db42cbf3e5d2ed4d12dc8a70c6cb26d
```

## Linux x86_64

Linux x86_64 er krysskompilert utan Docker med Zig. Artefaktet er kontrollert som ELF 64-bit x86-64, og full runtime-gap er køyrd i Ubuntu 24.04 Docker.

```text
linux_candidate=build/v3600/linux/norscode_native_linux_x86_64_v3605_zigargon
linux_stage0=bootstrap/stage0/norscode-linux-x86_64
cross_compile=green
runtime_execution=green
production_ready_linux_x86_64=false
reason=current_fixed14_generation_not_attested
```

Aktuelle SHA256:

```text
build/v3600/linux/norscode_native_linux_x86_64_v3602: fee58cf80a741f27fd84aa6242a859ede0117a42b3637ef5244d0a76f4ab6586
build/v3600/linux/norscode_native_linux_x86_64_v3605_zigargon: 6d89bea8f18cbd323948c4b0873a4ee0307ff501ec9eb953813b59b2633497fb
build/v3600/linux/norscode_native_linux_aarch64_v3608_zigargon: 90b12e6fa2ff28f6c39cc874905d7028b09dec1a7c02eb519b2005389220ab7f
bootstrap/stage0/norscode-linux-x86_64: 1b13546961aa2c647cc97211ca86e45eb41b79191cecdf3e4577765bb8bc1acc
```

## Global status

```text
production_ready_macos=false
production_ready_linux_x86_64=false
production_ready_linux_arm64=false
production_ready_windows=false
production_ready_all_platforms=false
reason=current_fixed14_generation_requires_new_signed_attestations
```

Dei eldre v3600-resultata under viser at plattformporten har vore grøn for ein
tidlegare generasjon. Dei kan ikkje brukast som produksjonsbevis for fixed14.
Stage0-seed, ekte Windows-køyring, ACME-utferding og andre
plattformspesifikke native backendar blir framleis rapporterte separat i
`std.runtime_status`.

Historisk vart Linux ARM64 køyrd native med 560/560 testar og utan
Rosetta-emulering. Same port må køyrast på nytt for fixed14.

Dei historiske v3602/v3606 Zig-verktøya er fjerna frå aktiv `tools/`-flate og
kan ikkje brukast som kommandoar eller gjeldande bevis. Ny readiness skal gå
gjennom dei native `.no`-eigarane og publisere ferske, signerte rapportar for
same kandidatgenerasjon. `tools/platform_readiness_v3600.no` er framleis den
fail-closed samla rapportporten.
