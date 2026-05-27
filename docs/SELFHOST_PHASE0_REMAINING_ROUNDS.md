# Fase 0: Resten i omganger

Mål:
Få Fase 0 i mål ved å rydde bort gjenværende Python-avhengigheter, stabilisere CI og gjøre normalflyten tydelig Python-fri.

Dette dokumentet er en praktisk arbeidsliste. Hver omgang er liten nok til å kunne fullføres og verifiseres separat.
Det hører til som omgang 0 i [SELFHOST_FULL_AUTONOMY_PLAN.md](SELFHOST_FULL_AUTONOMY_PLAN.md).

## Kort status

- `bin/nc` og `bin/bootstrap` er allerede strammet inn mot native-first flyt.
- Legacy Python-fallback er fortsatt tilgjengelig for overgangsarbeid.
- CI og release-workflows peker nå i hovedsak på `bin/nc` for bootstrap-gate.
- CI har egen `phase0-verify`-gate; siste åpne punkt er ekstern grønn-bekreftelse i GitHub Actions.
- Linux-release bygger ikke lenger via Docker, og Windows-jobben i `ci.yml` bruker en ren test.
- Historiske Python-referanser finnes fortsatt i arkiv- og bakgrunnsdokumentasjon.

## Omgang 1 — Fiks CI-feilene som blokkerer Fase 0

Mål:
Få workflowene til å bruke riktig bane for hver plattform.

Leveranser:

- [x] macOS-jobben bruker `./bin/nc --legacy-python-fallback selfhost-bootstrap-gate` i stedet for gammel Python-vei.
- [x] Linux-jobben fjerner eller erstatter Docker-steget som fortsatt peker på slettet C-kode.
- [x] Windows-jobben bruker en test som ikke krever manglende `std.web`.
- [ ] Alle CI-jobber er grønne igjen.

Ferdig når:

- [ ] CI kjører uten røde jobber på macOS, Linux og Windows.
- [ ] Ingen av de kjente Fase 0-feilene er igjen i workflow-logikken.

## Omgang 2 — Rydd ut daud Python og gamle bootstrap-spor

Mål:
Fjerne filer og referanser som bare finnes som historisk ballast.

Leveranser:

- [x] `setup.py` er fjernet eller gjort helt overflødig.
- [x] Gamle pytest-filer som peker på slettet C/legacy-VM er ryddet bort.
- [x] `tools/export_ncbb_as_bin.py`, `export_ncbb_as_c.py`, `generate_linux_bootstrap_artifacts.py`, `v42_trace_probe.py` er enten fjernet eller flyttet til tydelig legacy-lag.
- [x] `build/norcode-native-stage0.py` er borte eller erstattet av ny native flyt.
- [x] Eventuelle gamle alias-/compat-filer som ikke lenger trengs i normal flyt er fjernet.

Ferdig når:

- [x] Repoet har ikke lenger daud Python som er aktivt nevnt i Fase 0-lista.
- [x] Det er tydelig hvilke filer som fortsatt er legacy, og hvorfor de finnes.

## Omgang 3 — Gjør legacy Python tydelig og begrenset

Mål:
Sikre at Python bare er en eksplisitt overgangsbane, ikke en skjult standard.

Leveranser:

- [x] Alle direkte lenker til den gamle `main.py`-veien i docs og workflows er byttet ut eller forklart som legacy.
- [x] `--legacy-python-fallback` er den tydelige navngitte fallbacken i CLI.
- [x] `bin/bootstrap` bruker bare `bin/nc` som siste utvei, ikke direkte Python-entrypoint.
- [x] README og CI-dokumentasjon beskriver normalbane først og legacy-bane tydelig etterpå.

Ferdig når:

- [x] En ny bidragsyter ser umiddelbart hva som er normal vei og hva som er legacy.
- [x] Python kan ikke forveksles med primær brukerflyt.

## Omgang 4 — Verifiser at Fase 0 faktisk er ferdig

Mål:
Låse statusen med en enkel, repeterbar sjekk.

Leveranser:

- [x] En kort Fase 0-sjekkliste kan kjøres manuelt eller i CI.
- [x] Sjekken bekrefter at den aktive flaten er fri for kjente fase-0-blokkere.
- [ ] Sjekken bekrefter at CI er grønn i GitHub Actions.
- [x] Sjekken bekrefter at normalflyten er native-first.
- [x] Sjekken bekrefter at kun eksplisitt legacy-fallback kan bruke Python.

Verifiser med:

```bash
bash tools/selfhost_phase0_verify.sh
```

Ferdig når:

- [ ] Fase 0 kan markeres som fullført i ROADMAP.md uten forbehold.
- [x] Neste omgang kan ta over uten at det er uklart hva som er igjen.

## Anbefalt rekkefølge

1. CI-feilene først
2. Daud Python og bootstrap-spor
3. Legacy-avgrensing
4. Sluttverifisering

## Kortversjon

Det som gjenstår i Fase 0 er i praksis:

- få alle workflows grønne
- rydde bort eventuelle gjenværende historiske Python-referanser i arkivdokumentasjon
- holde Python kun som eksplisitt legacy-fallback
- verifisere at normalflyten er native-first
