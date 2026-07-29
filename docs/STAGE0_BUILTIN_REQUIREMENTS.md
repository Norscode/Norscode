# STAGE0 Builtin-krav v811

Denne spesifikasjonen definerer kva innebygde funksjonar (builtins) stage0 må støtte for å halde v731-webplanen og dei påfølgjande fase 0-oppgåvene trygt i drift.

## Omfang

- Dokumentet dekkjer runtime-kompatibilitet mellom Norscode-script og stage0-native.
- Fokus er funksjonar som er i aktiv bruk og som må vere deterministiske/sikre for vidare web- og sessionsfunksjonalitet.
- Dette er eit kravdokument, ikkje ei implementasjon.

## Påkrevde builtins (v801–v820)

| Builtin | Kravstatus | Operasjon | Minimum krav til stage0 |
|---|---|---|---|
| `builtin.random_hex` | Krav | Tilfeldig hex-streng | Må gi trygg, kryptografisk sterk streng med førehandsdefinert lengde (`n` i teikn) i produksjonssamanheng. Inntil no må kall vere avgrensa til DEV-modus. |
| `builtin.exec_prosess` | Krav | Kjøre eksterne prosessar | Må vere eksplisitt styrt/limt (policy), og støttast berre i definerte tryggleiksflater (t.d. avgrensa kommandoar, whitelist, timeout). Stage0-implementasjon må dokumentere korleis kall er isolerte per runtime. |
| `builtin.tid` | Krav | Tidspunkt/epoch | Må returnere eit stabilt numerisk timestamp (Unix-epoch eller tilsvarande definisjon) som standardisert i std/kompilatoren kan forvente. |
| `builtin.now` | Krav | Dagens tidspunkt | Må gi ein deterministisk og maskin-uavhengig verdi i samsvar med språk-kontrakten. |
| `builtin.timestamp` | Krav | Formaterings-/tidsfelt | Må returnere tidsverdi i eit format som std-logikk for status/diagnostikk kan bruke utan fallback-triks. |
| `builtin.socket_listen` | Krav | Nettverksbinding | Må kunne opne ein lyttande socket på angitt port og returnere handle/ID eller feilkode i samsvar med runtime-kontrakt. |

## Verifikasjon (kva denne listen skal muliggjere)

- `web`-løypene (`/`, `/admin`, `/status.json`, `/logout`) må kunne handtere status- og session-fløyer uten at login/produksjons-sperre går i stykker.
- Builtin-avhengig kode i `std/` må kunne køyre i ein konsekvent mode utan at fallback-lyster (hardkoding av verdiar) blir naudsynt.
- Stage0 kan planleggjast for tryggleik ved at desse funksjonane blir med i ein eksplisitt policy:
  - dev-only for sensitive kall (`random_hex`, `exec_prosess`-grupper)
  - eksplisitt tillatelsesmodell for ekstern interaksjon

## Gå vidare

- `v821–v830`: design for trygg `random_hex`.
- `v831–v840`: trygg policy for `exec_prosess`.
