# v3600 Platform readiness status (historisk snapshot)

Dette dokumentet er eit eldre v3600-snapshot med kandidathashar. Det er ikkje
releasebevis for Norscode 1.0. Bruk den signerte
`reports/norscode-completion-v1.json` frå same commit som kjelda for aktuell
plattformstatus og `production_ready_all_platforms`.

Status: macOS runtime-gap er verifisert lokalt. Linux x86_64-kandidat er krysskompilert og full runtime-gap er verifisert i Ubuntu 24.04 Docker. Portable Zig Argon2id og OpenSSL PBKDF2/ACME er i tillegg verifisert i Linux-releasekandidaten.

## macOS

```text
dist=norscode_native
stage0=bootstrap/stage0/norscode-macos-arm64
runtime_gap=green
production_ready_macos=true
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
production_ready_linux_x86_64=true
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
production_ready_macos=true
production_ready_linux_x86_64=true
production_ready_all_platforms=true
reason=linux_docker_runtime_gap_verified
```

Dette betyr at macOS- og Linux x86_64-linene er verifiserte, inkludert portable Argon2id og OpenSSL-krypto i Linux-releasekandidaten. Stage0-seed, ekte Windows execution, ACME-utstedelse og andre plattformspesifikke native backendar blir framleis rapporterte separat i `std.runtime_status`.

Linux ARM64 er i tillegg køyrd native, med 560/560 testar bestått og ingen Rosetta-emulering.

Repeterbar Docker-kommando:

```sh
NORSCODE_ROOT="$PWD" ./bin/nc run tools/build_linux_cross_candidate_v3602.no

NORSCODE_ROOT="$PWD" ./bin/nc run tools/build_linux_zig_argon_candidate_v3606.no

NORSCODE_ROOT="$PWD" NORSCODE_VERIFY_LINUX_DOCKER=1 ./bin/nc run tools/platform_readiness_v3600.no
```
