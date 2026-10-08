# Typemetadata i AST og NCB (R3 spor A)

Status: **v1, avtalt format**. Skrive av spor A (typesystemet, F27/F28/F29) for
at spor B (FastAPI-binding: rutemetadata, `nc_main`, `std/web`) kan lese
deklarerte typar frå signaturar (avgjerd A1: typebinding frå signatur).

Grunnregel (avgjerd A2): typar er **erased** ved køyring. Typeannotasjonar
endrar aldri bytekoden (`code`), berre metadata. Ein funksjon eller struktur
utan nokon typeannotasjon får **ingen** ny nøkkel, så NCB-en er byte-identisk
med før for utypa kode.

## 1. Typeuttrykk (felles JSON-form)

Eit typeuttrykk er eit JSON-objekt med `kind`:

| Kjeldesyntaks              | JSON                                                                 |
|----------------------------|----------------------------------------------------------------------|
| `heltall`, `Punkt`, `T`    | `{"kind":"name","name":"heltall"}`                                   |
| `liste<heltall>`           | `{"kind":"apply","name":"liste","args":[{"kind":"name","name":"heltall"}]}` |
| `ordbok<tekst, heltall>`   | `{"kind":"apply","name":"ordbok","args":[<tekst>,<heltall>]}`        |
| `Boks<T>`                  | `{"kind":"apply","name":"Boks","args":[<T>]}`                        |
| `heltall?`                 | `{"kind":"optional","inner":<heltall>}`                              |
| `funksjon(A, B) -> C`      | `{"kind":"function","params":[<A>,<B>],"returns":<C>}`               |
| `funksjon(A)` (utan `->`)  | `{"kind":"function","params":[<A>],"returns":null}`                  |
| `funksjon` (berr)          | `{"kind":"name","name":"funksjon"}`                                  |

`name` er namnet slik det står i kjelda (ingen kanonisering: `heiltall` og
`heltall` er begge `name`). Kanonisering (t.d. `heiltall` = `heltall`,
`boolsk` = `bool`) gjer typesjekkaren, ikkje formatet.

Kvar stad eit typeuttrykk står, står òg ei **tekstform** ved sida av, laga
deterministisk frå uttrykket (ingen mellomrom):
`heltall`, `liste<heltall>`, `ordbok<tekst,heltall>`, `heltall?`,
`funksjon(heltall,tekst)->bool`, `funksjon(heltall)`. Les tekstforma om du
berre treng å vise eller samanlikne; les `*_ast` om du treng strukturen.

## 2. NCB: funksjonar

Funksjonsobjektet i `functions["<modul>.<namn>"]` (òg metodar
`"<modul>.<Struktur>.<metode>"` og lambda-hjelparar) får nøkkelen `types`
**berre** når funksjonen har minst éin parametertype, ein eksplisitt
returtype eller typeparametrar:

```json
"types": {
  "type_params": [ {"name": "T", "bound": null, "bound_ast": null} ],
  "params": [
    {"name": "x", "type": "heltall", "type_ast": {"kind":"name","name":"heltall"},
     "vararg": false, "has_default": false},
    {"name": "y", "type": null, "type_ast": null, "vararg": false, "has_default": true}
  ],
  "returns": "liste<T>",
  "returns_ast": {"kind":"apply","name":"liste","args":[{"kind":"name","name":"T"}]}
}
```

- `params` har same rekkjefølgje og lengd som `params` i funksjonsobjektet
  (namna er dei same). Utypa parameter: `type`/`type_ast` er `null`.
- `returns`/`returns_ast` er `null` når returtypen ikkje er skriven.
- `type_params` er `[]` for ikkje-generiske funksjonar. `bound` er typen i
  `<T: Samanliknbar>` / `<T implementerer Samanliknbar>` (F28 bounded generics).
- Vararg (`...rest: tekst`): `vararg: true`, `type` er den skrivne typen.

## 3. NCB: strukturar

Konstruktøren `functions["<modul>.<Struktur>"]` (har `"struct": true`) får
`types` når minst eitt felt er typa eller strukturen har typeparametrar:

```json
"types": {
  "type_params": [ {"name": "T", "bound": null, "bound_ast": null} ],
  "fields": [
    {"name": "x", "type": "heltall", "type_ast": {...}, "has_default": true},
    {"name": "v", "type": "T", "type_ast": {...}, "has_default": false}
  ]
}
```

`fields` har same rekkjefølgje som konstruktørparametrane (utan `__antal__`).

## 4. AST (for verktøy som les parser-output direkte)

Parseren (`selfhost/parser.no`) held den gamle forma og legg til nøklar:

- `Type`- og `Returtype`-noden: `verdi` er som før **grunnamnet** (`liste` for
  `liste<heltall>`). Nye nøklar: `typeuttrykk` (typeuttrykk som over, som
  Norscode-ordbok) og `typetekst` (tekstforma). Ein `Returtype` med verdi
  `tom` som parseren set inn når returtypen manglar, har ingen `typeuttrykk`.
- `Felt` i struktur: `typeuttrykk`/`typetekst` når feltet er typa (barna er
  uendra: einaste barn er framleis standardverdien).
- `Funksjon`, `Struktur`, `Grensesnitt`: `typeparametrar` = liste
  av `{"namn": "T", "grense": <typeuttrykk>|null, "grense_tekst": <tekst>}`
  når `<T, …>` er skrive (`grense_tekst` berre når grensa finst).
- `Grensesnitt`: `Metodekrav`-nodar har barna `Parametere` og `Returtype`
  (same form som `Funksjon`), og eventuelt ein `Blokk` (standardmetode).
  `utvider` = liste av grensesnittnamn frå `grensesnitt B implementerer A, …`. `Struktur.implementerer` er framleis
  fyrste grensesnitt (tekst); alle står i `implementerer_alle` (liste).

## 5. Stabilitet

Nøklane over er additive. Nye `kind`-verdiar kan kome; ein lesar skal
handsame ukjend `kind` som «ukjend type». Endringar i eksisterande nøklar
krev ny versjon av dette dokumentet.
