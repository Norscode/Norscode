#!/usr/bin/env bash
# Build an isolated macOS native candidate for the mappe_opprett runtime gap.
# This is a maintainer lane: it writes only under build/v3009 and never
# promotes to dist/norscode_native or bootstrap/stage0.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT/build/v3009"
NCB_INPUT="${NCB_INPUT:-$ROOT/build/v9400/hybrid_compiler_bundle_v9400.ncb.json}"
GEN_C="$OUT_DIR/norscode_generated.c"
WRAP_C="$OUT_DIR/native_candidate_combined.c"
CANDIDATE="$OUT_DIR/norscode_native_macos_arm64_v3009"
MANIFEST="$OUT_DIR/native_macos_candidate_v3009.no"

if [ "$(uname -s)" != "Darwin" ]; then
  printf '[STOP] v3009 macOS-kandidat må byggjast på macOS, ikkje %s.\n' "$(uname -s)" >&2
  printf '[STOP] Ingen filer i dist/ eller bootstrap/stage0 vart endra.\n' >&2
  exit 2
fi

case "$(uname -m)" in
  arm64) ;;
  *)
    printf '[STOP] v3009 er berre verifisert for macOS arm64, ikkje %s.\n' "$(uname -m)" >&2
    exit 2
    ;;
esac

if ! command -v clang >/dev/null 2>&1; then
  printf '[FAIL] clang manglar. Installer Xcode Command Line Tools.\n' >&2
  exit 1
fi

if [ ! -f "$NCB_INPUT" ]; then
  printf '[FAIL] NCB-input manglar: %s\n' "$NCB_INPUT" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
cd "$ROOT"

printf '== v3009 macOS native candidate build ==\n'
printf 'root: %s\n' "$ROOT"
printf 'input: %s\n' "$NCB_INPUT"
printf 'output: %s\n' "$CANDIDATE"
printf 'regel: ikkje skriv til dist/norscode_native\n'
printf 'regel: ikkje skriv til bootstrap/stage0\n'

"$ROOT/bin/nc" check "$ROOT/archive/legacy_c_backend/ncb_to_c.no" >/dev/null
env NC_NCB_INPUT="$NCB_INPUT" NC_C_OUTPUT="$GEN_C" \
  "$ROOT/bin/nc" run "$ROOT/archive/legacy_c_backend/ncb_to_c.no" >/dev/null

# The Norscode C generator can emit dotted/non-ASCII C symbol names in this
# maintainer lane. Keep string dispatch names intact and sanitize only nc_fn_*
# symbols in the generated C.
perl -0pi -e 's/(nc_fn_[A-Za-z0-9_.\x80-\xFF]+)/do { my $x=$1; $x =~ s{[^A-Za-z0-9_]}{_}g; $x }/ge' "$GEN_C"

{
  printf '#define main nc_generated_main\n'
  printf '#include "norscode_generated.c"\n'
  printf '#undef main\n'
  printf '#include "../../archive/legacy_c_backend/nc_native_main.c"\n'
} >"$WRAP_C"

clang -O2 -Wno-everything -DNORSCODE_NATIVE_MAIN "$WRAP_C" -lsqlite3 -o "$CANDIDATE"
chmod +x "$CANDIDATE"

bash "$ROOT/tools/native_runtime_gap_gate_v3001.sh" "$CANDIDATE"
bash "$ROOT/tools/promote_native_stage0_v3001.sh" "$CANDIDATE" --dist --dry-run >/dev/null
bash "$ROOT/tools/promote_native_stage0_v3001.sh" "$CANDIDATE" --stage0 --dry-run >/dev/null

sha="$(shasum -a 256 "$CANDIDATE" | awk '{print $1}')"
size="$(wc -c < "$CANDIDATE" | tr -d ' ')"
stamp="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"

{
  printf 'native_candidate("schema", "norscode.native_candidate.v3009")\n'
  printf 'native_candidate("status", "macos_candidate_ready_not_promoted")\n'
  printf 'native_candidate("created_utc", "%s")\n' "$stamp"
  printf 'native_candidate("candidate", "build/v3009/norscode_native_macos_arm64_v3009")\n'
  printf 'native_candidate("size_bytes", "%s")\n' "$size"
  printf 'native_candidate("sha256", "%s")\n' "$sha"
  printf 'native_candidate("platform", "macos-arm64")\n'
  printf 'native_candidate("source_ncb", "%s")\n' "${NCB_INPUT#$ROOT/}"
  printf 'native_candidate("gate", "tools/native_runtime_gap_gate_v3001.sh build/v3009/norscode_native_macos_arm64_v3009")\n'
  printf 'native_candidate("gate_status", "green")\n'
  printf 'native_candidate("dry_run_dist", "green")\n'
  printf 'native_candidate("dry_run_stage0", "green")\n'
  printf 'native_candidate("dist_norscode_native", "unchanged")\n'
  printf 'native_candidate("bootstrap_stage0", "unchanged")\n'
  printf 'native_candidate("production_promoted", "false")\n'
} >"$MANIFEST"

printf '[OK] macOS-kandidat klar: %s\n' "$CANDIDATE"
printf '[OK] Manifest: %s\n' "$MANIFEST"
printf '[OK] Ingen promotering utført.\n'
