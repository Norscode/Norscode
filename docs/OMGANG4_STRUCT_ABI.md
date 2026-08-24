# Omgang 4 · struktur-ABI i native codegen (M5-metodar)

**Mål:** køyre struktur-konstruktørar + metode-dispatch AOT-native, byte-likt med
tolk-orakelet. To oppgåver: (1) `__struct_type__`-tagging i konstruktørane,
(2) `kall_metode`-dispatch (`<type>.<metode>`).

---

## 1. Orakelet: struktur i tolken

`selfhost/vm.no`:
- **Konstruktor** (`kall_innebygd`, `:3838`) — eit CALL til eit namn der det korte
  namnet (siste `.`-segment, `vm_kort_typenamn`) startar med **stor bokstav**
  returnerer eit **tomt kart** med éin nøkkel: `{"__struct_type__": <kort type>}`.
  Argumenta blir ignorerte. Kompilatoren emitterer `Punkt()` → `CALL "builtin.Punkt", 0`.
- **Felt** — `p.x = 3` → `INDEX_SET` (`p["x"] = 3`); `p.x` → `INDEX_GET`. Vanleg
  kart-semantikk (INDEX_SET/GET er alt native-støtta, Omgang 1).
- **Metode-dispatch** (`:5030`) — `p.sum()` → `CALL "builtin.kall_metode", argc+2`
  med mottakaren + metodenamn. VM byggjer `"<p.__struct_type__>.<metode>"` og
  kallar den funksjonen med mottakaren først (`self`).

Eksakt dispatch/suffiks-semantikk (`Punkt.sum` vs `Kvadrat.sum`) er fasit for
oppgåve 2 (`test_metode`, `test_grensesnitt`).

---

## 2. Native representasjon

Ein struktur-instans er **ingen ny tagg** — det er eit vanleg **tagg-5 kart**
(NcVal type 5) der `mapdata = [count][key0][val0]…` inneheld nøkkelen
`"__struct_type__"` (streng) → typenamn (streng), pluss felta som blir sette av
etterfølgjande `INDEX_SET`. Dette speglar tolken 1:1 (som representerer struct som
`ordbok`), så `INDEX_SET`/`INDEX_GET`/`type()` fungerer uendra.

### Oppgåve 1 — konstruktor-emittar *(GJORT)*

I CALL-builtin-halen (`macho_arm64_codegen.no`), før gap-stubben:

- `ncval_er_struct_konstruktor(kallnamn)` — kort namn (strip `builtin.`, siste
  `.`-segment) startar med `A`–`Z`. Reelle funksjonar/builtins vert sjekka FØR i
  dispatchen, så berre syntetiske konstruktørar når hit.
- Emitterer eit tagg-5 kart `mapdata = [count=1]["__struct_type__"][typenamn]`
  (to `emit_ncval_str_const` + `emit_ncval_alloc_ptr(...,5)`), ignorerer
  argumenta (djupn: `-nargs + 1`, som tolken).
- `typenamn = ncval_struct_kort(kallnamn)` (= `vm_kort_typenamn`).

Reachability: konstruktørnamnet er **ikkje** i `funksjonar` → `ncval_reachable`
følgjer det ikkje (rett — det er syntetisk, treng ingen `fn_start`).

**Strukturelt verifisert:** kjelde-codegen kompilerer ein konstruktor+felt-NCB
(`CALL builtin.Punkt` + INDEX_SET/GET) → image utan feil. Tolk-sida gjev alt
exit 42 (seed-VM-en kan konstruktørar). Maskinkode-enkoding = Docker/CI-loop.

### Oppgåve 2 — metode-dispatch *(neste)*

`CALL "builtin.kall_metode", argc+2` i native: mottakar er ein tagg-5 struct;
les `__struct_type__`-verdien (map_get), bygg `"<type>.<metode>"`, slå opp
funksjonsadressa og kall med mottakaren som første argument. Krev:
- runtime string-konkat av type + "." + metodenamn (finst: `emit_str_concat`),
- namn→adresse-oppslag i runtime (ulikt closures sin compile-tid-`lamaddr`; her
  er målet datadrive) → anten ein **metodetabell** `{"<type>.<metode>": fn_addr}`
  emittert i imaget, eller ei kjede av compile-tid-kjende kandidatar. Val + ABI
  vert fastlagt i oppgåve 2.
- `test_metode` (Punkt.sum vs Kvadrat.sum-dispatch) + `test_grensesnitt` AOT == tolk.

MERK: seed-frontenden parsar ikkje `funksjon Type.metode(...)`-syntaksen enno, så
metode-differensial via `bygg-native` ventar på seed-rebuild; konstruktør+felt
(oppgåve 1) parsar seed-frontenden alt (`tools/fixtures/diff_struct.no`).
