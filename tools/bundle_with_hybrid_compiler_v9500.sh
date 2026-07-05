#!/usr/bin/env bash
set -euo pipefail

FROM_NO=0
if [ "${1:-}" = "--from-no" ]; then
  FROM_NO=1
  shift
fi
ROOT="${1:-}"
OUT="${2:-}"
ENTRY="${3:-}"
NODE_BIN="${NODE_BIN:-}"
shift 3 || true

if [ -z "$ROOT" ] || [ -z "$OUT" ]; then
  echo "bruk: bundle_with_hybrid_compiler_v9500.sh <repo-root> <out.ncb.json> <entry> <alias=fil.no>..." >&2
  exit 2
fi

if [ "$FROM_NO" != "1" ]; then
  SPECS=""
  for spec in "$@"; do
    if [ -z "$SPECS" ]; then
      SPECS="$spec"
    else
      SPECS="$SPECS $spec"
    fi
  done
  exec env \
    NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_HYBRID_ROOT="$ROOT" \
    NORSCODE_HYBRID_OUT="$OUT" \
    NORSCODE_HYBRID_ENTRY="$ENTRY" \
    NORSCODE_HYBRID_SPECS="$SPECS" \
    "$ROOT/bin/nc" run "$ROOT/tools/bundle_with_hybrid_compiler_v9500.no"
fi

if [ -z "$NODE_BIN" ]; then
  if command -v node >/dev/null 2>&1; then
    NODE_BIN="$(command -v node)"
  elif [ -x "/Users/jansteinar/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node" ]; then
    NODE_BIN="/Users/jansteinar/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node"
  else
    echo "feil: fann ikkje node. Sett NODE_BIN eller installer node i PATH." >&2
    exit 1
  fi
fi

TMPDIR_BUNDLE="$(mktemp -d /tmp/norscode_hybrid_bundle_v9500.XXXXXX)"
cleanup() {
  rm -rf "$TMPDIR_BUNDLE"
}
trap cleanup EXIT

compiled_jsons=()
for spec in "$@"; do
  alias="${spec%%=*}"
  src="${spec#*=}"
  if [ -z "$alias" ] || [ -z "$src" ] || [ "$alias" = "$src" ]; then
    echo "ugyldig bundle-spesifikasjon: $spec" >&2
    exit 2
  fi
  out_json="$TMPDIR_BUNDLE/${alias//./_}.ncb.json"
  "$ROOT/tools/compile_with_hybrid_bundle_v9400.sh" "$ROOT" "$src" "$out_json" "$alias" >/dev/null
  compiled_jsons+=("$out_json")
done

env ROOT="$ROOT" OUT="$OUT" ENTRY="$ENTRY" TMPDIR_BUNDLE="$TMPDIR_BUNDLE" "$NODE_BIN" <<'NODE'
const fs = require('fs');
const path = require('path');

const dir = process.env.TMPDIR_BUNDLE;
const out = process.env.OUT;
const entry = process.env.ENTRY || '';
const files = fs.readdirSync(dir).filter((f) => f.endsWith('.ncb.json')).sort();
if (!files.length) {
  console.error('ingen bundle-fragment vart bygde');
  process.exit(1);
}

const merged = {
  format: 'norscode-bytecode-v1',
  entry: entry || '__main__.start',
  imports: [],
  route_handlers: [],
  dependency_providers: {},
  guard_providers: {},
  request_middlewares: [],
  response_middlewares: [],
  error_middlewares: [],
  startup_hooks: [],
  shutdown_hooks: [],
  tests: [],
  functions: {}
};

for (const file of files) {
  const j = JSON.parse(fs.readFileSync(path.join(dir, file), 'utf8'));
  for (const [name, fn] of Object.entries(j.functions || {})) {
    merged.functions[name] = fn;
  }
  for (const rh of (j.route_handlers || [])) merged.route_handlers.push(rh);
}

fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(merged));
console.log(`Bundle: ${out} (${fs.statSync(out).size} bytes)`);
NODE
