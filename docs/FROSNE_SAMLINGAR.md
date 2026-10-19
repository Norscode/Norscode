# Uforanderlege samlingar: `frys`, `er_frosen`, `tin` (F46)

Status: implementert i VM-vegen (kjelde-kjeda `.no` -> NCB -> `selfhost/vm.no`).
AOT (x86/arm64) er ikkje med, sjå "AOT" nedst.

```
la f = frys([1, [2, 3]])      # djup kopi, frosen
f[0]                          # 1
lengde(f)                     # 2
for x i f { ... }             # iterasjon som vanleg
f == [1, [2, 3]]              # sann (likskap som vanleg)
er_frosen(f)                  # sann
er_frosen(f[1])               # sann (heile treet er frose)

f[0] = 9                      # kastar FrosenFeil
legg_til(f, 4)                # kastar FrosenFeil
f[1][0] = 9                   # kastar FrosenFeil (djupt)

la m = tin(f)                 # mutabel djup kopi, f er uendra
legg_til(m, 4)                # ok
```

- `frys(x)` gjev ei frosen DJUP kopi av ei liste, ordbok eller struktur-instans.
  Originalen blir ikkje endra og forblir mutabel. Alle nøsta samlingar er òg
  frosne. `frys` av ei allereie frosen samling gjev same verdi. Skalarar
  (heiltal, tekst, bool, ingenting), closures og generatorar blir returnerte
  uendra.
- `er_frosen(x)`: `sann` for frosne samlingar og for skalarar (dei kan ikkje
  endrast); `usann` for vanlege lister/ordbøker/strukturar.
- `tin(x)`: ei mutabel djup kopi (fungerer også på vanlege samlingar).
- Skriving kastar `FrosenFeil` (under `Unntak`, sjå UNNTAK.md; kan fangast med
  `fang (e: FrosenFeil)`): `x[i] = v`, `x.felt = v`, og mutasjons-innebygde
  (`legg_til`, `pop_siste`/`fjern_siste`, `fjern`, `sett_inn`, `fjern_nokkel`,
  `liste.fjern*`, `liste.sett_inn*`, `liste.omvendt*`, …). Ein metode på ei
  frosen struktur kan lese felta (`self.x`), men skriving til `self` kastar.
- Lesing, indeksering, `lengde`, `for`, `==`, `tekst()`/`skriv` og
  ikkje-muterande innebygde fungerer som for vanlege samlingar.
  `builtin.type(frys([1]))` er `"liste"`. Likskap mellom ordbøker følgjer
  elles vanleg VM-semantikk (ordbok == ordbok utan strukturar er ikkje
  innhaldslikskap), også for frosne ordbøker.
- Trådsikkerheit: ei frosen samling kan ikkje endrast av nokon, så ho kan
  delast fritt mellom (grøne) trådar og generatorar utan lås. Ho blir ikkje
  kopiert ved deling, berre ved `frys`/`tin`.

## Implementering

Ei frosen samling er ei boks `{"__frosen__": djup kopi}` (nøsta samlingar er
sjølve bokser). VM-en pakkar ut boksen ved `INDEX_GET`, `LOAD_FIELD`,
metodeoppslag og for innebygde funksjonar (`vm_frosen_args`), og kastar
`FrosenFeil` ved `INDEX_SET`, `STORE_FIELD` og mutasjons-innebygde. Alt er
styrt av eit globalt flagg (`vm_spesial_aktiv`) som først blir slått på av
fyrste `frys`/generator, så vanleg kode betaler ingenting ekstra. Bytekode
for kode utan `frys` er uendra.

Avgrensing: mutasjon som går via ein brukarfunksjon som skriv til ein
parameter kastar òg (INDEX_SET/STORE_FIELD), men ein innebygd funksjon som
mutterer ein ANNAN stad enn første argument blir ikkje fanga.

## AOT

`frys`, `er_frosen` og `tin` er fail-closed i x86- og arm64-kodegen
(`NATIV-GAP:` unntak ved køyring). Dette verkar først i den pinna kodegen-ELF-en
etter regenerering (reseed); før det blir kallet ei stille null-fallback i
AOT-bygde program. WASM har ingen støtte.
