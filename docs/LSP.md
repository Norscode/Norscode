# Norscode Language Server (LSP)

Rein Norscode, ingen Python/C/JS i aktiv flate. Tenaren er `selfhost/lsp/straum.no`
(strøymande, LSP 3.17 over stdio med `Content-Length`-ramming). Den eldre
`selfhost/lsp/server.no` er att som diagnose-/batchmodus (env `NORSCODE_LSP_PAYLOAD`,
`NORSCODE_LSP_STREAM`, `NORSCODE_LSP_USE_STDIN`) og blir ikkje lenger brukt av editorar.

## Start

```sh
./bin/nc lsp                      # rutar til straum.no (krev reseed av nc_main, sjå under)
./bin/nc run selfhost/lsp/straum.no   # fungerer i dag (batch-lesing i VM, sjå «Strøyming»)
./bin/nc bygg-native --target linux-x86_64 selfhost/lsp/straum.no straum.elf   # ekte strøyming
```

Dei to første køyrer i VM. Prosjektrota kjem frå `initialize.rootUri` (modulfiler under
`<rot>/std/...`); VM-disk-scopet må inkludere rota, t.d. `NORSCODE_VM_DISK_ROOT=$PWD,/tmp`.

## Strøyming

Lykkja (`lsp_straum_lykkje`) les `fd_les(0, 32768)`, pakkar ut kvar fullstendige ramme
(`lsp_pakk_ut`), handsamar ho (`lsp_behandle`), skriv svar med `fd_skriv(1, …)` og blokkerer
først på neste `read` når bufferet er tomt. Diagnostikk blir *koalesert*: ligg fleire
`didChange` i bufferet, blir berre siste versjon per dokument publisert, rett før lykkja
blokkerer.

| Mål | stdin | Merknad |
|---|---|---|
| AOT `linux-x86_64` (og andre mål med `std.native_gap`-I/O) | `fd_les` — ekte strøyming | verifisert, sjå test under |
| VM (`nc run`, macOS-seed) | `fil_les("/dev/stdin")` til EOF — *batch* | VM har ingen blokkerande delvis stdin-builtin; krev `fd_les` i `vm.no` (reseed) |

Reserven vel automatisk: `lsp_io_modus()` prøver `fd_les`; manglar builtin-en, brukast batch.
Rå samanhengande JSON-objekt utan header blir òg godteke (feilsøking, og prosess-ABI-en
tillèt ikkje linjeskift i stdin).

## Støtta

`initialize` (capabilities, rot frå `rootUri`/`workspaceFolders`), `initialized`, `shutdown`,
`exit` (exit-kode 0 etter shutdown, ellers 1), `$/cancelRequest` (ignorert),
`textDocument/didOpen|didChange|didClose|didSave`. `didChange`: inkrementell (range) og full.
`textDocument/publishDiagnostics`: syntaks (lexer/parser), typefeil (`selfhost/compiler/typecheck.no`,
same som `nc check`), og lint-åtvaringar (ubrukt variabel/import, re-deklarasjon, uoppnåeleg kode,
typeendring) frå `selfhost/linter_kjerne.no`; linje/kolonne frå funnet, rekkja dekkjer ordet.

- `hover`: signatur (`funksjon f(a: heltall) -> tekst`), dokumentasjonskommentar (`//`-linjer rett over deklarasjonen),
  type på parameter/lokal (annotasjon eller literal), struktur/enum, strukturfelt, importalias, `modul.symbol`, nøkkelord.
- `definition`: funksjon/struktur/enum/global i fila, lokal/parameter, andre opne dokument,
  `bruk a.b som x` og `x.symbol` til modulfila under rota.
- `completion` (trigger `.`): lokale, toppnivå-symbol, modulalias, nøkkelord; etter `alias.` modulens
  funksjonar/strukturar; etter `verdi.` strukturfelt. Filtrert på prefikset.
- `documentSymbol` (hierarkisk: strukturar med felt), `formatting` (`selfhost/formatter.no`, ei
  heildokument-redigering), `references` og `rename` (ord-basert innan fila: IDENT-token, ikkje
  strengar/kommentarar; nytt namn valideres, nøkkelord avvist).

Posisjonar mot klienten er UTF-16-einingar; internt byte (tekst i Norscode er ei byte-sekvens).

## Avgrensingar (ærlege)

- Namneoppslag er leksikalsk (ingen full skopeoppløysing): `references`/`rename` rører alle
  forekomstar av ordet i fila, ikkje berre den bundne. Rename går ikkje på tvers av filer.
- Hover-type for utypa variablar byggjer berre på literal-initialisator; ingen full typeinferens
  frå R3-metadata for uttrykk.
- `Formatter` (`format_kjelde`) har kjende feil på enkelte konstruksjonar (t.d. `enum`-linja);
  tenaren leverer det formatteren gjev.
- VM-stdin er batch (sjå over). `nc lsp` rutar til straum.no først etter reseed av `nc_main.no`.
- `workspace/*`, `semanticTokens`, `codeAction`, `signatureHelp` er ikkje implementert.

## VS Code-klient

`vscode-norscode/` har berre grammatikk/snippets/ikon (ingen JS i aktiv flate, AGENTS.md).
Klienten (`extension.js` + manifest med `main`, `vscode-languageclient`, innstillingar
`norscode.server.command|args|env`) blir GENERERT av `tools/lag_vscode_klient.no` til
`archive/vscode-klient/`:

```sh
NORSCODE_VM_DISK_ROOT=$PWD NORSCODE_VM_CAPABILITIES=disk.read,disk.write,env.read ./bin/nc run tools/lag_vscode_klient.no
LAG_KLIENT_SJEKK=1 ... ./bin/nc run tools/lag_vscode_klient.no    # feilar viss utgåva er utdatert
```

Pakking: sjå `archive/vscode-klient/README.md`. Inntil `nc lsp` rutar til straum.no i ein reseeda
`nc`, sett `norscode.server.command` til `./bin/nc` og `norscode.server.args` til
`["run", "selfhost/lsp/straum.no"]`.

## Testar

| Test | Dekkjer |
|---|---|
| `tests/test_lsp_straum.no` | protokollkjerne: ramming (delt ramme, byte-lengd), UTF-16, init/capabilities, diagnostikk med eksakt linje/kolonne, hover/definition/completion/documentSymbol/references/rename/formatting, inkrementell sync, koalesering, shutdown/exit, modul-oppslag |
| `tests/test_lsp_straum_prosess.no` | `nc run straum.no` som barneprosess (macOS; hoppar over på Linux-seed) |
| `tests/test_lag_vscode_klient.no` | innsjekka klient = generatorutdata |
| `tests/aot_lsp_straum_drivar.no` | **ekte interaktiv strøyming**: driveren startar tenaren med to røyr og krev kvart svar *før* neste melding vert send (stdin ope). Ikkje ein `nc test`-test; køyrast manuelt: |

```sh
./bin/nc bygg-native --target linux-x86_64 selfhost/lsp/straum.no build/rclsp/straum.elf
./bin/nc bygg-native --target linux-x86_64 tests/aot_lsp_straum_drivar.no build/rclsp/drivar.elf
docker run --rm --platform linux/amd64 -v $PWD/build/rclsp:/w nc-x86tools:latest /w/drivar.elf
```

Verifisert at driveren feilar mot ein variant av tenaren som les stdin til EOF (alle strøymingssjekkane tidsavbrot).
