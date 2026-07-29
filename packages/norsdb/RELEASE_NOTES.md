# NorsDB - Release Notes

## MVP-ferdig

Denne versjonen av NorsDB er ferdig som en Norscode-basert MVP.

### Hva som er med

- samlet databasekjerne i `norsdb_*.nors`
- vanlig API-demo i `example.nors`
- snapshot-demo i `snapshot_demo.nors`
- enkel smoke-test i `norsdb_smoke.nors`, med info, DB-info, connection-status, status og samandrag
- pakke-selftest i `norsdb_selftest.no`, via `norsdb.selftest()`
- `norsdb.nors` som samlet inngangspunkt med `samandrag()` og `status()`
- lagring og gjenoppretting av schema, rader og indeksmetadata
- ryddigere transaksjoner, parser og evaluator

### Hva som fortsatt er kjent

- ekte runtime-kjøring er ikke verifisert i dette miljøet
- full produksjonsklar database med komplett runtime-verifisering er fortsatt neste nivå
