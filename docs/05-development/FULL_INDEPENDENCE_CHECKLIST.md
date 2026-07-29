# Full Independence Checklist

Målet med denne sjekklista er å gjere Norscode fullt uavhengig i praksis og på dokumentnivå:

- normal bruk skal ikkje peike på legacy eller maintainer-flater
- normal CI skal ikkje krevje særskilde vedlikehaldslaner
- bootstrap/seed-vedlikehald skal vere historisk/arkivert, ikkje del av normal drift
- alle aktive statusdokument skal skildre normalvegen som den einaste operative kjeda

## Status no

- [x] Normalvegen er native-first og selfhost-basert
- [x] LSP er ferdig og markert som ferdig i checklisten
- [x] CI er grønn
- [x] README seier at C-bruk er ein isolert vedlikehaldsbru, ikkje normal drift
- [x] Roadmap seier at legacy bootstrap/runtime er arkivert og isolert frå normalflyt
- [x] Migrasjons-/deprecationsdokument omtalar maintainer-sporet som historisk/isolert
- [x] Bootstrap-/maintainer-skript og bootstrap-README-ar omtalar no vedlikehald som historisk/isolert
- [x] Workflow-filene er omformulert til historisk / opt-in vedlikehaldsspråk
- [x] Full-uavhengigheitssporresten er no avgrensa til den bevisste historiske vedlikehaldsvegen
- [x] Det som no står att er den eksplisitte vedlikehaldsflata og arkivert historikk
- [x] Normal bruk er fri for skjult legacy-/maintainer-beslag
- [ ] Alle aktive docs utanfor arkiv skal vere fri for formuleringar som antydar at maintainer-regen er nødvendig for normal drift
- [ ] Alle aktive workflow-filene skal berre omtale maintainer-lane som opt-in eller historisk
- [ ] Alle aktive verktøy som peikar på bootstrap-/legacy-flata skal anten vere arkiverte eller merkte som eksplisitt vedlikehald
- [ ] Ingen normal driftsdokumentasjon skal referere til `NORSCODE_BOOTSTRAP_C` som ein del av standardvegen
- [ ] Ingen normal driftsdokumentasjon skal referere til `REGEN=1` som ein del av standardvegen
- [ ] Det skal vere ein tydeleg, dokumentert skilnad mellom:
  - normal bruk
  - historisk vedlikehald
  - arkivert legacy

## Arbeidsrekkefølgje

1. Kartlegg alle aktive referansar til maintainer-, seed- og bootstrap-bru.
   - sjå [FULL_INDEPENDENCE_INVENTORY.md](FULL_INDEPENDENCE_INVENTORY.md)
2. Flytt kvar referanse til anten:
   - historisk dokumentasjon
   - arkiv
   - eller eksplisitt vedlikehaldsdocs
3. Stram inn README, ROADMAP og dei viktigaste statusdokumenta til å omtale berre normalvegen.
4. Revider workflow-filene slik at maintainer-lane ikkje framstår som nødvendig drift.
5. Verifiser sluttstatus med søk etter:
   - `maintainer`
   - `seed-fornying`
   - `NORSCODE_BOOTSTRAP_C`
   - `REGEN=1`
   - `bootstrap/maint/c`
6. Når normaldocs er reine, vurder om dei historiske filene skal flyttast til `/docs/_archive` eller få tydelegare historikkmerking.

## Prinsipp

- Historikk skal bevarast.
- Normal drift skal vere fri for legacy-beslag.
- Vedlikehald kan finnast, men skal vere eksplisitt, isolert og aldri framstilt som standardveg.
