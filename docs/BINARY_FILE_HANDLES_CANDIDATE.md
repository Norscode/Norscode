# Kandidat: binærlesing frå native filhandtak

Status: implementert og testa i kjeldebasert backend og som faktisk macOS ARM64-program. Ingen seed eller kodegenerator er promotert. Heile plattformmatrisa står att.

## Endring

read_binary bruker det eksisterande innhaldsbiletet frå open og same posisjon som tekstlesing. Han opnar ikkje fila på nytt. Returen inneheld data_bytes som faktisk bytes-verdi, data er tom tekst, og value er talet på lesne byte. max=0 les ingenting, negative lengder blir avviste, og EOF flyttar ikkje posisjonen. Utrekning av tilgjengeleg lengd før addisjon hindrar overflow ved svært store maksverdiar. Den same EOF-korreksjonen gjeld tekstlesing.

Ingen async-operasjon er omdefinert som implementert. Sti-/handtaksvalidering og capability-grenser er bevarte.

## Faktiske testresultat

- Den nye backendtesten feila før endringa med status feil i staden for ok. Etter rettinga består han i VM og som faktisk native macOS ARM64-program. Han dekkjer nullbyte, 255 og 128, uendra open-bilete etter filoverskriving, blanda tekst-/binærposisjon, søking, null/negativ/maksimal 64-bit leselengd, EOF forbi filslutt og avvising av lukka/skrivehandtak.
- Fire relaterte testar består, null feil og null hopp: test_filesystem_binary_handle_backend, test_runtime_filesystem_handles, test_native_filesystem_handles og test_vm_filesystem_request_scope.
- Den eksisterande test_native_filesystem_binary_stream.no er køyrd med NORSCODE_REQUIRE_BINARY_FILESYSTEM_LIVE=1. I ein komplett native bunt med std.native_gap består heile den uendra testen, også lukking og sletting. Kompileringsrapporten er uløyste=0, stubba=0.
- Eit vanleg enkeltfilbygg av same live-test stoppar framleis på NATIV-GAP:native_fs_unlink ved sletting. Han manglar native_gap-avhengigheita i den bunten; dette er ikkje kalla bestått.
- Den pinna gamle VM-en feilar framleis den direkte live-testen. Endra kjelde er ikkje det same som ei godkjend utskifting av seed/runtime.

Native testar bruker den kontrollerte ARM64-kodegeneratorkandidaten frå PR #254, SHA-256 83885951c59d72473dda8bbdb256e8cac35b5a86b3722a95666af7bcef0f895f. Den gamle assert_eq-implementasjonen kunne gi falsk PASS for ulike typar og avvise like bytes; ingen assertion i filtesten er svekt for å gå rundt dette.

## Reproduksjon av komplett native live-test

Lag ei kandidatfil under build med:

~~~norscode
bruk tests.test_native_filesystem_binary_stream som prove
bruk std.native_gap som native_gap
funksjon start() -> heiltall { returner prove.start() }
~~~

Kompiler denne fila til NCB JSON med ./bin/nc compile, NORSCODE_USE_PRECOMPILED_SELFHOST=0 og NORSCODE_DISABLE_MODULE_CACHE=1. Køyr den SHA-verifiserte kodegeneratorkandidaten med NC_INPUT sett til NCB-fila, NC_OUTPUT til ei ny fil under build og NC_BUILTIN_REPORT=1. Krev exit 0 og faktisk uløyste=0, stubba=0. Køyr den produserte Mach-O-fila med NORSCODE_REQUIRE_BINARY_FILESYSTEM_LIVE=1 og krev exit 0 samt test_native_filesystem_binary_stream OK.

## Restarbeid

Oppdaterte runtime-/kodegeneratorkandidatar må byggjast, funksjonstestast og bindast til kjeldehashar på alle relevante vert/mål før eventuell godkjend promotering. Endra filsystemkjelde ligg i kodegeneratorane si importlukking; gamle .depshash-filer skal avvisast, ikkje omskrivast for å godta gamle binærar. Ufullstendig enkeltfilbundling og gammal pinna VM er separate opne manglar. Dette dokumentet er ikkje eit bevis på 100 prosent sjølvstende.
