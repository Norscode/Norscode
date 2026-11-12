# Storleiksdelte fri-lister i x86-GC

Den gamle x86-allokatoren vandra dei første 512 hola i éi fri-liste. Etter
sweep kunne hundrevis av små boksar liggje framfor store ledige blokker. Ei
stor allokering nådde vandringstaket og gjekk til bump sjølv om holet fanst.
Dette er same feilklasse som ARM64 allereie handterer med storleiksklassar.

Med `NORSCODE_GC_ALLOC=1` brukar x86 no den eksisterande smålista på
`HEAP_VA+40` for hol under 1 KiB og 14 separate lister for større hol.
Klasse 0 held 1–2 KiB, klasse 1 held 2–4 KiB, og så vidare; siste klasse
held alt frå 8 MiB. Tabellens 112 byte ligg på `0x41000000`, mellom
handlerområdet og live-map, utanfor allokerbar heap og kanon-cachane.

Ein førespurnad startar i sin eigen klasse og prøver deretter større
klassar. Hurtigpasset har framleis eit tak på 512 steg per liste. Dersom
ingen blokk er funnen, kjem eit komplett pass gjennom eigna klassar før
allokatoren får auke bump. Det finn òg eit passande hol etter node 512 i
same klasse. Passet er avgrensa av talet på 16-byte-slott i den uendra
heapen; gyldige, disjunkte fri-nodar kan aldri overstige dette talet.
Når ei blokk blir
splitta, blir resten straks lagd i klassen som svarar til den nye
storleiken. Alle innsette hol blir avgrensa til komplette 16-byte-slottar;
leiande og siste ufullstendige slott blir ikkje skrivne til. Dette gjev
allokatoren ei enkel invariant: alle listeblokker er aligna og klassen
svarar til faktisk brukbar storleik.

`gc_alloc`, `gc_alloc16`, `gc_alloc_var` og `gc_box_int` deler denne
allokatoren. `gc_free` og begge sweep-inngangane brukar same innsetjing.
Collect nullstiller alle listehovuda før han byggjer dei opp att, slik at
ingen gammal node blir frigjord to gonger. Rå allokering bevarer register
og flagg, bortsett frå returregisteret `rax`.

Heapgrensa, soft/hard-grensene og GC-budsjettet er ikkje auka. Bump-vegane
avviser storleiks-overflyt og stoppar før den eksisterande vaktsida. Ein
nullstorleiksførespurnad får eitt eige 16-byte-slott. GC-av-emisjonen er
uendra; byteidentisk output er kontrollert mot førre codegen på same NCB.

## Native regresjon

`tests/test_native_gc_sizeclasses.no` byggjer og køyrer fire ekte native
prøvar, og er med i `tools/ci_native_gc.no`:

1. `gc_sizeclass_probe.no` lagar ein sortert live-map med 600 små hol
   framfor eit ledig 64 KiB-hol. To vanlege strengkonkateneringar på
   8 KiB må bruke det store holet og splitten utan bump-vekst. Levande
   skiljeblokker, ualigna holgrenser og nullstorleiksallokering blir òg
   kontrollerte. Førre codegen gjev exit 7; kandidaten gjev exit 0.
2. `gc_ncb_serialize_probe.no` held ein graf med 500 bytecode-funksjonar
   levande og køyrer den reelle `serde.serialiser_tekst` 60 gonger i same
   prosess. Dette er implementasjonen bak VM-builtin `ncb_til_binaer`.
   Alle 1 092 204 byte må vere like kvar gong, og den siste dekodinga må
   framleis gje rett funksjonsgraf. Kandidaten målte maks bump 101 658 720.
3. `gc_sizeclass_oom_probe.no` krev at ein umogleg heapstor førespurnad
   framleis avsluttar med exit 199 før skriving.
4. `gc_sameclass_probe.no` legg eit 15 KiB-hol bak 600 hol på 8 KiB i
   same klasse og ber om ein strengpayload på 12 016 byte. Både den
   opphavlege allokatoren og klasseversjonen med berre 512-stegspasset
   gjev exit 7; det komplette reservepasset finn holet utan bump-vekst.

Kandidaten vart bygd frå gjeldande `.no`-kjelde: `nc bundle` laga NCB,
den eksisterande native codegen bygde ein ny codegen under `build/`, og
den nye codegen bygde prøvane med GC på. Alle codegen-stega rapporterte
null uløyste builtins. `gc_hashmap_probe.no` var òg grøn. Kandidatvegen i
testen er eksplisitt med `NORSCODE_GC_TEST_CODEGEN`; normal CI går gjennom
`bygg-native` og den vanlege ferskheitsporten.

Serializer-prøven er ein integritets- og belastningsregresjon, ikkje ein
negativ kontroll: han passerer òg med førre codegen. Den deterministiske
600-hol-prøven er den negative kontrollen for rettinga. Det opphavlege
tolka `forbered_ncb`-forløpet må i tillegg køyrast gjennom den samla,
nybygde nc_main-seeden før endeleg promotering.

Dette er ein ikkje-flyttande allokator. Reservepasset har lineær kostnad
i talet på fri-nodar, så rettinga gjev ikkje konstant maksimal latenstid.
Han finn ei passande enkeltblokk i gyldige fri-lister før bump, men kan
ikkje slå saman hol som er skilde av levande objekt. ARM64 sin eksisterande
klasseallokator er uendra. Windows deler x86-emisjonen, men prøvane over
er køyrde på Linux x86-64.
