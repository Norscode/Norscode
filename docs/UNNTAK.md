# Unntak: fleire fang, hierarki, ressursar og stakkspor (R4: F32, F33, F34)

Status: implementert i VM-vegen (kjelde-kjeda `.no` -> NCB -> `selfhost/vm.no`).
AOT (x86/arm64) er ikkje med, sjå "AOT" nedst.

Unntaksverdiar er uendra frå før: ein tekst `"Type: melding"`, ei ordbok med
`type`-felt, eller ein struktur-instans (`__type__`). Typen til eit unntak er
teksten før fyrste `:` (tekst), `__type__`/`type` (ordbok/struktur). Alt under
er bygd oppå dette, utan å endre kva `e` er i ei vanleg `fang (e)`-blokk.

## 1. Fleire fang-klausular og multi-catch (F32)

```
prøv {
    arbeid()
} fang (e: TypeFeil eller IndeksFeil) {   // multi-catch: eller skil typane
    ...
} fang (e: ArgumentFeil) {                // fleire klausular på same prøv
    ...
} fang (e) {                              // fang-alt (valfri, sist)
    ...
} endeleg {
    ...
}
```

- Første klausul (ovanfrå) som passar vinn. Resten blir ikkje prøvd.
- Ingen klausul passar (og ingen fang-alt): unntaket går vidare til ytre
  handler, med `endeleg` kjørt ein gong på vegen. Stakksporet frå kast-staden
  blir behalde.
- Eit unntak som blir kasta INNE i ei klausul blir ikkje fanga av ei
  søsterklausul (berre av ytre prøv).
- Same variabelnamn i fleire klausular er fint. `bryt`, `fortsett`, `returner`
  frå ei klausul verkar som før.
- `fang (e: A eller B)` med ei einskild klausul blir ein TRY_BEGIN med
  `catch_type = "A|B"`; fleire klausular blir senka i parseren til ei
  fang-alt-klausul med ei `hvis unntak_passar(e, "A|B")`-kjede og
  `unntak_behald_stakk()` + `kast e` når ingen passar. Typesjekk, namnesjekk,
  linter og bytekode-emitteren ser berre vanlege noder.
- Enkel `fang (e: Type)` og `fang (e)` gjev byte-identisk NCB som før (verifisert
  mot 15 NCB-ar frå fiksturar og modular).

## 2. Unntakshierarki

Rota er det innebygde namnet `Unntak`. `fang (e: Unntak)` fangar alle verdiar
(og `er_instans(e, Unntak)` er sant for alle unntak).

```
Unntak
 |- TypeFeil            (aritmetikk/operator på feil typar, sett_inn på ikkje-liste)
 |- VerdiFeil
 |   |- DivisjonMedNull
 |   |- ModuloMedNull
 |   `- ValideringsFeil
 |- IndeksFeil          (indeks utanfor liste)
 |- ArgumentFeil        (aritetsfeil, feil tal argument til innebygd)
 |- MetodeFeil          (ukjend metode / metode på ingenting)
 |- FilFeil
 |- FrosenFeil          (skriving til ei frosen samling, sjå FROSNE_SAMLINGAR.md)
 |- GeneratorFeil       (neste() på ein generator som køyrer, sjå GENERATORAR.md)
 |- HostFeil
 |   |- HostTypeFeil, HostHandleFeil, HostMetodeFeil
 `- FutureFeil
     `- TimeoutFeil, AvbruttFeil
```

`fang (e: VerdiFeil)` fangar altså `1 / 0` (DivisjonMedNull) men ikkje
`"a" - 1` (TypeFeil). Hierarkiet er eit fast kart i `vm_unntak_forelder`
(`selfhost/vm.no`) og i `er_innebygd_unntak` (`ir_to_bytecode.no`, for
`er_instans`). Gamle namn som ikkje står i kartet fangast som før på eksakt
typenamn (og av `Unntak`).

### Brukartypar: grensesnitt, ikkje arv (A4)

Roten for eigne unntak blir uttrykt med eit vanleg `grensesnitt`. Ein struktur
som implementerer grensesnittet blir fanga av `fang (e: Grensesnitt)`:

```
grensesnitt FeilRot { funksjon kode(self) -> heiltall }
struktur ValFeil implementerer FeilRot { melding }
...
prøv { kast ny_val("x") } fang (e: FeilRot) { ... e.kode() ... }
```

Kompilatoren løyser grensesnittnamnet til modulkvalifisert namn
(`modul.FeilRot`) i `catch_type`; VM-en testar `implementerer`-lista til
strukturen (same mekanikk som `er_instans`). Alt anna står som før.

### Hjelparar

| Kall | Resultat |
|---|---|
| `er_instans(e, Unntak \| TypeFeil \| … \| Grensesnitt \| Struktur)` | sant/usant |
| `unntak_type(e)` | unntakstypen som tekst (`"TypeFeil"`, `"ValFeil"`) |
| `typenamn(e)` | strukturnamn for struktur-unntak; unntakstype for pakka unntak; ellers runtime-typen (`tekst` for tekst-unntak) |
| `unntak_pakk(e)` | ordbok med `type`, `melding`, `tekst` (originalen), `stakk`, `undertrykte` |

## 3. try-with-resources (F33)

```
prøv med a = opne_a(), b = opne_b() {
    ...
} fang (e) {
    ...
} endeleg {
    ...
}
```

- Ressursane blir opna i rekkjefølgje, `lukk()` blir kalla garantert i OMVEND
  rekkjefølgje (b, så a), òg ved `returner`, `bryt`, `fortsett` og unntak.
  `fang`/`endeleg` er valfrie; utan dei er `prøv med … { }` ei vanleg setning.
- `fang`/`endeleg` omsluttar ressursane: feil frå `opne_b()` fangast av
  brukarens `fang`, og `a` blir lukka først.
- Ressurs som er `ingenting` blir ikkje lukka (som Java).
- Kastar kroppen OG `lukk()`: kropps-unntaket er hovudunntaket; lukk-unntaket
  blir undertrykt og lagt til `e.undertrykte` (i den rekkjefølgja lukkinga
  skjedde). Er `e` ein rå tekst, blir han pakka (`unntak_pakk`) så
  `e.melding`, `e.stakk`, `e.undertrykte` finst; `typenamn(e)` og
  `er_instans(e, …)` verkar på det pakka unntaket.
- Kastar berre `lukk()`: lukk-unntaket går vidare som hovudunntak (og
  undertrykker ikkje noko). Kastar `lukk()` medan kroppen gjer `returner`, vinn
  lukk-unntaket.
- Utan feil i lukkinga blir hovudunntaket kasta vidare uendra (rå tekst/struktur).
- Senking (parseren): kvar ressurs blir `la r = uttrykk` + ei `prøv/fang/endeleg`
  rundt resten, med hjelpevariablar `__u*_N__`.

## 4. Stakkspor (F34)

`e.stakk` er ei liste av ordbøker `{funksjon, fil, linje}`, innerste ramme
fyrst (maks 64 rammer). Felta har nøkkelnamn `"funksjon"`, `"fil"`, `"linje"`;
`funksjon` er eit nøkkelord, så bruk `p["funksjon"]`.

- Sporet blir berre BYGD VED KAST (`vm_registrer_kast`, i `THROW` og i
  feilvegen for innebygde kall). Kode som ikkje kastar har ingen ekstra
  kostnad, og NCB-en er uendra. Linjenummera kjem frå `source_lines` per
  instruksjon som alt låg i NCB-en (parser -> IR -> bytecode).
- `e.stakk` og `e.undertrykte` finst når fang-blokka les dei på fangvariabelen
  (`e.stakk`, `e.undertrykte`): parseren set då inn `e = unntak_pakk(e)` fyrst i
  blokka. Elles er `e` den rå verdien som før, så eksisterande kode er uendra.
- Rethrow av eit pakka unntak (`kast e` der `e` er pakka) bevarer det
  opphavlege sporet. Ein rå tekst som blir kasta på nytt får eit nytt spor frå
  den nye kast-staden. Automatisk rethrow når ingen klausul passar, og
  ressurs-lukking, bevarer sporet.
- Halekall (`returner f()`) gjenbruker rammer og står ikkje i sporet.
- `fil` er modulstien frå modulnamnet (`selfhost.vm` -> `selfhost/vm.no`).
  Hovudmodulen har ikkje filnamn i NCB-en og heiter `__main__` (verten kan setje
  `functions["__vm_hovudfil__"]`).
- Ufanga unntak: `nc run` (nc_main) skriv til stderr

```
ERROR: EigenFeil: djupt
    ved a (__main__:2)
    ved b (__main__:5)
    ved start (__main__:8)
```

  Fyrste linje er som før (R1 B, `vm_ufanga_tekst`), sporet kjem etter.
  Verten kallar `vm_skriv_ufanga_ncb(feil, ncb["functions"])`. `vm_executor.no`
  kallar framleis `vm_skriv_ufanga` (utan spor) til executor-NCB-en blir
  regenerert.

## 5. AOT (x86/arm64): ikkje med

AOT-kompilatorane kompilerer ikkje feil stille:

- `fang (e: Unntak)` blir fang-alt (rett semantikk).
- Multi-catch (`A|B`), grensesnitt-fang (`modul.Grensesnitt`) og fang på
  ikkje-bladtypane `VerdiFeil`, `HostFeil`, `FutureFeil` gjev ein
  kompileringsfeil `NATIV-GAP: fang-typen …`.
- `unntak_passar`, `unntak_pakk`, `unntak_behald_stakk`, `unntak_type` er
  fail-closed: kallet kastar `NATIV-GAP:<namn>` ved køyring (fangbart). Det
  råkar fleire fang-klausular (kjeda), `e.stakk`/`e.undertrykte` og
  `prøv med` (som brukar `unntak_pakk`/`unntak_behald_stakk` på unntaksvegen;
  vegen utan unntak køyrer).
- Eksakt typa fang (`fang (e: TypeFeil)`) og ikkje-bladtypar som ikkje er
  nemnde over verkar som før, utan hierarki (AOT samanliknar typenamn eksakt).
- Ufanga unntak i AOT skriv ikkje stakkspor.

## 6. Filer

`selfhost/parser.no` (syntaks og senking), `selfhost/compiler/ir_to_bytecode.no`
(grensesnitt-løysing i fang, `er_instans` mot hierarkiet),
`selfhost/vm.no` (hierarki, matching, pakking, stakk, ufanga-rapport),
`selfhost/nc_main.no` (stakkspor ved ufanga),
`selfhost/native_execution/native_codegen_v2.no` og `macho_arm64_codegen.no`
(AOT-gap). Testar: `tests/test_r4_unntak_fleire.no`, `…_ressurs.no`,
`…_stakk.no` (fiksturar i `tests/fixtures/r4_*.no`).
