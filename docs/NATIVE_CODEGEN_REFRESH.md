# Oppfrisking av native codegens

`tools/refresh_native_codegen.no` byggjer codegen-artefakta for den faktiske
verten han køyrer på. Støtta vertar er Linux x86-64 og macOS ARM64. Køyr
verktøyet i ein eigen checkout etter at kjeldeendringane er samla:

```sh
NORSCODE_ROOT="$PWD" ./bin/nc run tools/refresh_native_codegen.no
```

Runtimen må vere materialisert som `dist/norscode_native`, eller valt med
`NORSCODE_NATIVE_BIN`. Verktøyet brukar berre Norscode sine fil-, hash- og
prosessfunksjonar. Han byggjer ikkje stage0 eller runtime-payloadar og
commit-ar eller push-ar ingenting.

## Portar og rekkjefølgje

1. Identifiser faktisk vert med `system_info`. Les den eksisterande
   verts-codegenen og krev at SHA-256 er lik den eintydige, fullstendige
   pin-linja i `tools/active_surface_allowlist.txt`. Denne Gen0-binæren kan
   vere eldre enn kjelda; det er den eksplisitte bootstrap-inngangen.
2. Ta kopi av Gen0 under `build/codegen-refresh/` og bundle gjeldande
   codegen-kjelde til éin NCB. Rapporten identifiserer òg runtime-binæren
   som utførte dette frontend-steget.
3. La Gen0 byggje Gen1, Gen1 byggje Gen2 og Gen2 byggje Gen3 frå same NCB.
   Kvar prosess må fullføre, skrive ein binær og rapportere null uløyste
   builtins med tom kvitliste. Manglande rapport er feil.
4. Krev både lik SHA-256 og byteidentisk Gen2/Gen3. Kjeldenes `.srchash`
   og `.depshash` må framleis vere like dei som vart tekne før bygget.
   Før dette punktet blir ingen spora binær, hashfil eller pin endra.
5. Installer verifisert Gen2 som verts-codegen, skriv kjeldehashar og
   oppdater berre pin-linja for denne fila. Den eksisterande
   `codegen_prebuilt.vel`-porten må godta resultatet.
6. Køyr `tools/build_cross_codegen.no` for dei eksisterande, eigne
   kryss-binærane til denne verten. Verktøyet skriv `.vertcg` frå den
   faktiske verts-codegenen. Sjekk null uløyste builtins, uendra kjelder,
   ferskheitsporten og proveniens før kryss-pinnane blir oppdaterte.

Med dagens artefaktsett byggjer Linux-jobben verts-codegen og dei to
Linux-køyrbare kryss-codegenane for Linux ARM64 og macOS ARM64. Mac-jobben
byggjer Mac-verts-codegen og dei to Mac-køyrbare kryss-codegenane for Linux
x86-64 og NorscodeOS x86-64. Ein Linux-køyring skriv aldri Mac-vertsproveniens.

## Resultat og feil

`build/codegen-refresh/report.json` inneheld faktisk vert, kjeldehashar,
Gen0-identitet, generasjonshashar, byte-storleikar, fikspunktstatus og
artefaktproveniens. Kvart barn får eigen `.log`. Berre status `ok` betyr
at heile vertssettet er oppdatert. Ved feil blir status `failed`, årsaka
blir lagra, og verktøyet avsluttar med kode 1.

Ei feilande kryssbygging kan skje etter at det verifiserte vertsfikspunktet
er installert. Bruk difor ein eigen checkout og promoter berre eit komplett,
grønt artefaktsett. Verktøyet avviser endringar i allowlist undervegs, slik
at han ikkje overskriv ein annan prosess sine pin-oppdateringar.

`tests/test_refresh_native_codegen.no` kontrollerer pin- og rapportportane,
inkludert ein feilpinna Gen0 som må avvisast før binær eller hashfiler blir
endra. Linux-løypa er i tillegg køyrd gjennom reelle Gen1–Gen3 og begge
kryssbygg. Mac-løypa må køyrast på ein faktisk Mac-vert.

Dette fikspunktet gjeld native codegen frå same kjelde-NCB. Frontend-
sjølvhosting og kjeldebygd nc_main blir framleis verifiserte av dei eigne
djupe bootstrap-portane. Ein grøn codegen-refresh åleine erstattar ikkje
desse portane eller native funksjonstestar av nye runtime-endringar.
