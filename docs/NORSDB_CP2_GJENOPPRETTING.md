# NorsDB: kontrollert gjenoppretting frå .cp2

Når kompaktering blir avbroten, kan både `.cp` og `.cp2` finnast.
Tidlegare vart alle `.cp2` med ein vilkårleg Z-markør kopierte til `.cp`,
sjølv om `.cp` inneheldt nyare fullførte transaksjonar. Det kunne føre til
tilbakerulling og datatap ved neste opning.

## Val av checkpoint

- Finn siste fullførte S/Z-segment og epoken for begge filer.
- Ei `.cp2` må ha avsluttande komplett segment og samsvarande S/Z-epoke.
- Gjenopprett frå `.cp2` berre om epoken ikkje er eldre enn den siste
  fullførte epoken i `.cp`.
- Ei eldre eller ufullstendig `.cp2` blir kasta, og `.cp` blir brukt.
- Den eksisterande WAL-replayen og handtering av trunkerte `.cp`-halar er
  uendra.

## Regresjon

`./bin/nc run tests/test_norsdb_checkpoint.no` prøver både eldre `.cp2`
med ein nyare `.cp` og ei ufullstendig `.cp2`, og kontrollerer at rader
ikkje forsvinn. CI og manuelt testresultat må stadfestast før merge.

## Avgrensingar

Dette er ei avgrensa recovery-retting, ikkje fullstendig durabilitetsgaranti.
Diskfeil, delvis skrift, korrupt data og fsync/atomisk rename krev vidare arbeid.

## Neste hardening: validering og skrivekontroll (stacka PR)

Oppstart krev no ein korrekt NORSDB2-header og fullførte S/Z-segmentpar.
Ei heil `.cp2` må innehalde nøyaktig eitt komplett segment, utan
overskytande postar eller avbrote hale. Ei ordinær `.cp` kan framleis
ha ein ufullstendig SISTE delta; dette blir forkasta før replay, medan
siste heile segment blir brukt. Ei fil utan heilt segment gjev eksplisitt
feil i staden for at databasen ser tom ut.

Før full checkpoint er rekna som vellykka, blir både `.cp2` og `.cp`
kontrollerte mot forventa filstorleik. Ved inkrementell checkpoint må
storleiken auke med heile det nye segmentet. Ved feila/ufullstendig
skriving blir ikkje WAL fjerna eller epoken endra. Det er ein
**storleikskontroll**, ikkje checksumming eller garanti for `fsync`;
feil som ikkje endrar lengda kan framleis sleppe gjennom.

Regresjonstestane i `tests/test_norsdb_checkpoint.no` dekkjer feil
segmentstruktur, ufullstendig/ekstra `.cp2`, primærfil utan gyldig
segment og feil ved checkpoint-skriving.

## CRC32 per nytt checkpoint-segment

Nye `S ... Z`-segment brukar `Z<US>epoke<US>crc32<RS>` der
CRC32 er eit desimalt u32-tal for alle byte frå `S` til og med
siste `RS` før `Z`. Kontrollsummen er over eksakte lagringsbyte,
ikke over den dekoda SQL-verdien. Full checkpoint, backup og
inkrementell checkpoint skriv dette feltet. Ved opning blir CRC32
verifisert før eit checkpoint blir spelt av. Endring av éin byte med
same filstorleik blir dermed oppdaga. NORSDB2-segment utan CRC-felt
kan framleis lesast for bakoverkompatibilitet, men dei gir ikkje den
same korrupsjonsdeteksjonen.

CRC32 er ein ikkje-kryptografisk feilkontroll; han gir ikkje vern
mot medviten manipulering eller garanti for fsync/atomisk lagring.
Test i `tests/test_norsdb_checkpoint.no` byter ut ein byte i den
lagra radverdien utan å endre storleik og krev at opning feilar.
