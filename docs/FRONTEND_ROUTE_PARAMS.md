# Frontend Route and Query Parameters

Route-parametre og query-parametre er en del av frontenden sin adressestruktur.

## Mål

- gjøre URL-er lesbare
- holde state delvis delbar via adressefeltet
- støtte deep links og refresh uten ekstra arbeid

## Kontrakt

- route-parametre brukes for identitet og hierarki
- query-parametre brukes for filtrering, sortering og visningsvalg
- URL-en skal være nok til å gjenskape den viktige visningen

## Eksempler

- `/brukere/42` for en bestemt bruker
- `/produkter?kategori=sykler&sort=pris`
- `/sok?q=norscode&page=2`

## Regler

- bruk route-parametre når objektet er en del av pathen
- bruk query-parametre når verdien kan endres uten å endre sideidentitet
- la query-parametre være stabile og enkle å lese
- unngå å bruke query for skjult intern state

## Praktisk konsekvens

- klient og server bør tolke parametrene likt
- navigasjon skal oppdatere URL-en når visningen endres
- direkte lenker skal fungere uten ekstra init-logikk

## Når dette er ferdig

- appen kan bruke URL-en som del av state-modellen
- brukere kan dele og gjenåpne spesifikke visninger

Se også:

- [docs/FRONTEND_NAVIGATION_MODEL.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_NAVIGATION_MODEL.md)
- [docs/FRONTEND_CLIENT_ROUTING.md](/Users/jansteinar/Projects/language_handoff/projects/language/docs/FRONTEND_CLIENT_ROUTING.md)
