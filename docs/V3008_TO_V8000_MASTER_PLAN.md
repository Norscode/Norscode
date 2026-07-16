# Norscode v3008-v8000 masterplan

Status: plan og sjekkliste.

Hovudregel: ingen aktiv runtime, stage0, dist eller AI-autoskriving skal reknast ferdig før gate, dokumentasjon og rollback er på plass.

## Globale stopp-reglar

- Ikkje overskriv `bootstrap/stage0` utan eksplisitt ja.
- Ikkje overskriv `dist/norscode_native` utan eksplisitt ja.
- Ikkje kall `random_hex` produksjonsklar før aktiv runtime er grønn.
- Ikkje opne `exec_prosess` i produksjon utan whitelist/policy.
- Ikkje la AI skrive direkte til kritiske filer utan patch-plan, guard og menneskeleg godkjenning.
- Alle v-steg skal vere reine: eitt mål, eigne filer/rapportar, tydeleg status.

## Status ved start

- v3002 macOS-kandidat: grønn.
- v3003 kandidatmanifest: ferdig.
- v3004 dry-run promotering: ferdig.
- v3005 Linux-kandidatspor: klart, ikkje køyrt her.
- v3006 promotering-sjekkliste: ferdig.
- v3007 aktiv-runtime-status: ferdig.
- Aktiv `dist/norscode_native`: ikkje påstått grønn.
- `bootstrap/stage0`: ikkje påstått grønn.
- AI self-programming: ikkje klar.

---

# Fase 1: v3008-v3200 AI-sikker grunnmur

## v3008-v3020 AI self-programming status

Mål: definere kva AI får lov til før aktiv runtime er trygg.

Sjekkliste:

- [ ] `docs/V3008_AI_SELF_PROGRAMMING_STATUS.md`
- [ ] `ai_can_write_files=false`
- [ ] `ai_can_generate_patch_plan=true`
- [ ] `ai_requires_human_approval=true`
- [ ] `production_ready=false`
- [ ] `reason=active_runtime_not_promoted_and_no_safe_patch_guard`

Ferdig når:

- [ ] AI kan lage plan, men ikkje auto-endre kritiske filer.
- [ ] Dokumentet skil mellom patch-plan og faktisk patch.

## v3021-v3050 AI patch guard

Mål: lage guard som stoppar farlege patchar.

Sjekkliste:

- [ ] `tools/ai_patch_guard_v3021.no`
- [ ] Blokker sletting av filer som standard.
- [ ] Blokker endring av `dist/`.
- [ ] Blokker endring av `bootstrap/stage0/`.
- [ ] Blokker endring av `archive/` utan eksplisitt flagg.
- [ ] Blokker store masseendringar utan manifest.
- [ ] Tillat docs-only endringar.
- [ ] Tillat test/probe-script endringar.

Ferdig når:

- [ ] Guard kan køyre mot ei patchfil.
- [ ] Guard gir forklaring per blokkering.

## v3051-v3100 AI patch-plan format

Mål: standardisere patchforslag som tekst før endring.

Sjekkliste:

- [ ] `docs/AI_PATCH_PLAN_FORMAT.md`
- [ ] `patch_plan_id`
- [ ] `mål`
- [ ] `filer_som_endrast`
- [ ] `risiko`
- [ ] `rollback`
- [ ] `tester`
- [ ] `menneskeleg_godkjenning`

Ferdig når:

- [ ] AI kan produsere patch-plan utan å skrive kode.
- [ ] Planen kan vurderast av menneske før patch.

## v3101-v3200 AI write sandbox

Mål: gi AI eit trygt område der han kan skrive utan å påverke runtime.

Sjekkliste:

- [ ] `ai_workspace/`
- [ ] `ai_workspace/patch_plans/`
- [ ] `ai_workspace/drafts/`
- [ ] `ai_workspace/reports/`
- [ ] Ingen automatisk kopiering ut av sandbox.

Ferdig når:

- [ ] AI kan lage forslag i sandbox.
- [ ] Kritiske filer er framleis sperra.

---

# Fase 2: v3201-v3600 Aktiv runtime og promotering

## v3201-v3250 Aktiv dist dry-run

Mål: kontrollere kva som skjer før faktisk dist-promotering.

Sjekkliste:

- [ ] `tools/native_active_status_v3007.no`
- [ ] Dry-run mot macOS-kandidat.
- [ ] SHA256 før promotering.
- [ ] SHA256 etter eventuell promotering.
- [ ] Backup-path dokumentert.

Ferdig når:

- [ ] Dry-run viser kva som ville bli endra.
- [ ] Ingen filer er endra.

## v3251-v3300 macOS dist promotering

Mål: promotere macOS-kandidat til aktiv `dist` berre etter eksplisitt ja.

Sjekkliste:

- [ ] Brukar har sagt eksplisitt ja.
- [ ] Kandidat-gate er grønn.
- [ ] Backup av eksisterande `dist/norscode_native`.
- [ ] Kopier kandidat til `dist/norscode_native`.
- [ ] Køyr aktiv dist-gate.
- [ ] Oppdater statusdokument.

Ferdig når:

- [ ] `NORSCODE_NATIVE_GAP_BIN=dist/norscode_native ./bin/nc run tools/native_runtime_gap_gate_v3001.no` er grønn.
- [ ] `random_hex` kan kallast grønn i aktiv dist.

## v3301-v3400 stage0 promotering

Mål: promotere stage0 per plattform, ikkje globalt.

Sjekkliste:

- [ ] Brukar har sagt eksplisitt ja.
- [ ] Aktiv dist er grønn.
- [ ] Plattform er kontrollert.
- [ ] Backup av eksisterande stage0-binær.
- [ ] Promoter berre riktig plattformfil.
- [ ] Køyr smoke-test.

Ferdig når:

- [ ] Stage0 for målplattform er grønn.
- [ ] Rollback er dokumentert.

## v3401-v3600 Linux runtime-kandidat

Mål: byggje og gate Linux-kandidat på Linux.

Sjekkliste:

- [ ] Køyr `tools/build_linux_native_candidate_v3005.no` på Linux.
- [ ] Manifest er skrive.
- [ ] Runtime-gap gate er grønn.
- [ ] Linux SHA256 dokumentert.
- [ ] Ingen automatisk stage0-promotering.

Ferdig når:

- [ ] Linux-kandidat er grønn.
- [ ] Linux-promotering er separat godkjenningssteg.

---

# Fase 3: v3601-v4200 Runtime builtins ferdigstilling

## v3601-v3700 random og tid

Mål: ferdigstille random/tid API.

Sjekkliste:

- [ ] `builtin.random_hex(n)`
- [ ] `builtin.random_bytes(n)`
- [ ] `builtin.uuid()`
- [ ] `builtin.tid_ms()`
- [ ] `builtin.tid_no()`
- [ ] `builtin.now()`
- [ ] `builtin.timestamp()`
- [ ] Plattformtest macOS.
- [ ] Plattformtest Linux.

Ferdig når:

- [ ] API er dokumentert.
- [ ] Aktiv runtime og stage0 er grønn.

## v3701-v3800 exec_prosess policy

Mål: trygg prosesskøyring.

Sjekkliste:

- [ ] DEV-only default.
- [ ] `NORSCODE_ENABLE_EXEC_PROSESS=1`.
- [ ] Ingen shell i produksjon som default.
- [ ] Whitelist-design.
- [ ] Timeout.
- [ ] Maks output-storleik.
- [ ] Exit-code struktur.
- [ ] Logging.

Ferdig når:

- [ ] DEV fungerer.
- [ ] Produksjon er sperra utan policy.

## v3801-v3900 socket runtime

Mål: stabil socket-API.

Sjekkliste:

- [ ] `socket_listen`
- [ ] `socket_accept`
- [ ] `socket_read`
- [ ] `socket_write`
- [ ] `socket_close`
- [ ] Timeout.
- [ ] Port-binding feilhandtering.
- [ ] macOS test.
- [ ] Linux test.

Ferdig når:

- [ ] Web serve kan bruke socket-laget stabilt.

## v3901-v4200 fil/json/map/tekst stabilisering

Mål: rydde runtime-basics.

Sjekkliste:

- [ ] JSON parse/stringify robust.
- [ ] Map keys/values fungerer.
- [ ] Unicode/tekst dokumentert.
- [ ] Fil-les/skriv/slett policy.
- [ ] Feilmeldingar er forståelege.
- [ ] Testpakke for alle basale builtins.

Ferdig når:

- [ ] Selfhost og web-app brukar same runtime-reglar.

---

# Fase 4: v4201-v5000 Selfhost, compiler og stage0 kvalitet

## v4201-v4400 Parser og semantic rydding

Sjekkliste:

- [ ] Ingen `navn`/`namn`-mismatch.
- [ ] Dot-access vs map-access standard.
- [ ] Import-konfliktar dokumentert.
- [ ] Feilrapport med linje/kolonne.
- [ ] Semantisk pass for ukjende variablar.

Ferdig når:

- [ ] Compiler stoppar tidleg med god feil.

## v4401-v4600 IR/bytecode stabilisering

Sjekkliste:

- [ ] Funksjonar etter import blir ikkje droppa.
- [ ] `returner` utan verdi er definert.
- [ ] Closure/lambda status.
- [ ] Loop labels robuste.
- [ ] Try/fang kodegen robust.

Ferdig når:

- [ ] Store selfhost-filer kompilerer stabilt.

## v4601-v4800 NCB format v1

Sjekkliste:

- [ ] `entry`
- [ ] `functions`
- [ ] `metadata`
- [ ] `route_handlers`
- [ ] `imports`
- [ ] Versjonsfelt.
- [ ] Kompatibilitetsreglar.

Ferdig når:

- [ ] NCB kan validerast før run.

## v4801-v5000 Stage0 release discipline

Sjekkliste:

- [ ] Stage0 build manifest.
- [ ] Per-plattform checksums.
- [ ] Rollback-script.
- [ ] Smoke-test før release.
- [ ] Release-notat.

Ferdig når:

- [ ] Stage0 kan oppgraderast utan handarbeid og utan gjetting.

---

# Fase 5: v5001-v5800 Web og admin ferdigstilling

## v5001-v5200 Web route standard

Sjekkliste:

- [ ] Alle routes bruker `web.route("GET /...")`.
- [ ] Ingen gamle `route(path)`-variantar.
- [ ] `/`
- [ ] `/docs`
- [ ] `/download`
- [ ] `/status`
- [ ] `/kontakt`
- [ ] `/status.json`

Ferdig når:

- [ ] Curl-testpakke er grønn.

## v5201-v5400 Admin DEV til trygg auth-plan

Sjekkliste:

- [ ] DEV-login tydeleg merka.
- [ ] Fast session berre i DEV.
- [ ] Logout.
- [ ] Admin guard.
- [ ] Produksjonssperre.
- [ ] Auth-plan for random-backed session.

Ferdig når:

- [ ] Ingen kan mistolke DEV-login som produksjon.

## v5401-v5600 CMS demo

Sjekkliste:

- [ ] `data/pages/home.txt`
- [ ] `data/pages/about.txt`
- [ ] `data/pages/docs.txt`
- [ ] Admin lesing.
- [ ] Admin redigering berre DEV.
- [ ] Backup før skriv.

Ferdig når:

- [ ] CMS-demo kan brukast lokalt utan risiko for runtime.

## v5601-v5800 Norscode.no innhald

Sjekkliste:

- [ ] Hjem
- [ ] Docs
- [ ] Download
- [ ] Status
- [ ] Kontakt
- [ ] Om
- [ ] Personvern
- [ ] Vilkår

Ferdig når:

- [ ] Webside er komplett nok for publisering.

---

# Fase 6: v5801-v6500 AI-assistent funksjonar

## v5801-v6000 AI dashboard

Sjekkliste:

- [ ] `/ai/status`
- [ ] `/ai/minne`
- [ ] `/ai/plan`
- [ ] `/ai/logg`
- [ ] DEV-only admin guard.
- [ ] Ingen auto-skriving.

Ferdig når:

- [ ] AI-status kan visast trygt i web/admin.

## v6001-v6200 AI minnevisning

Sjekkliste:

- [ ] Les minnefil trygt.
- [ ] HTML-escape.
- [ ] Maks filstorleik.
- [ ] Ingen skriving.

Ferdig når:

- [ ] Minne kan visast utan XSS eller filrisiko.

## v6201-v6350 AI kodegenerator demo

Sjekkliste:

- [ ] `/kode/hello`
- [ ] `/kode/kalkulator`
- [ ] `/kode/webside`
- [ ] Output som tekst.
- [ ] Ingen automatisk filskriving.

Ferdig når:

- [ ] Generator er demo, ikkje auto-patcher.

## v6351-v6500 AI patch-forslag

Sjekkliste:

- [ ] Patch-plan blir skrive til sandbox.
- [ ] Guard køyrer mot planen.
- [ ] Menneske godkjenner.
- [ ] Først deretter kan patch brukast.

Ferdig når:

- [ ] AI kan foreslå endringar trygt.

---

# Fase 7: v6501-v7200 Testing, CI og release

## v6501-v6700 Testpakker

Sjekkliste:

- [ ] Runtime builtins.
- [ ] Compiler.
- [ ] Web routes.
- [ ] Admin DEV.
- [ ] AI sandbox.
- [ ] Stage0 smoke.

Ferdig når:

- [ ] Alle testpakker kan køyrast separat.

## v6701-v6900 CI lokal først

Sjekkliste:

- [ ] `tools/check_all_safe.sh`
- [ ] Ingen destruktive steg.
- [ ] Ingen stage0-promotering.
- [ ] Ingen dist-promotering.
- [ ] Klar exit-code.

Ferdig når:

- [ ] Lokal CI kan køyrast før release.

## v6901-v7100 Release bundle

Sjekkliste:

- [ ] Binær.
- [ ] Manifest.
- [ ] Checksums.
- [ ] Docs.
- [ ] Changelog.
- [ ] Rollback.

Ferdig når:

- [ ] Release kan pakkast utan manuell jakt.

## v7101-v7200 Installer/oppdatering

Sjekkliste:

- [ ] Installer-script.
- [ ] Update-script.
- [ ] Backup før update.
- [ ] Rollback ved feil.
- [ ] Plattformdeteksjon.

Ferdig når:

- [ ] Brukar kan oppdatere trygt.

---

# Fase 8: v7201-v8000 Produksjonsklar Norscode

## v7201-v7400 Security hardening

Sjekkliste:

- [ ] Secure random aktiv.
- [ ] Auth ikkje DEV-only.
- [ ] Cookie flags.
- [ ] CSRF-plan.
- [ ] Exec policy.
- [ ] Filsystem-policy.
- [ ] Logg-policy.

Ferdig når:

- [ ] Produksjonsrisikoar er dokumentert og sperra.

## v7401-v7600 Dokumentasjon komplett

Sjekkliste:

- [ ] Språksyntaks.
- [ ] CLI.
- [ ] Web.
- [ ] Stage0.
- [ ] Runtime builtins.
- [ ] AI assistant.
- [ ] Troubleshooting.

Ferdig når:

- [ ] Ein ny brukar kan installere og køyre Norscode.

## v7601-v7800 Stabil release candidate

Sjekkliste:

- [ ] macOS arm64.
- [ ] macOS x86_64 hvis relevant.
- [ ] Linux x86_64.
- [ ] Linux arm64 hvis relevant.
- [ ] Alle gates grøne.
- [ ] Alle manifests skrivne.
- [ ] Rollback testa.

Ferdig når:

- [ ] Release candidate kan frysast.

## v7801-v8000 v1-ready

Sjekkliste:

- [ ] Aktiv runtime grønn.
- [ ] Stage0 grønn.
- [ ] Web grønn.
- [ ] AI sandbox grønn.
- [ ] Security checklist grønn.
- [ ] Docs komplett.
- [ ] Release bundle komplett.
- [ ] `production_ready=true` berre dersom alle over er sanne.

Ferdig når:

```text
norscode_v8000_status=ready
production_ready=true
runtime_gap=closed
stage0=green
dist=green
ai_self_programming=sandboxed_with_human_approval
release_candidate=frozen
```

---

# Dagleg arbeidsregel fram til v8000

For kvart nytt v-steg:

- [ ] Lag berre eitt reint mål.
- [ ] Skriv eller oppdater doc/status.
- [ ] Lag script/test berre dersom det er nødvendig.
- [ ] Ikkje promoter utan eksplisitt ja.
- [ ] Ikkje bland kandidat og aktiv runtime.
- [ ] Skriv ferdig-status med `production_ready`.
- [ ] Gå vidare til neste v.
