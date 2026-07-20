# std_fil_patch.no — v36

Denne fila er ein trygg patch-kopi av std/fil.no.

V36 mål:
- patch-kopien skal ha same offentlege funksjonar som originalen
- patch-kopien skal kunne validerast i v37
- original std/fil.no skal ikkje endrast

Offentlege funksjonar i kopien:
- finnes
- skriv_fil
- les_fil
- slett_fil
- status

Neste steg:
1. printf "valider std patch Norscode fil" > spm.txt && nc run ai.no
2. printf "foreslå kontrollert std endring Norscode fil" > spm.txt && nc run ai.no
3. vent med direkte endring av std/fil.no
