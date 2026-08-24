# JavaScript-paritet i Norscode

Fire std-modular gir dei innebygde JavaScript-funksjonane som Norscode-funksjonar.
Alt er skrive i Norscode — ingen JavaScript-motor og ingen skriptlag er involvert.

| Modul | Dekker | Test |
|-------|--------|------|
| `std/js_liste.no` | `Array.prototype.*` | `tests/test_js_liste.no` |
| `std/js_streng.no` | `String.prototype.*` | `tests/test_js_streng.no` |
| `std/js_tal.no` | `Number.*`, `Math.*`, `parseInt`, `parseFloat` | `tests/test_js_tal.no` |
| `std/js_objekt.no` | `Object.*`, spread, `delete`, `??` | `tests/test_js_objekt.no` |

```norscode
bruk std.js_liste som jsl
bruk std.js_streng som jss
bruk std.js_tal som jst
bruk std.js_objekt som jso
```

## Array — `std/js_liste.no`

| JavaScript | Norscode |
|------------|----------|
| `arr.at(i)` | `jsl.ved(l, i)` |
| `arr.slice(a, b)` | `jsl.skiv(l, a, b)` / `jsl.skiv_frå(l, a)` |
| `arr.splice(a, n, ...v)` | `jsl.spleis(l, a, n, v)` |
| `arr.concat(b)` | `jsl.konkat(a, b)` |
| `arr.join(sep)` | `jsl.sett_saman(l, sep)` |
| `arr.indexOf` / `lastIndexOf` / `includes` | `jsl.indeks_av` / `jsl.siste_indeks_av` / `jsl.inkluderer` |
| `arr.toReversed()` | `jsl.snu(l)` |
| `arr.flat()` / `arr.flat(n)` / `arr.flatMap(fn)` | `jsl.flat` / `jsl.flat_djup` / `jsl.flat_kart` |
| `arr.fill(v)` / `arr.keys()` / `arr.entries()` | `jsl.fyll` / `jsl.nøklar` / `jsl.oppføringar` |
| `arr.push` / `pop` / `unshift` / `shift` | `jsl.legg_bak` / `jsl.pop` / `jsl.legg_framfor` / `jsl.utan_første` |
| `arr.forEach` / `map` / `filter` / `reduce` / `reduceRight` | `jsl.for_kvar` / `kart` / `filtrer` / `reduser` / `reduser_høgre` |
| `arr.find` / `findLast` / `findIndex` / `some` / `every` | `jsl.finn` / `finn_siste` / `finn_indeks` / `nokre` / `alle` |
| `arr.toSorted(cmp)` | `jsl.sorter_med(l, cmp)` — stabil |

## String — `std/js_streng.no`

Alle posisjonar er **teiknposisjonar**, ikkje bytar, så `æøå` oppfører seg som i JavaScript.

| JavaScript | Norscode |
|------------|----------|
| `s.length` | `jss.lengde_teikn(s)` (`jss.lengde_bytes(s)` for bytar) |
| `[...s]` | `jss.teikn(s)` |
| `s.at(i)` / `s.charAt(i)` | `jss.ved(s, i)` / `jss.teikn_ved(s, i)` |
| `s.codePointAt(i)` / `String.fromCodePoint(k)` | `jss.kodepunkt_ved` / `jss.frå_kodepunkt` |
| `s.slice(a, b)` / `s.substring(a, b)` | `jss.skiv` / `jss.del_streng` |
| `s.includes` / `startsWith` / `endsWith` | `jss.inkluderer` / `startar_med` / `sluttar_med` |
| `s.indexOf` / `lastIndexOf` | `jss.indeks_av` / `jss.siste_indeks_av` |
| `s.replace` / `replaceAll` | `jss.erstatt` / `jss.erstatt_alle` |
| `s.split(sep)` / `s.split(sep, n)` | `jss.del` / `jss.del_grense` |
| `s.trim` / `trimStart` / `trimEnd` | `jss.trim` / `trim_start` / `trim_slutt` |
| `s.padStart` / `padEnd` / `repeat` | `jss.fyll_start` / `fyll_slutt` / `gjenta` |
| `s.toUpperCase` / `toLowerCase` | `jss.til_store` / `til_små` |
| `a.localeCompare(b)` | `jss.samanlikn(a, b)` |

## Number og Math — `std/js_tal.no`

`std/math.no` reknar berre med heiltal. Denne modulen reknar med desimaltal.

| JavaScript | Norscode |
|------------|----------|
| `a / b` | `jst.del(a, b)` — `del(7, 2)` er `3.5` |
| `Math.trunc` / `floor` / `ceil` / `round` | `jst.trunker` / `golv` / `tak` / `rund` |
| `Math.abs` / `Math.sign` | `jst.absolutt` / `jst.signum` |
| `Math.min` / `Math.max` | `jst.min` / `jst.maks` (og `min_av` / `maks_av` over lister) |
| `Math.pow` / `Math.sqrt` | `jst.potens` / `jst.kvadratrot` |
| `x.toFixed(n)` | `jst.til_fast(x, n)` |
| `parseInt` / `parseFloat` | `jst.parse_heltall` / `jst.parse_desimal` (+ `_eller`-variantar) |
| `!isNaN(Number(s))` | `jst.er_talstreng(s)` |
| `Number.isInteger` | `jst.er_heiltal` |

## Object — `std/js_objekt.no`

| JavaScript | Norscode |
|------------|----------|
| `Object.keys` / `values` / `entries` | `jso.nøklar` / `verdiar` / `oppføringar` |
| deterministisk rekkjefølgje | `jso.nøklar_sortert` / `jso.oppføringar_sorterte` |
| `Object.fromEntries` | `jso.frå_oppføringar` |
| `"k" in o` | `jso.har(o, "k")` |
| `o.k ?? standard` | `jso.hent(o, "k", standard)` |
| `{...o}` | `jso.kopi(o)` |
| `Object.assign(a, b)` | `jso.tildel(a, b)` |
| `{...a, ...b}` | `jso.slå_saman(a, b)` |
| `delete o.k` | `jso.utan(o, "k")` |
| `Object.keys(o).length` | `jso.antal(o)` |

## Callback-konvensjonen

Norscode har ikkje metodesyntaks, og lambdaer er enno ikkje køyrbare i VM-en
(`BUILD_LAMBDA` er ikkje implementert). Callbacks blir difor sende som
funksjonsnamn, same konvensjon som `std/funktools.no`:

```norscode
bruk std.js_liste som jsl

funksjon dobbel(x: heltall) -> heltall {
    returner x * 2
}

funksjon start() -> heiltall {
    la ut = jsl.kart([1, 2, 3], "dobbel")   # [2, 4, 6]
    returner 0
}
```

Når `BUILD_LAMBDA` og `CALL_VALUE` kjem inn i `selfhost/vm.no`, kan dei same
funksjonane ta imot lambdaverdiar direkte utan at API-et endrar seg.

## Kjende skilnader frå JavaScript

- **Ingen `null`/`undefined`.** `ingenting` er 0 i praksis. Funksjonar som kan
  «ikkje finne noko» har difor indeksvariantar (`finn_indeks`, `har_indeks`,
  `indeks_av`) som du bør bruke når 0 er ein gyldig verdi.
- **Ingen `NaN` eller `Infinity`.** `jst.del(x, 0)` kastar unntak i staden for å
  gi `Infinity`, og `jst.parse_heltall("abc")` gir 0 — bruk
  `jst.er_talstreng` eller `parse_heltall_eller` når du må skilje.
- **Muterande metodar er ikkje-muterande.** `spleis`, `snu`, `sorter_med`,
  `legg_framfor` og `utan_første` returnerer nye lister. `legg_bak` og `pop`
  er dei einaste som endrar lista, som i JS.
- **`til_fast` rundar desimalt, ikkje binært.** `jst.til_fast(1.005, 2)` gir
  `"1.01"`, medan JavaScript gir `"1.00"` på grunn av flyttalsrepresentasjonen.
- **Nøkkelrekkjefølgje er ikkje garantert.** JavaScript held på
  innsetjingsrekkjefølgja for strengnøklar; Norscode-køyrarane gjer ikkje det
  (`nc run` og `nc test` kan gi ulik rekkjefølgje). Bruk `jso.nøklar_sortert`
  eller `jso.oppføringar_sorterte` når rekkjefølgja betyr noko.
- **`samanlikn` samanliknar byte-orden**, ikkje språkspesifikk kollasjon slik
  `localeCompare` gjer med locale.
