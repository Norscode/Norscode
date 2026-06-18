# Selfhost Fase 2 - Definition of Done

Dette dokumentet definerer kva som tel som ferdig for fase 2-leveransar.

## Felles krav

Ein fase 2-leveranse er ferdig når:

- implementasjonen finst i repoet
- statusen er dokumentert i fase 2-planen
- minst ein minimal test eller smoke-kontroll dekkjer området
- `./bin/nc ci` viser det relevante rapportpunktet
- resultatet er synleg i den samla fase 2-statusen

## Delkrav per område

### ABI og integrasjon

- offentleg ABI er dokumentert
- kall-kontraktar er skrive ned
- FFI-smoke kan køyrast utan manuell inngripen

### Runtime og verktøy

- runtime-målet er konkretisert
- minimum eitt runtime-arbeidspunkt er låst
- baseline benchmark er køyrbar i CI

### Standardbibliotek

- modulen har status `ferdig`, `pågår` eller `mangler`
- statusflata er dokumentert
- minst éin regresjonskontroll peikar på modulområdet

### Kvalitet og CI

- gatepunktet er maskinlesbart
- gatepunktet er kopla til `./bin/nc ci`
- ei enkel, stabil benchmark er del av gateflata
- regresjonstestar for fase 2 er refererte frå planen

## Når vi kan kalle fase 2 stabil nok

- ABI og runtime er beskrive på ein måte som hindrar tilfeldig breaking
- standardbiblioteket har faste statusflater
- CI kan seie klart om fase 2 er grøn, gul eller raud
- regresjonar har eigne, enkle inngangar
