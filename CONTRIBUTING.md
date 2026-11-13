# Bidrag til Norscode

Les [AGENTS.md](AGENTS.md) for utviklingsreglane og
[LICENSING.md](LICENSING.md) for lisensen til filene du endrar.

- Bidrag til kompilator- og verktøyområda følgjer MPL-2.0.
- Bidrag til Apache-områda, mellom anna runtime, standardbibliotek, native
  kodegeneratorar og prosjektmalar, følgjer Apache-2.0.
- Hald på lisens- og opphavsrettsmerknader for tredjepartskode. Oppgi kjelda og
  lisensen når slik kode blir teken inn.
- Bidragsytaren må ha rett til å levere bidraget under lisensen til komponenten.
- Ny støtte som skal kopierast inn i brukarprogram, må plasserast i rett
  Apache-område. Ikkje flytt kode mellom lisensområda utan å kontrollere vilkåra.

Tidlegare innvilga Apache-rettar blir ståande. Nye endringar i MPL-områda får
ikkje automatisk ein alternativ Apache-lisens.

Vanleg arbeidsveg er ei avgrensa grein og ein pull request med forklaring av
endringa og relevante kontrollar. Nye funksjonar blir kontrollerte med
`./bin/nc feature-check` slik AGENTS.md krev.
