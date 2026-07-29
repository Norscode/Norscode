# V5800 Bootstrap codegen diff

## Formål
Dette steget lagar ein fast diff-rapport mellom aktiv bootstrap/generated-C compile-output og dei forventa bytekode-signala frå referansefasiten.

## Kva det gjer
`tools/bootstrap_codegen_diff_v5800.no` les output frå `build/v5600/out/*.ncb.json` og markerer kvar probe som:
- `green` dersom forventa signal finst
- `red` dersom eitt eller fleire signal manglar
- `missing-output` dersom compile-output ikkje er generert enno

## Kvifor dette er nyttig
Når vi byrjar å rette bootstrap-compileren, slepp vi å lese heile JSON for hand. Vi får i staden ein kort rapport som viser:
- kva probe som framleis er broten
- kva opcode som manglar
- eit lite utsnitt av generert kode

## Avgrensing
Dette steget fiksar ikkje codegen i seg sjølv. Det gjer berre selve bootstrap-fiksen mykje raskare å styre og verifisere i neste omgang.
