# AI patch plan format

Status: standardformat for patch-forslag før faktisk filendring.

## Formål

AI skal kunne foreslå endringar utan å skrive direkte til kritiske område. Denne malen gjer patch-planar lesbare, vurderbare og trygge å godkjenne.

## Påkravde felt

```text
patch_plan_id:
mål:
filer_som_endrast:
risiko:
rollback:
tester:
menneskeleg_godkjenning:
```

## Tilrådd mal

```text
patch_plan_id: vXXXX-samandrag
mål: Kort beskriving av kva planen skal løyse.
filer_som_endrast:
- path/til/fil1
- path/til/fil2
risiko:
- låg | middels | høg
- kva som kan gå gale
rollback:
- kva som skal reverserast
- backup eller restore-strategi
tester:
- kva som skal køyrast etter patch
- kva som ikkje er testa enno
menneskeleg_godkjenning: kravd
```

## Reglar

- Ein patch-plan er ikkje ein patch.
- Ein patch-plan skal kunne lesast utan å opne diffen.
- Kritiske stiar skal nemnast eksplisitt dersom dei blir rørte.
- Dersom planen rører `dist/` eller `bootstrap/stage0/`, skal han normalt stoppast av patch guard og krevje eksplisitt menneskeleg godkjenning.

## Minstekrav før patch

- Mål er tydeleg.
- Filer er opplista.
- Risiko er nemnd.
- Rollback er definert.
- Test/gate er nemnd.
- Menneskeleg godkjenning er eksplisitt markert.
