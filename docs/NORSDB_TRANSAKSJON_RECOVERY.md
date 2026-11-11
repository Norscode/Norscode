# NorsDB: transaksjonar og gjenoppretting

Eksplisitte transaksjonar gjennom std.db / std.norsdb_db held alle
WAL-endringar i minnet fram til commit. Ein ny prosess som opnar databasen
etter at skrivaren avslutta før commit, får den sist committa tilstanden.
Denne garantien gjeld den nye skrivaren. Gamle WAL-filer manglar informasjon
om transaksjonsgrenser; allereie lagra endringar frå den gamle motoren kan
ikkje automatisk skiljast i committa og ucommitta endringar.

## API og lagringsrekkjefølgje

1. **begin(handle)** opprettar ein tom angre-logg og ein tom WAL-buffer. Arbeidet
   ved begin er konstant; databasen blir ikkje kopiert.
2. Mutasjonar endrar den aktuelle handelen og legg før-bilete i angre-loggen.
   WAL-postane blir lagde i bufferen. Ingen av desse endringane blir skrivne
   til databasefilene enno.
3. **commit(handle)** skriv éin komplett transaksjonspost til WAL. Først etter
   at append og filstorleik er kontrollerte, blir buffer og angre-logg kasta.
   Adapteren køyrer deretter det vanlege inkrementelle eller fulle checkpointet.
4. **rollback(handle)** spelar angre-loggen baklengs og kastar WAL-bufferen.
   Rollback treng ikkje skrive eit checkpoint.
5. **close(handle)** rullar tilbake ein aktiv transaksjon før checkpoint og
   eventuell retur til tilkoplingspoolen.

Minnebruken veks med endringane i transaksjonen: angre-loggen og dei enno
uskrivne WAL-postane blir haldne i minnet fram til commit eller rollback.

begin på ein handel som alt har ein aktiv transaksjon, returnerer usann.
transaction opprettar heller ikkje ein nøsta transaksjon; den returnerer 0
og let den ytre transaksjonen stå urørt. commit utan aktiv transaksjon
returnerer usann. transaction rullar tilbake ved ugyldig SQL og før eit
unntak frå sjølve skriptet blir kasta vidare.
Den boolske fasaden std.transactional_storage.transaksjon returnerer sann
berre for resultatet 1; rollback eller avvist nøsting (0) blir usann.

Direkte db_checkpoint, db_checkpoint_full og db_backup blir avviste
medan transaksjonen er aktiv. Dette hindrar at eit snapshot publiserer
ucommitta minnetilstand. Ein backup kan takast etter commit eller rollback.
Vanlege einskildmutasjonar utanfor ein eksplisitt transaksjon held fram med å
bruke det eksisterande WAL-postformatet.

Dette endrar ikkje SQL-parseren sine kontrollsetningar: bruk transaksjons-API-et,
ikkje execute(handle, "BEGIN"), SAVEPOINT eller RELEASE, for desse
garantiane. SQL-kontrollsetningane har framleis den tidlegare kompatibilitets-
oppførselen. Ein SQL-operasjon med fleire mutasjonar er berre dekt av den nye
gruppegarantien når han køyrer inne i ein eksplisitt transaksjon.

## Transaksjonsposten

NORSDB2 får følgjande nye posttype:

| Felt | Innhald |
|---|---|
| Operasjon | B |
| Versjon | 1 |
| Tal | Talet på indre mutasjonspostar |
| CRC | CRC32 over heile den uescapa kroppen |
| Kropp | Dei vanlege mutasjonspostane, kvar avslutta med RS |

Som før er US (chr(31)) feltskilje, RS (chr(30)) postslutt og GS
(chr(29)) escape-teikn. Heile kroppen blir escapa som eitt felt i den ytre
posten. Ingen indre RS kan dermed avslutte den ytre posten. Berre ein komplett
append inneheld den siste, uescapa RS-en som gjer heile transaksjonen lesbar.
Dette føreset ikkje at éin filskriving er atomisk.

Ved replay blir versjon, tal, sluttmarkør, CRC og indre posttypar kontrollerte
før nokon av mutasjonane blir brukte. Ein komplett, ugyldig B-post gjer opning
til ein feil. Ein ufullstendig siste post blir forkasta. Eit avbrot midt i
WAL-hovudet blir òg handsama som ein ufullstendig hale.

Eit inkrementelt checkpoint tek med B-posten uendra; den blir validert på nytt
ved opning. Eit fullt checkpoint serialiserer den gjenoppretta tilstanden som
vanlege tabell-, rad- og metadatapostar. Epoken vernar framleis mot å spele
same WAL på nytt etter at checkpoint er fullført, men før WAL er sletta.

Etter ein ufullstendig WAL-hale blir den gyldige tilstanden skriven gjennom
den eksisterande .cp2-protokollen før WAL blir sletta. Motoren trunkerer
ikkje lenger heile WAL-fila på staden for å fjerne halen; eit nytt prosessavbrot
under slik trunkering kunne elles miste tidlegare committa postar.

## Filfeil og varigheit

WAL-append kontrollerer filstorleiken både før og etter skriving. Ein uventa
storleik, eit skriveunntak eller ein ufullstendig append gjer handelen
ubrukeleg for vidare lagring. Checkpoint-feil ved skriving har same verknad.
Vidare commit, rollback som ville hevde eit sikkert resultat, checkpoint og
backup blir avviste. close returnerer usann, skriv ingenting og returnerer
ikkje ein slik handel til poolen. Forkast handelen og opne databasen på nytt.

Etter ein skrivefeil kan utfallet vere usikkert: ein komplett commit-post kan
ha nådd fila sjølv om eit seinare steg feila. Applikasjonen må lese tilbake
tilstanden etter ny opning før han avgjer om ein operasjon skal prøvast på nytt.
Dette er særleg relevant dersom WAL-commit lukkast, men checkpoint feilar.

Den native filsystemimplementasjonen brukar fil_skriv / fil_append;
std.runtime_filesystem_native sin flush returnerer utan eit fsync-kall.
Motoren har heller ingen filsystembarriere for katalogoppføringar. Difor er
garantien her **atomisk gjenoppretting etter prosessavbrot og trunkerte
append-prefiks**, ikkje dokumentert varigheit ved straumbrot, kernelkrasj
eller vilkårleg omordning av diskblokk-skrivingar. Storleikskontroll er ikkje
ein erstatning for fsync, og CRC er ikkje kryptografisk autentisering.

Motoren føreset éin skrivande handel per databasefilsett. Storleikssjekken
oppdagar nokre konfliktar med eksterne endringar, men er ikkje ei fillås og
gjev ikkje støtte for samtidige skrivarar eller full transaksjonsisolasjon.

## Formatkompatibilitet og nedgradering

Den nye lesaren støttar eksisterande NORSDB1-migrering og NORSDB2-checkpoint
og WAL med vanlege einskildpostar. Dette er **bakoverlesing av gamle filer**.

Ein eldre NorsDB-lesar kjenner ikkje B-postar og kan ignorere dei i både WAL
og delta-checkpoint. **Ikkje opne eit slikt filsett med den eldre motoren.**
Det eksisterande NORSDB2-magictalet tvingar ikkje ein eldre lesar til å
avvise filene. Eit vanleg commit køyrer checkpoint, men eit inkrementelt
checkpoint kan framleis innehalde B-postar og gjer ikkje nedgradering trygg.

For å eksportere til det tidlegare formatet: avslutt transaksjonen, køyr
db_checkpoint_full med den nye motoren og steng skrivaren. Alternativt
lag ein full backup med den nye motoren og gjenopprett han som mål.cp.
Begge vegane serialiserer vanlege postar og fjernar B-rammene frå det
eksporterte snapshotet. Hald fram med den nye motoren dersom dette ikkje
er gjort. Direkte nedgradering av nye WAL-/deltafiler er ikkje støtta.

## Regresjonstestar

- **tests/test_norsdb_tx_prosess.no** startar 11 separate Norscode-prosessar:
  ein skriv grunnlaget, så fem skrivar/lesar-par. Para dekkjer avslutting med
  kode 23 før commit og før første checkpoint, rollback, close med aktiv transaksjon, commit direkte
  til WAL før checkpoint og vanleg commit. Lesarane kontrollerer rader,
  UPDATE/DELETE/INSERT, skjema, indeksar og tekst med Unicode og kontrollbyte.
- **tests/test_norsdb_tx_recovery.no** skjer gjennom kvar bytegrense i ein WAL
  med to transaksjonar. Kvar opning må gi anten tilstanden før begge, etter
  den første eller etter begge, og aldri ein delvis transaksjon. Testen
  kontrollerer òg vidare skriving etter reparasjon, ugyldig CRC/versjon,
  begge checkpoint-vegane, absorbert WAL, aktive checkpoint-/backup-sperrer,
  ein faktisk filfeil ved commit, nøsta begin og pool-gjenbruk.
- Eksisterande test_norsdb_tx_angre, test_norsdb_tx_commit,
  test_norsdb_checkpoint, test_norsdb_motor_wal,
  test_norsdb_motor_recovery og test_norsdb_skjema_persist vernar rollback,
  eldre format, skjema og checkpoint-gjenoppretting.
- **tests/test_transactional_storage_transaksjon.no** prøver den offentlege
  boolske fasaden med commit, feil som rullar tilbake og avvist nøsting.

Testane simulerer alle append-prefiksa og ekte prosessgrenser. Dei simulerer
ikkje straumbrot eller fysisk diskskriving, og påstår ikkje slike garantiar.
