# Selfhost Fase 3 - AST-kontrakt v1

Dette dokumentet beskriver den stabile AST-kontrakten som fase 3 byggjer vidare på.

## Formål

- Vere felles grunnlag for parser, semantic og backend
- Gjere AST lett å validere, skumlese og serialisere
- Halde felt og tyding stabile på tvers av vidare utvikling

## Minimumsfelt

Kvar AST-node bør kunne støtte desse felta:

- `type`
- `verdi`
- `barn`
- `linje`
- `kolonne`

## Normalreglar

- `type` må vere sett for gyldige noder
- `verdi` kan vere tom eller manglande når nodetypen ikkje treng verdi
- `barn` skal vere ei liste, aldri `null`
- `linje` og `kolonne` skal vere nummeriske posisjonar når dei finst

## Validering

- `ast_valider(node)` skal feile deterministisk på tom node eller manglande type
- `ast_v1_valider(node)` skal vere stram nok til å fange v1-formatet
- AST med `format = "norscode-ast-v1"` skal ha dei sentrale v1-felta

## Snapshot-reglar

- `ast_snapshot(node)` skal gi ei lesbar tekstform
- Snapshot skal returnere deterministisk tekst for same node
- Ugyldig AST skal gi eksplisitt invalid-markering

## Builders

Dei sentrale builderane i `selfhost/ast.no` er:

- `ast_binary_expr(left, operator, right)`
- `ast_unary_expr(operator, operand)`

Desse skal produsere enkle noder med klart barneoppsett.

## Vidare utvikling

- Fase 3 kan leggje til fleire node-byggarar utan å bryte minimumsfelta
- Nye node-typar skal dokumenterast før dei blir normative
- Semantic-laget skal kunne stole på desse minimumsfelta
