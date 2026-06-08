# native_codegen_v2 ABI-kontrakt

Formålet er å dokumentere kva `selfhost/native_execution/native_codegen_v2.no` krev av
runtime-kontrakt, minneoppsett og inngangspunkt-åtferd for staging/bootstrapping.

## Runtime-seksjoner

- `TEXT_VA()` – tekst-segmentbase: `4198400`.
- `DATA_VA()` – konstant-data base (string-litteraler): `5242880`.
- `HEAP_VA()` – heapbase: `6291456`.
- `HEAP_SZ()` – reserves 64MiB heap.

Kompilerte ELF-plattformer plasserer runtime-kode på `TEXT_VA` og data/heap i faste områder
for deterministic bygging.

## Oppstart og stack

`_start`:

1. Initialiserer `rbp`/stakk og legg `argv/envp` i runtime-slot.
2. Justerer stack til SysV-aligned tilstand.
3. Kallar `RT_INIT_HEAP()` før start-funksjon.
4. Kallar entryfunksjon (`start_fn`) frå NCB-entryfeltet.
5. Utfører `exit(0)` på return.

I debugging-løp blir det skrive `A/B/C` som korte sys_write-markørar før/under desse fasane.
`S` vert skrive ved fatal invalid-entry og før eksplisitt avslutning.

## Funksjons- og kallkontekst

- Kall frå NCB-bytecode går gjennom standard runtime-tabell generert i `native_codegen_v2.no`.
- For kvart kall:
  - argument er lasta på stack og flytta til register i omvendt rekkjefølge.
  - resultat vert alltid lagra som `NcVal*` peikar på stack.
- Start-funksjonen kjem frå NCB-feltet `entry`; dersom det er ugyldig og oppslag feilar,
  vil koden gå via ei eksplisitt `exit`-sti, ikkje ukontrollert `call 0`.

## Kjernekonvensjonar

- ABI mellom native koden og runtime bruker i hovudsak:
  - `rdi`, `rsi`, `rdx`, `rcx` for brukarfunksjonar (inntil 4 argument).
  - `rax` som returnverdi for runtime.
  - stack-baserte `NcVal*`-peikarar.
- Alle runtime-API-er eksponert via VA-konstanter `RT_*()` i same fil.

## Inngangspunkt og determinisme

- `start_fn` er deterministisk avleia frå:
  1. NCB `entry` når den er gyldig.
  2. fallback til `__main__.start`/`*.start` dersom `entry` manglar.
- Funksjonar er ikkje sortert globalt alfabetisk; ordninga kjem frå NCB-parse/rekkjefølgje.
  Runtime-innkjøring må difor alltid stole på eksplisitt `entry` når determinisme krev det.
- For bootstrap/elf-steg er `start` no sett via `NORSCODE_BUNDLE_ENTRY` i bundleren.
  Den same mekanismen blir brukt både når host byggjer stage-0 NCB og når Gen1 ELF
  byggjer Gen2 NCB i 6b-løypa.

## Relokasjon og output

- `ncb_to_elf` bygger frå JSON-format med:
  - `functions` som namn→`code`/`constants`.
  - `entry` som funnbar nøkkel i denne kartstrukturen.
- Genererte binærar er forventa deterministiske på same input + same byggereglane.
