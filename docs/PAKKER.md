# Pakkehandtering med register (`nc pkg`)

Rein Norscode: `tools/nc_pkg.no` (CLI), `tools/pkg_kjerne.no` (kjerne). Test: `tests/test_pkg_register.no`.
Dette erstattar ikkje den eldre `toolchain/norspkg/`-selftesten (`tests/test_norspkg.no`); det er ein eigen,
ende-til-ende-testbar flyt med register, sjekksum-låsefil og transitiv løysing.

## Kommandoar

| Kommando | Verknad |
|---|---|
| `nc pkg init [namn]` | oppretta `norcode.toml` med `[project]` og `[dependencies]` |
| `nc pkg add <namn>[@krav]` | legg til avhengnad (utan krav: `^<nyaste>`), løys, skriv `norscode.lock`, installer |
| `nc pkg remove <namn>` | fjernar avhengnad (og transitive som ikkje trengst lenger) |
| `nc pkg install [--frozen]` | installerer EKSAKT etter `norscode.lock`; manglar/er lock utdatert blir ho laga på nytt (`--frozen`: feil i staden) |
| `nc pkg update [namn]` | løys på nytt til nyaste tillatne versjonar (med namn: berre den pakka, resten held låst versjon) |
| `nc pkg publish` | pakkar katalogen (NKA1-arkiv), lastar til registeret |
| `nc pkg search [tekst]` | lister pakkar og nyaste versjon |
| `nc pkg register-serve <mappe> [--port N] [--host H]` | referanse-registertenar |
| `nc pkg env` | skriv stien som skal i `NORSCODE_PATH` (`.norscode/pakkar`) |

Register: `--register <url|mappe>`, elles `NORSCODE_PKG_REGISTER`, elles `[registry] url = "..."` i `norcode.toml`.
Registeret er anten ei mappe (også `file://`) eller `http://host:port`.

## Manifest og krav

```toml
[project]
name = "min_app"
version = "0.1.0"

[dependencies]
a = "^1.2.0"          # >=1.2.0 <2.0.0   (^0.2.3 = >=0.2.3 <0.3.0)
b = "~1.4.0"          # >=1.4.0 <1.5.0
c = ">=1.0.0, <2.0.0" # område (komma eller mellomrom)
d = "1.2.3"           # nøyaktig;  "1.2" = >=1.2.0 <1.3.0
lokal = "./packages/lokal"   # sti-avhengnad: urørt av nc pkg
```

Versjonar er `x.y.z` (tal samanliknast numerisk; ingen pre-release). Pakkenamn: `a-z`, `0-9`, `_`.

## Løysing

Backtracking-løysar: høgaste versjon som oppfyller alle krav, transitive avhengnadar frå registerindeksen,
låst versjon blir føretrekt. Ingen løysing gir konfliktmelding, t.d.
`versjonskonflikt for «b»: krava ^1.0.0 (frå a@1.0.0), ^2.0.0 (frå c@1.0.0) har ingen felles versjon; tilgjengeleg: 1.0.0, 1.1.0, 2.0.0`.
Syklar gir `syklus i avhengnadar: x -> y -> x`. Ved feil blir `norcode.toml` rulla tilbake og låsefila rørt ikkje.

## Låsefil `norscode.lock`

Deterministisk (pakkar sorterte på namn, ingen tidsstempel): `namn`, `versjon`, `sha256` av arkivet, `arkiv`
(sti i registeret) og `krav` (krava pakka sjølv stiller). `install` følgjer låsefila og verifiserer SHA-256 av
kvart arkiv (cache eller nedlasta) FØR noko blir installert. Feil sjekksum er hard feil
(`sjekksumfeil for b@1.2.0: ...`); ein skadd cachefil blir kasta og henta på nytt, og verifisert på nytt.

## Installasjon og modulløysing

Pakkar går til `.norscode/pakkar/<namn>/` (vendor-mappe; cache i `.norscode/cache/`). Importer med
`bruk <namn>.<modul>` ved å ha `.norscode/pakkar` i `NORSCODE_PATH` (F42-mekanikken, same som kompilatoren og
`nc run/test/serve`):

```
export NORSCODE_PATH=$PWD/.norscode/pakkar
nc run app.no      # bruk a.rot  →  .norscode/pakkar/a/rot.no
```

## Registerformat

```
<register>/indeks.json        {"pakkar": {"<namn>": {"versjonar": {"<x.y.z>":
                                 {"arkiv": "arkiv/<namn>-<x.y.z>.nka", "sha256": "...",
                                  "avhengnader": {"<dep>": "<krav>"}}}}}}
<register>/arkiv/<namn>-<x.y.z>.nka
```

Arkivformat NKA1 (tekst, tar-liknande, deterministisk): `NKA1`, deretter `FIL <byte-lengd> <sti>` + innhald, sortert
på sti, `SLUTT`. Utpakking avviser `..`, absolutte stiar og feil lengd. **Avgrensing:** berre tekstfiler (kjeldekode,
toml, md). Publisering hoppar over `.`-filer, `build/`, `node_modules/` og `norscode.lock`. Utgjevingar er
uforanderlege (ny publisering av same versjon = 409/duplikat).

HTTP-protokoll (referansetenaren `register-serve`): `GET /indeks.json`, `GET /arkiv/<fil>`,
`PUT /arkiv/<namn>/<versjon>` (arkivet som kropp; svar 201, 409 duplikat, 400 ugyldig, 401 utan token).
Set `NORSCODE_PKG_REGISTER_TOKEN` på tenaren for å krevje `Authorization: Bearer <token>` ved publisering;
klienten sender `NORSCODE_PKG_TOKEN`. Berre `http://` (ingen TLS) i klienten; bruk ein TLS-front for offentleg register.
Tenarlogikken (`register_handter`) er ein rein funksjon som lett kan hengast på `nc serve`/std.web.

## Sikkerheit

SHA-256 blir alltid verifisert mot låsefila. Ed25519-signering av utgjevingar er IKKJE implementert
(`std/ed25519.no` finst; indeksen kan utvidast med eit `signatur`-felt). Sjekksummen i låsefila er rotfestet
til første løysing, så ein kompromittert register kan ikkje bytte arkiv i hemmelegheit, men første `add` stolar på registeret.

## Integrasjon

`nc pkg` er eit verktøyprogram (`tools/nc_pkg.no`) med tre små linjer i `selfhost/nc_main.no`
(`nc_direkte_verktøysti`, `nc_direkte_bruk`, `nc_forbered_direkte_miljø`; argumenta går via `NORSCODE_PKG_ARGV`).
Dispatchen verkar i ein ferdig `./bin/nc` først etter reseed; til då: `NORSCODE_PKG_ARGV=$'add\na' ./bin/nc run tools/nc_pkg.no`.
