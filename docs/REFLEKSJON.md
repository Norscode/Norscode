# Refleksjon og annotasjonar (F43)

Status: VM-vegen (kjelde-kjeda). AOT/WASM: sjå seksjon 5.

## 1. Annotasjonar

```
@rute("GET", "/vare/{id}")
@lat
@utdatert("bruk nyFunksjon")
funksjon hent_vare(id: heltall) -> tekst { … }

@modell("vare")
struktur Vare {
    @nokkel
    id: heltall
    @felt("visningsnamn", 3)
    namn: tekst = "ukjend"
}
```

- `@namn` eller `@namn(lit, lit, …)` står rett framfor ein funksjon (inkl.
  `Type.metode`, `privat`/`asynk`), ein struktur eller eit strukturfelt. Fleire
  annotasjonar kan stå etter kvarandre (same namn fleire gonger er lov).
- Argument er berre literalar: tekst, heiltal, desimaltal (`-2.5`), `sann`,
  `usann`, `null`. Heksadesimale tal og uttrykk er ein kompileringsfeil.
- Annotasjonar er **rein metadata**: dei endrar aldri koden som blir generert.
  `@` finst elles ikkje i språket (utanfor tekst og kommentar), so syntaksen
  kolliderer ikkje med eksisterande kode.
- Feil: annotasjon framfor noko anna enn funksjon/struktur
  («annotasjonar kan berre stå framfor …»), ikkje-literalt argument,
  annotasjon utan deklarasjon på slutten av fila.

## 2. NCB-format (additivt)

Kode utan annotasjonar gjev **uendra NCB**; nøklane finst berre når det er
annotasjonar.

| Stad | Nøkkel | Form |
|---|---|---|
| `functions["modul.f"]` | `annotasjonar` | `[{"name":"rute","args":["GET","/vare/{id}"]}, …]` |
| `functions["modul.Struktur"]` (konstruktøren) | `annotasjonar` | same form |
| `functions["modul.Struktur"]` | `felt_annotasjonar` | `{"felt": [{"name":…,"args":[…]}, …]}` (berre annoterte felt) |

Typemetadata (`types`, sjå TYPEMETADATA.md) er uendra. AST: `annotasjonar` på
`Funksjon`/`Struktur`/`Felt`-nodar, liste av
`{"namn", "argument":[{"type","verdi"}], "linje"}`. Token: `AT` (`@`).

## 3. Refleksjons-API (køyretid, VM)

Namna er vanlege innebygde kall (ingen import).

| Kall | Resultat |
|---|---|
| `typar_av(x)` | typeinfo for instansen `x` (struktur). For ikkje-struktur: minimal info (`namn`, `struktur: usann`, tomme lister). |
| `type_info(T)` | typeinfo for strukturen `T`. `T` kan vere eit namn (`type_info(Vare)`, `type_info(alias.Vare)` – løyst ved kompilering), ein tekst (`"Vare"` eller `"modul.Vare"`) eller ein instans. |
| `funksjon_info(f)` | funksjonsinfo. `f` kan vere eit namn (`funksjon_info(hent_vare)` – løyst ved kompilering), ein tekst (`"hent_vare"`, `"modul.f"`, `"Type.metode"`) eller ein closure/lambda. |
| `finn_med_annotasjon("rute")` | liste over alle deklarasjonar i alle lasta modular med annotasjonen. |
| `hent_felt(obj, "namn")` | verdien av feltet. Struktur utan feltet: `RefleksjonFeil`. Vanleg ordbok: `null` om nøkkelen manglar. |
| `sett_felt(obj, "namn", v)` | set feltet, gjev `obj`. Struktur utan feltet: `RefleksjonFeil`. |

Typeinfo (ordbok):
`namn`, `fullt_namn` (`modul.Namn`), `modul`, `struktur`, `type_parametrar`,
`annotasjonar`, `grensesnitt` (modulkvalifiserte, med forfedrar), `metodar`
(namn utan `Type.`), og `felt` = liste av
`{namn, type, har_standard, standard, annotasjonar}`. `type` er tekstforma frå
`types` (`null` for utypa felt). `standard` er verdien konstruktøren gjev når
argumentet manglar (konstruktøren blir køyrt utan argument; `null` utan
standard). Utan typemetadata blir `har_standard` utleidd av `standard != null`.

Funksjonsinfo (ordbok):
`namn`, `fullt_namn`, `modul`, `er_async`, `type_parametrar`, `returtype`
(tekst eller `null`), `annotasjonar`, og `parametrar` = liste av
`{namn, type, har_standard, vararg}`. Standardverdiar til parametrar blir
evaluerte på kallstaden og ligg difor ikkje i NCB: berre `har_standard`.

Annotasjon (ordbok): `{namn, argument:[verdiar]}`.

`finn_med_annotasjon`-oppføring: `{art, namn, fullt_namn, modul, annotasjon,
annotasjonar}` – éi oppføring per (deklarasjon, treffande annotasjon), sortert
på `fullt_namn`. `art` er `funksjon`, `metode`, `struktur` eller `felt` (felt
har òg `struktur`). `@` framfor namnet er valfritt (`"@rute"`). Interne
funksjonar (`__module_init__`, lambda-hjelparar) er ikkje med.

Avgrensingar:
- Refleksjon ser **ikkje** `privat`: `hent_felt`/`sett_felt` omgår
  kompileringstids-sjekken for private felt.
- Funksjonar er ikkje førsteklasses verdiar i språket (berre closures), difor
  tek `funksjon_info` namn (kompileringstids-løyst eller tekst), ikkje ein
  «funksjonsreferanse».
- Berre toppnivå-strukturar har `type_info`; grensesnitt/enum har ikkje det.

## 4. Døme: `@rute` via refleksjon

`tests/fixtures/f43_rute_app.no` registrerer handlarar utan eit eige
rutetabell-API:

```
@rute("GET", "/vare/{id}")
funksjon hent_vare(id: heltall) -> tekst { … }

funksjon send(metode: tekst, sti: tekst) -> tekst {
    la rutene = finn_med_annotasjon("rute")
    … # for kvar: a = rutene[i]["annotasjon"]["argument"]; match mønster `a[1]`
    … # builtin.ncb_call_fn(rutene[i]["fullt_namn"], args…)
}
```

apptenar/serve er **ikkje** endra; `web.route`-metadata (R3-signaturbinding,
`route_handlers`) er uavhengig av annotasjonar og urørt.

## 5. AOT / WASM

- x86-64 AOT (`native_codegen_v2`) og arm64 (`macho_arm64_codegen`): kalla til
  dei seks refleksjonsbuiltins blir *fail-closed*: bygget går, kallet kastar
  `NATIV-GAP:<namn>` ved køyring (fangbart). Annotasjonar i NCB blir ignorerte
  (ingen effekt på AOT-kode). Dette krev reseed/ny codegen-ELF for å verke i
  `bin/nc`; kjelde-endringa ligg i `er_refleksjon_builtin` og `ncval_gap_stub`.
- WASM (`std/wasm_lowring.no`): ukjende builtins gjev alt kompileringsfeil
  (ikkje stille feil).
- Lexer/parser/ir-endringane (`@`-syntaks, `annotasjonar`-nøklar) er i
  seed-lukkinga, men verkar i kjelde-kjeda utan reseed; `./bin/nc` (stage-0)
  får dei først ved reseed.

## 6. Filer og testar

`selfhost/lexer/lexer_m1.no` (token `AT`), `selfhost/parser.no`
(`les_annotasjon`, `ann_fest`), `selfhost/compiler/ir_to_bytecode.no`
(`ann_json`, `ann_felt_json`, kompileringstids-løysing av `type_info`/
`funksjon_info`), `selfhost/vm.no` (`vm_refl_*`), native codegen-stubbar.
Test: `tests/test_f43_refleksjon.no` med `tests/fixtures/f43_refl_app.no` og
`tests/fixtures/f43_rute_app.no`.
