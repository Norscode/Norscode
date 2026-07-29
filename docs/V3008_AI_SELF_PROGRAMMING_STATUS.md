# v3008 AI self-programming status

Status: AI kan planleggje og foresla endringar, men ikkje skrive fritt til kritiske filer.

## Samandrag

AI i Norscode er ikkje sjølvprogrammerande i produksjonsforstand enno. På dette steget er AI avgrensa til trygg planlegging, rapportering og patch-forslag. Kritiske filer og aktiv runtime skal framleis vere verna av menneskeleg godkjenning.

## Gjeldande status

```text
ai_can_write_files=false
ai_can_generate_patch_plan=true
ai_requires_human_approval=true
production_ready=false
reason=active_runtime_not_promoted_and_no_safe_patch_guard
```

## Kva AI har lov til i v3008

- Lage patch-plan som tekst.
- Lage docs, rapportar og statusfiler.
- Lage test/probe-script som ikkje promoterer runtime.
- Lage dry-run-script og kontrollscript.
- Foreslå endringar i sandbox eller utkast-format.

## Kva AI ikkje har lov til i v3008

- Skrive direkte til `dist/`.
- Skrive direkte til `bootstrap/stage0/`.
- Promotere kandidat til aktiv runtime.
- Aktivere produksjons-login eller produksjonsautentisering.
- Gjere `exec_prosess` produksjonsopen.
- Auto-applisere patchar mot kritiske filer utan menneskeleg godkjenning.

## Kritiske område

Desse områda skal behandlast som sperra inntil patch guard og godkjenningsflyt er på plass:

- `dist/`
- `bootstrap/stage0/`
- aktive native binærar
- release-bundles
- install/update-script som påverkar brukarens aktive miljø

## Skilje mellom patch-plan og patch

Ein patch-plan er eit tekstleg forslag som minst skal beskrive:

- mål
- filer som blir rørte
- risiko
- rollback
- test/gate
- krav om menneskeleg godkjenning

Ein faktisk patch er ei filendring. I v3008 er patch-plan tillaten, medan faktisk patch mot kritiske område ikkje er tillaten utan seinare guard-steg.

## Produksjonsstatus

Så lenge aktiv `dist/norscode_native` og `bootstrap/stage0` ikkje er promotert og verifisert, skal denne statusen stå fast:

```text
production_ready=false
```

## Neste steg

- v3021-v3050: bygg patch guard som stoppar farlege endringar.
- v3051-v3100: standardiser patch-plan-format.
- v3101-v3200: gi AI eit trygt sandbox-område for utkast og forslag.
