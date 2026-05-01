# Deprecation Policy

Norscode holder en lav-friksjons policy for navn og CLI-overganger.

## Grunnregler

- Nye brukere skal møte `norcode` og `Norscode`.
- Legacy-navn kan bestå kun for kompatibilitet.
- Kompatibilitetsnavn skal ikke få ny primærfunksjonalitet.

## Legacy-aliaser

- `nor`, `nc`, `nl` og `norsklang` kan videreføres som aliaser så lenge de peker til samme kontrakt som `norcode`.
- Ved brudd skal ny hovedbruk dokumenteres først, og gammel bruk fases ut med tydelig migreringssti.

## Filer og config

- Nye filer bruker `norcode.toml`, `norcode.lock` og `.norcode/`.
- Legacy-filer kan leses og migreres, men bør ikke introduseres på nytt i nye prosjekter.

## Varsler

- Legacy-ting kan varsles kort i CLI, men skal ikke blokkere vanlig bruk.
- Nye brudd skal følges av dokumentert migrering og oppdatert release-notat.
