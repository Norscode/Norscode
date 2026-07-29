# v3000 release gate

## Current status on 2026-06-27

The active AIKernel production gate is green for the current macOS arm64 host:

```json
{
  "production_ready": true,
  "source": "prosjekter/NorscodeAIKernel/status/production_readiness.json",
  "gate": "tools/production_readiness_gate.sh --full"
}
```

The older status below is historical context from before the full production gate was introduced.

Norscode v3000 kan berre kallast release candidate dersom dette er sant.

## Må vere grønt

- `builtin.random_hex(n)` verkar i native/stage0 og brukar sikker OS-random.
- `builtin.random_bytes(n)` eller tilsvarande runtime-kjelde er definert.
- `builtin.uuid()` er anten ferdig eller eksplisitt ute av scope.
- `tid`, `now` og `timestamp` har stabil kontrakt.
- `exec_prosess` er av i produksjon eller låst med allowlist, timeout og output-grense.
- Admin-login brukar ikkje fast DEV-session.
- `/status.json` viser ikkje skjulte runtime-gap.
- Backup og rollback er dokumentert.

## Historical no-status

```json
{
  "production_ready": false,
  "reason": "stage0_mangler_random_hex",
  "native_source_patch": "v3001_candidate_only",
  "dist_promoted": false,
  "stage0_promoted": false
}
```

## Historical conclusion

v3000-planen er på plass. v3001 har kjeldepatch for maintainer-lane, men produksjonsklar runtime er blokkert til ein grønn kandidat er promotert til `dist/norscode_native` og seinare til stage0-seed med eksplisitt godkjenning.
