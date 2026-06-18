# Selfhost Fase 2 - FFI Smoketest v1

Dette dokumentet definerer det første enkle smoketestløpet for ABI og modulkall.

## Mål

- Bekrefte at offentlege API kan kallast i rekkje utan skjulte brot
- Bekrefte at builtin-kall og extern-modul-kall fungerer saman
- Gi ei lita, deterministisk regresjonssikring for integrasjonsflata

## Smoketest-flyt

1. Opprett ein loggobjekt og verifiser felt
2. Opprett ein konfigurasjon og les status
3. Skriv og les ein midlertidig fil
4. Lagre og les eit lite datamateriale
5. Verifiser cache-, scheduler- og trådstatus

## Forventa signal

- Loggfunksjonar returnerer strukturert data
- Filfunksjonar handterer sti og status deterministisk
- Lagring kan persistere og hente JSON-data
- Runtime-statusfunksjonar returnerer lesbar oversikt

## Akseptkriterium

- Løpet er kort nok til å brukast i CI
- Løpet er lesbart utan ekstra kontekst
- Løpet feilar tidleg dersom ABI-flata bryt
