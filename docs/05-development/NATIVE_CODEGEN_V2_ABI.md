# native_codegen_v2 ABI-kontrakt

Formålet er å dokumentere kva `selfhost/native_execution/native_codegen_v2.no` krev av
runtime-kontrakt, minneoppsett og inngangspunkt-åtferd for staging og oppstart.

## Runtime-seksjonar

- `TEXT_VA()` – tekstsegmentbase: `4198400`.
- `DATA_VA()` – konstantdatabase (string-litteraler): `5242880`.
- `HEAP_VA()` – heapbase: `6291456`.
- `HEAP_SZ()` – reserverer 64 MiB heap.

Kompilerte ELF-binarar legg runtime-kode på `TEXT_VA` og data/heap i faste område
for deterministisk bygging.

## Skilnad mot historisk C-runtime

Det finst no to ulike runtime-kontraktar i repoet:

- Aktiv native kontrakt:
  - `selfhost/native_execution/native_codegen_v2.no`
  - brukar fast `RT_*`-tabell, ELF-segment og `NcVal*`-basert kallkonvensjon direkte i native utdata
- Historisk vedlikehaldskontrakt:
  - `archive/legacy_c_backend/ncb_to_c.no`
  - embedder `archive/legacy_c_backend/nc_runtime_mini.c` i generert `bootstrap/maint/c/norscode_generated.c`
  - blir berre brukt i den historiske vedlikehaldsflata for seed-regenerering

`archive/legacy_c_backend/nc_runtime_full.c` er historisk referanse, ikkje del av normal native kjede
og ikkje del av seed-first bygg i dagleg flyt.

## Oppstart og stack

`_start`:

1. Initialiserer `rbp`/stakk og legg `argv/envp` i runtime-slot.
2. Justerer stack til SysV-aligned tilstand.
3. Kallar `RT_INIT_HEAP()` før start-funksjon.
4. Kallar entryfunksjon (`start_fn`) frå NCB-entryfeltet.
5. Utfører `exit(0)` på return.

I debugging-løp blir det skrive `A/B/C` som korte `sys_write`-markørar før og under desse fasane.
`S` vert skrive ved fatal invalid-entry og før eksplisitt avslutning.

## Funksjons- og kallkontekst

- Kall frå NCB-bytecode går gjennom standard runtime-tabell generert i `native_codegen_v2.no`.
- For kvart kall:
  - argument er lasta på stack og flytta til register i omvendt rekkjefølge.
  - resultat vert alltid lagra som `NcVal*` peikar på stack.
- Start-funksjonen kjem frå NCB-feltet `entry`; dersom det er ugyldig og oppslag feilar,
  vil koden gå via ei eksplisitt `exit`-sti, ikkje ukontrollert `call 0`.

## Kjernekonvensjonar

- ABI mellom native koden og runtime brukar i hovudsak:
  - `rdi`, `rsi`, `rdx`, `rcx` for brukarfunksjonar (inntil 4 argument).
  - `rax` som returnverdi for runtime.
  - stackbaserte `NcVal*`-peikarar.
- Alle runtime-API-er eksponert via VA-konstanter `RT_*()` i same fil.

## Inngangspunkt og determinisme

- `start_fn` er deterministisk avleia frå:
  1. NCB `entry` når den er gyldig.
  2. fallback til `__main__.start`/`*.start` dersom `entry` manglar.
- Funksjonar er ikkje sortert globalt alfabetisk; ordninga kjem frå NCB-parse/rekkjefølgje.
  Runtime-innkjøring må difor alltid stole på eksplisitt `entry` når determinisme krev det.
- For bootstrap-/elf-steg vart `start` sett via `NORSCODE_BUNDLE_ENTRY` i bundleren.
  Den same mekanismen vart brukt både når host bygde stage-0 NCB og når Gen1 ELF
  bygde Gen2 NCB i 6b-løypa.

## Relokasjon og output

- `ncb_to_elf` byggjer frå JSON-format med:
  - `functions` som namn→`code`/`constants`.
  - `entry` som søkjbar nøkkel i denne kartstrukturen.
- Genererte binærar er forventa deterministiske på same input og same byggereglar.
