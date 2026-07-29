# v1001-v3000 masterplan

Dette er hovudplanen fram til v3000.

Planen er med vilje delt i reine blokker. Kvar blokk skal kunne testast, dokumenterast og rullast tilbake utan å blande web-demo, runtime, stage0 og native release.

## Grunnreglar

- Ikkje overskriv `dist/norscode_native` automatisk.
- Ikkje overskriv `bootstrap/stage0` automatisk.
- Ikkje kall `random_hex` ferdig før native/stage0 faktisk støttar sikker random.
- Alt med fast session-id er DEV-only.
- Produksjonsstatus skal vere `production_ready: false` til runtime-gate er grønn.

## v1001-v1200: stabilisering

- Samle v-status i docs og web.
- Halde `/status.json`, `/native-status`, `/runtime/gate` og `/v3000.json` konsistente.
- Standardisere web-testar.
- Merke alle runtime-gap tydeleg.

## v1201-v1600: native/stage0 design

- Lage design for stage0 random.
- Lage design for tid/now/timestamp.
- Lage policy for exec_prosess.
- Lage Linux/macOS probe-script.
- Ikkje skrive ny stage0 før eksplisitt godkjenning.

## v1601-v2000: web og CMS

- Gjere Norscode.no-sidene meir komplette.
- Utvide CMS-modellen utan ekte produksjonsauth.
- Lage download-/release-side som ikkje lovar meir enn artefakta faktisk støttar.
- Halde personvern, vilkår, kontakt og om-sider oppdaterte.

## v2001-v2400: AI-assistent

- Utvide AI status-dashboard.
- Vise minne og plan trygt.
- La AI lage patch-planar som tekst.
- Hindre automatisk sletting, stage0-overstyring og dist-endring.

## v2401-v2700: produksjonslogin og sikkerhet

- Bytte bort fast DEV-session.
- Koble login til sikker random når `random_hex` er grønn.
- Legge inn session-expiry, logout, cookie-policy og admin-guard.
- Lage test for at DEV-login ikkje kan feilmerkast som produksjon.

## v2701-v3000: release candidate

- Full testmatrise for web, runtime og native.
- Release checklist.
- Backup og rollback.
- Handover-dokument.
- `production_ready` kan berre bli `true` dersom alle gates er grøne.

