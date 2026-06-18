# Selfhost Fase 3 - Semantic-kjerne v1

Dette dokumentet samlar den første stabile semantic-kjernen for fase 3.

## Formål

- Gje deterministisk symboloppslag
- Hald scopes tydelege og forutsigbare
- Rapportere semantic-feil med posisjon
- Vere kompatibel med AST-kontrakten frå fase 3

## Hovudstruktur

Semantic-laget byggjer på desse delane i `selfhost/semantic.no`:

- `Symbol`
- `Scope`
- `SemanticFeil`
- `SemanticState`

## Sentral tilstand

- `ny_semantic_state()` lagar global scope og nullstiller feil
- `semantic_push_scope(state, namn)` opnar nytt scope
- `semantic_pop_scope(state)` går tilbake til foreldrescope når det finst

## Symbolreglar

- `semantic_definer(...)` skal avvise duplikat i gjeldande scope
- `semantic_finn(...)` skal leite frå gjeldande scope og oppover
- `scope_definer(...)` legg symbol inn i lokal scope-liste
- `scope_finn_lokal(...)` avgrensar søk til eit enkelt scope

## Feilreglar

- `semantic_feil(node, melding)` skal lagre melding og posisjon
- `semantic_rapporter(state, node, melding)` skal leggje feil i state
- Duplikat namn skal rapporterast som semantic-feil, ikkje krasj

## AST-kopling

Semantic-laget bruker AST-hjelparar frå `selfhost/ast.no`, særleg:

- `ast_normaliser_type(...)`
- `ast_er_uttrykk(...)`
- `ast_valider(...)`
- `ast_v1_valider(...)`

## Node-dispatch

`semantic_node(state, node)` skal rute noder deterministisk til riktige pass:

- Program
- Funksjon
- Test
- La
- Returner
- Hvis
- Match
- Await
- Mens
- Binop
- Unar
- Ident
- Kall
- Felt
- Indeks
- Slice

## Praktisk status

- `semantic_analyser(program)` byggjer state og køyrer semantic-pass
- `semantic_analyser_rent(program)` er ei enkel bru for rein analyse
- Semantic-laget har allereie struktur for scope, symbol og feil

## Vidare utvikling

- Typeanalyse kan bli strengare utan å endre grunnstrukturen
- Flere pass kan leggjast til så lenge status og feilmeldingar er deterministiske
- Fase 3 kan utvide semantic-reglar utan å bryte AST-kontrakten
