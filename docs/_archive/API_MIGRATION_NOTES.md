# API Migration Notes

Kort migrasjonsmodell for Norscode-baserte API-er:

## Fra v1 til v2

- Legg til nye felt i responses, ikke fjern de gamle først.
- Lag nye ruter for breaking changes, for eksempel `/api/v2/...`.
- La v1 leve parallelt en overgangsperiode.
- Dokumenter forskjellen i et eget eksempel og i changelog/release notes.
- Fjern v1 først når klientene er oppdatert og det finnes en tydelig deprecation-policy.

## Hva som skal med i en migrasjonsnote

- hvilke ruter som er nye
- hvilke ruter som er deprecated
- hvilke felt som er lagt til, endret eller fjernet
- hvordan klienten bør oppdatere seg

