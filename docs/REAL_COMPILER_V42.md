# REAL COMPILER V42

Dette steget er et **trace/probe-steg**, ikke full feilfiks.

## Mål
Finne hvorfor `storre eller lik` fortsatt brytes opp feil i full selfhost-kjede,
selv om hotpathen kan håndtere uttrykket isolert.

## Konkret fokus
- logg rå tokenliste før uttrykkskompilering
- logg normalisert tokenliste
- logg kallested / funksjon
- sammenlign feilet case mot isolert grønn case

## Kommandoer å kjøre etter at probe-koden er koblet inn
```bash
python3 main.py selfhost-chain-run tests/test_selfhost.no --trace
python3 main.py selfhost-chain-run tests/test_selfhost.no --trace-focus uttrykk_til_ops_og_verdier_med_miljo
```

## Ærlig status
Dette er en diagnostikk-milepæl. Den fikser ikke `tests/test_selfhost.no` alene.
Neste reelle mål er å bruke probe-dataene til å rette tokenflyten.
