# NorsDB: lagringsbarrierar og gjenoppretting

NorsDB kvitterer ikkje ei WAL-endring før skriveoperasjonen og OS-barrieren
har fullført. Dette gjeld både éin endring utanfor transaksjon og den samla
`B`-posten ved `db_tx_slutt`. Ein database med usikker I/O-status blir sperra
for vidare skriving og må opnast på nytt.

## Plattformkontrakt

| Plattform | Filinnhald | Publisering av filnamn |
| --- | --- | --- |
| Linux x86-64 og ARM64 | `write` og `fsync` på same opne fd | `fsync` på den opne foreldrekatalogen etter oppretting, rename og unlink |
| macOS ARM64 | `write`, `fsync`, deretter `fcntl(F_FULLFSYNC)` | Katalog-`fsync` kjem før den siste diskbuffer-barrieren på fil-fd-en |
| Windows x86-64 | Overlapped `WriteFile` fullfører før `FlushFileBuffers` på same HANDLE; ved gjenoppretting blir eksisterande kjeldefil flusha før publisering | Atomisk erstatning brukar `MoveFileExW` med `REPLACE_EXISTING | WRITE_THROUGH`; Windows lovar ikkje ein generell POSIX-lik katalog-`fsync` gjennom denne API-en |

Det siste punktet er ei faktisk avgrensing: Windows-filbarrieren og
gjenopprettingsformatet er ikkje dokumentasjon på full namneromsvarigheit
under alle straumbrot. Ingen av testane i repoet er ei fysisk straumbrotprøve
av kontroller, disk eller nettverksfilsystem. Maskinvare og filsystem må halde
sin OS-kontrakt. Ein backend som ikkje støttar den påkravde barrieren, skal
returnere feil; han skal ikkje stille nedgradere til «fila vart lukka».

Primærkjelder:

- [Linux fsync](https://man7.org/linux/man-pages/man2/fsync.2.html): filmetadata
  og foreldrekatalogen er separate synkroniseringspunkt; forseinka skrivefeil
  kan bli rapporterte gjennom den opne fildeskriptoren.
- [Apple fcntl](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/fcntl.2.html):
  `F_FULLFSYNC` ber òg lagringseininga tømme diskbufferen.
- [Microsoft FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)
  og [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw).

## Rekkefølgje ved lagring

WAL-skriving opnar fila gjennom den rotbundne filesystem-ABI-en, skriv alle
byte og synkroniserer før statusen i databasehandtaket blir oppdatert. Det
same OS-handtaket blir halde gjennom skriving og flush, slik at ein ikkje
opnar ein ny fd etter at ein skrivefeil alt er rapportert til den gamle.
Storleikskontrollen etterpå avviser forkorta eller uventa samtidige endringar.

Inkrementell checkpoint legg eit heilt CRC-bunde segment til `.cp` og
synkroniserer det før WAL blir sletta. Full checkpoint skriv `.cp2`,
synkroniserer fila, erstattar `.cp` atomisk og synkroniserer publiseringa
før WAL blir sletta. Den gamle `.cp` blir ikkje trunkert under denne vegen.

Ved omstart kan ei komplett og minst like ny `.cp2` fullføre publiseringa.
Ei avbroten eller eldre `.cp2` erstattar aldri ei gyldig nyare `.cp`.
Checkpointet hugsar absorbert WAL-epoke: dersom ei gammal WAL framleis finst
etter avbrot rundt sletting, blir ikkje endringane spela av to gonger.

Backup brukar same mønster med ei mellombels `<mål>.cp2` og atomisk rename.
Det mellombelse namnet høyrer til NorsDB og må ikkje brukast til ei anna fil.
Checkpoint og backup er forbodne medan ein transaksjon er aktiv.

## Kva testane skal bevise

- `test_native_filesystem_durable.no` køyrer verkeleg write/flush/rename/unlink,
  binære byte over fleire skrivebitar og negative bane-/handtakstilfelle. På
  POSIX blir eit ope handtak halde gjennom unlink og gjenbruk av same filnamn;
  vidare skriving må treffe den opphavlege inoden.
- `test_norsdb_durable_checkpoint.no` prøver kvar bytegrense i ein full
  checkpoint før publisering. Den kvitterte transaksjonen må overleve gjennom
  gammal checkpoint + WAL eller den komplette nye checkpointen. Ein reell
  OS-feil på `.cp2` må bevare gammal `.cp`, WAL og skrivesperra.
- `test_norsdb_tx_recovery.no` prøver kvar bytegrense gjennom to transaksjonar,
  korrupsjon, inkrementell checkpoint, gjenbruk av absorbert WAL og feil ved
  commit. `test_norsdb_tx_prosess.no` køyrer separate skrive-/leseprosessar.

Desse testane må køyrast med ein runtime bygd frå kjeldene som implementerer
`write_durable`, `file_replace_durable` og `delete_durable`. Ein frosen
bootstrap-runtime er framleis byggjestart, men kan ikkje prove nye OS-operasjonar.
