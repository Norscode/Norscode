# Selvstendighet frå #179 — disiplinert, alltid-grøn veikart

**Baseline:** commit `52593ad` (PR #179, «Fix self-sufficiency seed verification
order»), siste fullt grøne tilstand. C-backa verktøykjede med fungerande GC.

**Kvifor denne linja:** b2-independence prøvde å fjerne C (og GC-en) i eit *big
bang* FØR den sjølvhosta erstatninga var ferdig → ~150 samtidige feil rota i ein
ufullstendig runtime (ingen GC). Denne greina byggjer selvstendighet inkrementelt
frå den grøne baselinen i staden.

## Grunnprinsipp

> **Fjern aldri ei C-avhengnad før den pure-Norscode-erstatninga er BEVIST grøn.
> CI-grønt er porten — kvart steg må passere.**

## Fasar

- **Fase 0 — Etabler porten.** Push denne greina → GitHub, stadfest CI grøn på
  baselinen. Utan ein fungerande grøn-port er «hald grønt» umogleg å verifisere.
- **Fase 1 — Port b2 sitt validerte, C-frie arbeid (grønt kvart steg).**
  Cherry-pick det som IKKJE treng GC og er bevist: språkfunksjonar (enum, lambda/
  closure, metode, destrukturering), krypto i rein Norscode (sha256/pbkdf2/web-
  push/TLS), ELF stage-0-forbetringar. Gjenvinn b2 sin ekte framgang, disiplinert.
- **Fase 2 — Fjern C-bibliotek, kvart gated.** sqlite→NorsDB (pure), Metal-GPU-
  C-runtime, legacy-C-backend. For kvar: bygg+verifiser pure erstatning grøn
  FØRST, deretter fjern C-en.
- **Fase 3 — Sjølvhosta seed-bygging (GC-en).** Fullfør sjølvhosta native codegen:
  legg inn GC (validert fundament, sjå AOT_GC_DESIGN nedanfor) + tett codegen-hull
  (crypto-gap-rute, JIT, minne-effektiv batched Mach-O mot OOM). Litmus: sjølvhosta
  codegen byggjer ein seed som passerer HEILE grøne suita (inkl. 10k-hmac). Først
  då: bytt seed-bygging C→sjølvhosta, fjern C-bootstrap.
- **Fase 4 — Siste C-fjerning + attestering.** Fjern gjenverande C; selvstendig-
  hets-gatane (L1–L6, active-surface) grøne med null C.

## Avgjerande sekvensering

Contained wins (Fase 1–2) FØRST — lågrisiko, held momentum, gir umiddelbar
selvstendighets-framgang utan å røre den skjøre runtime-en. Det harde GC/codegen-
byttet (Fase 3) SIST og fullt gated. Stikk motsett av b2 sin feil.

## GC-fundament (frå b2-undersøkinga, portast i Fase 3)

Validerte komponentar klare til port (b2-independence-commits, sjå
`docs/05-development/AOT_GC_DESIGN.md` der):
- Objekt-layout kartlagt (int/bool/streng/liste/map — peikarfelt + storleik).
- Mark-trace-logikk validert (gc_trace følgjer objekt-grafen korrekt).
- Reclaim-motor validert (gc_alloc fri-liste-pop + gc_free push, size-fit).
- Rot-skann (stakk er komplett rotsett; globals tomme).
- Empirisk: ~65KB/iter garbage-rate; bump-reset utrygt → full mark-sweep påkravd.
Att: allokeringsfri maskinkode-mark+sweep + re-pek allokatorar + safepoint-wiring.

## Status

- [x] Fase 0: roadmap dokumentert
- [ ] Fase 0: grøn CI-port stadfesta
- [ ] Fase 1 …
