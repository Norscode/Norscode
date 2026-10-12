# Utviklarverktøy: dev, coverage, doc, profile, debug, repl (5b + F44)

Alle verktøya er rein Norscode (`tools/nc_*.no`) og blir køyrde av `nc` via same
verktøydispatch som andre `nc`-kommandoar. Argument går i `NC_VERKTOY_ARGV` (linjeskild liste).
Dispatchen (`nc dev`, `nc coverage`, ...) ligg i `selfhost/nc_main.no` og verkar i `./bin/nc` etter
**reseed**; før det køyrer ein verktøyet direkte, t.d.
`NC_VERKTOY_ARGV=$'fil.no' NORSCODE_ROOT=$PWD ./bin/nc run tools/nc_doc.no`.

## nc serve --reload / nc dev (5b)

```
nc dev <app.no> [--port P] [--host H] [--workers N] [--watch STI]... [--intervall MS] [--debounce MS]
nc serve <app.no> --reload [...]        # alias
```

Overvakar `app.no` og lokale `bruk`-modular (rekursivt; `std.`/`selfhost.` blir ikkje overvaka;
`--watch` legg til fil/mappe). Polling med innhaldssamanlikning (ingen OS-spesifikke API).
Ved endring: debounce (`--debounce`, standard 250 ms stille), `nc check`, så SIGTERM til tenaren
(han drenerer sjølv), ventar på utgang (SIGKILL etter 10 s), startar ny. Logg:
`[dev] endra: app.no → startar på nytt`. **Kompileringsfeil**: feilen blir vist og den gamle
tenaren held fram til neste vellukka endring. Ein tenar som stoppar av seg sjølv blir meldt, og
verktøyet ventar på ny endring. Ctrl+C går til heile prosessgruppa (tenaren stoppar ryddig).
Testknappar: `NC_DEV_MAKS_MS`, `NC_DEV_STOPPFIL`. Test: `tests/test_5b_dev_reload.no`.

## nc coverage (F44)

```
nc coverage <testfil.no|mappe> [--alle] [--ut rapport.json]
```

Linjedekning per kjeldefil (dekte/køyrbare linjer, prosent, manglande linjer). Eksekveringsteljar i
`selfhost/vm.no`: `køyr_ncb_dekning` slår på `functions["__vm_dekning__"]`; utan den nøkkelen
kostar teljaren ingenting i fast-modus og éin boolsk test i standardmodus, og NCB er uendra. Teljaren
tvingar standardmodus og slår av JIT. Mappe: éi barneprosess per `test_*.no`, rapportane blir slått
saman (union). `--alle` tek med `std.`/`selfhost.`-modular. Krev ny VM (reseed) i `./bin/nc`.
Test: `tests/test_f44_dekning.no`.

## nc profile (F44)

```
nc profile <fil.no> [--topp N] [--ut rapport.json] [--trace trace.json]
```

Funksjonsprofil (kall, instruksjonar, tid ms **inkludert kalla funksjonar**; rekursive funksjonar
tel tid fleire gonger) frå VM-ens eksisterande metrikkar. `--trace` skriv Chrome-trace. Køyrer i
standardmodus utan JIT (via `køyr_ncb_dekning`). Test: `tests/test_f44_profil.no`.

## nc debug (F44)

```
nc debug <fil.no> [--bryt fil:linje]... [--kommandoar fil]
```

Stoppar ved brytepunkt (utan `--bryt`: ved start av `start()`). Kommandoar: `fortset`, `steg` (inn i
kall), `neste` (over kall), `skriv <var>`, `lokale`, `stakk`, `kjelde`, `bryt <linje>`, `fjern <linje>`,
`slutt`, `hjelp`. Bygd på `vm_debug_*` og `source_lines` i NCB; brytepunkt på ei linje blir sett på
første ip for linja (næraste linje med kode viss ho ikkje har noko). Berre `__main__`-fila. Interaktivt
krev `builtin.readline` (finst ikkje i committed seed); `--kommandoar` fungerer overalt.
Test: `tests/test_f44_debug.no`.

## nc repl (F44)

Forbetringar i `tools/nc_repl.no`: definisjonar (`la`, `funksjon`, `struktur`, `bruk`, ...) blir haldne
i økta; fleirlinjeinnskriving (opne `{ ( [` eller `\` på slutten); `:type uttrykk`; `:økt`,
`:nullstill`, `:historikk`, `!N`, `:hjelp`, `:avslutt`. Kvar evaluering køyrer økt + uttrykk som eit
nytt program (ingen delt prosesstilstand, so `la x = ...` blir omevaluert). `NORSCODE_REPL_SCRIPT=fil`
gjev innskriving frå fil (testbar). Test: `tests/test_f44_repl.no`.

## nc doc (F44)

```
nc doc <modul|fil.no|mappe> [--ut <fil.md|mappe>]
```

Markdown frå `///`-kommentarar (eller samanhengande `//`/`#`-blokk) rett over `funksjon`, `struktur`,
`grensesnitt`, `enum`, saman med signaturen slik han står i kjelda (parametertypar og returtype frå
R3), felt/variantar med trailing-kommentar, og modulomtale frå toppkommentaren. `privat` og
`_`-namn blir utelatne. Rein kjeldeskanning (kommentarar finst ikkje i AST-en). Test: `tests/test_f44_doc.no`.

## Avgrensingar

- `coverage`/`profile`/`debug` køyrer målprogrammet i ein nøsta VM (tolka) og er difor tregare enn `nc run`.
- `coverage`/`profile`/`debug` brukar `selfhost.vm`-funksjonar som ikkje finst i committed seed før reseed.
- Coverage er linjebasert (ikkje gren-/vilkårsdekning).
