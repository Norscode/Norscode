# GitHub Språkvisning

GitHub bruker `github-linguist` for å rekne ut språkbaren på repo-sida.

Norscode bruker `.no` og `.nors`, men GitHub viser ikkje automatisk nye språk før dei er registrerte i Linguist. Derfor kan Norscode-kode bli vist som ukjent, tekst, eller bli overskygga av Shell, JSON, Markdown eller andre kjende språk.

## Kva repoet gjer no

`.gitattributes` markerer genererte artefakt og historiske mapper som generated/vendored:

- `*.ncb.json`
- `*.tokens.json`
- `*.chain.ncb.json`
- `archive/**`
- `build/**`
- `dist/**`
- `release-artifacts/**`

Dette gjer språkstatistikken reinare, men det registrerer ikkje `Norscode` som nytt GitHub-språk.

## Kva som trengst for ekte Norscode i GitHub

For at GitHub skal vise `Norscode` i språkbaren må språket leggast til i `github-linguist` med:

- språkoppføring for `Norscode`
- filendelser som `.no` og `.nors`
- TextMate grammar eller annan klassifiseringsstøtte
- eksempel-filer for språkdeteksjon
- farge og metadata

Når Linguist støttar Norscode, kan repoet få ekte språkvisning utan å late som `.no` er eit anna språk.
