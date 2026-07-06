#!/usr/bin/env bash
set -euo pipefail
FROM_NO=0
if [ "${1:-}" = "--from-no" ]; then
  FROM_NO=1
  shift
fi
ROOT="${1:-$(pwd)}"
SRC="${2:-}"
OUT="${3:-}"
MODUL="${4:-__main__}"
PROJECT_ROOT="${5:-$ROOT}"
NODE_BIN="${NODE_BIN:-}"
NC_NATIVE_BIN="${NORSCODE_NATIVE_BIN:-$ROOT/dist/norscode_native}"
if [ -z "$SRC" ] || [ -z "$OUT" ]; then
  echo "bruk: compile_with_hybrid_bundle_v9400.sh <repo-root> <src.no> <out.ncb.json> [modul]" >&2
  exit 1
fi

case "$SRC" in
  "$ROOT/tools/compile_with_hybrid_bundle_v9400.no"|tools/compile_with_hybrid_bundle_v9400.no|"$ROOT/tools/bundle_with_hybrid_compiler_v9500.no"|tools/bundle_with_hybrid_compiler_v9500.no)
    FROM_NO=1
    ;;
esac

if [ "$FROM_NO" != "1" ]; then
  exec env \
    NORSCODE_ENABLE_EXEC_PROSESS=1 \
    NORSCODE_HYBRID_ROOT="$ROOT" \
    NORSCODE_HYBRID_SRC="$SRC" \
    NORSCODE_HYBRID_OUT="$OUT" \
    NORSCODE_HYBRID_MODULE="$MODUL" \
    NORSCODE_HYBRID_PROJECT_ROOT="$PROJECT_ROOT" \
    "$ROOT/bin/nc" run "$ROOT/tools/compile_with_hybrid_bundle_v9400.no"
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
cd "$PROJECT_ROOT"
NC_IMPORT_BUNDLE_DEPTH="${NC_IMPORT_BUNDLE_DEPTH:-0}"
NC_IMPORT_BUNDLE_MAX_DEPTH="${NC_IMPORT_BUNDLE_MAX_DEPTH:-4}"
mkdir -p "$(dirname "$OUT")"
env NORSCODE_CMD=compile \
  NORSCODE_FILE="$SRC" \
  NORSCODE_OUTPUT="$OUT" \
  NORSCODE_MODULE="$MODUL" \
  NORSCODE_ROOT="$ROOT" \
  NORSCODE_IMPORT_BASE="$PROJECT_ROOT" \
  "$NC_NATIVE_BIN" >/dev/null

if [ ! -f "$OUT" ]; then
  echo "feil: kompilering skreiv ikkje $OUT" >&2
  exit 1
fi

env SRC="$SRC" OUT="$OUT" NC_IMPORT_BUNDLE_DEPTH="$NC_IMPORT_BUNDLE_DEPTH" "$NODE_BIN" <<'NODE'
const fs = require('fs');
const srcPath = process.env.SRC;
const outPath = process.env.OUT;
const source = fs.existsSync(srcPath) ? fs.readFileSync(srcPath, 'utf8') : '';
const isRootCompile = (process.env.NC_IMPORT_BUNDLE_DEPTH || '0') === '0';
const sourceUsesJsonParse = source.includes('json.parse(') || source.includes('std.json.parse(');
const jsonTextLiterals = new Map();
const primitiveTextLiterals = new Map();
const primitiveParseTextLiterals = new Map();
const primitiveAssertTextLiterals = new Map();
const primitiveIndexTextLiterals = new Map();
const primitiveMapValueTextLiterals = new Map();
const rawJsonishTextLiterals = new Map();
const invalidJsonishTextLiterals = [];
const stringRe = /"((?:\\.|[^"\\])*)"/g;
function rememberPrimitive(map, raw) {
  const trimmed = raw.trim();
  if (/^-?[0-9]+$/.test(trimmed) || trimmed === 'true' || trimmed === 'false' || trimmed === 'null') {
    map.set(trimmed, raw);
  }
}
function collectContext(re, map) {
  let m;
  while ((m = re.exec(source))) {
    try {
      rememberPrimitive(map, JSON.parse(`"${m[1]}"`));
    } catch {}
  }
}
if (isRootCompile) {
  collectContext(/json\.parse\s*\(\s*"((?:\\.|[^"\\])*)"\s*\)/g, primitiveParseTextLiterals);
  collectContext(/assert_eq\s*\([\s\S]*?,\s*"((?:\\.|[^"\\])*)"\s*\)/g, primitiveAssertTextLiterals);
  collectContext(/\[\s*"((?:\\.|[^"\\])*)"\s*\]/g, primitiveIndexTextLiterals);
  collectContext(/:\s*"((?:\\.|[^"\\])*)"/g, primitiveMapValueTextLiterals);
}
let match;
while ((match = stringRe.exec(source))) {
  try {
    const raw = JSON.parse(`"${match[1]}"`);
    const trimmed = raw.trim();
    if ((trimmed.startsWith('[') && trimmed.endsWith(']')) || (trimmed.startsWith('{') && trimmed.endsWith('}'))) {
      const existing = rawJsonishTextLiterals.get(trimmed) || { raw, count: 0 };
      existing.count += 1;
      rawJsonishTextLiterals.set(trimmed, existing);
      try {
        jsonTextLiterals.set(JSON.stringify(JSON.parse(trimmed)), raw);
      } catch {
        invalidJsonishTextLiterals.push(raw);
      }
    } else if (/^\+\d+$/.test(trimmed) || /^-0\d+$/.test(trimmed) || /^0\d+$/.test(trimmed)) {
      const existing = rawJsonishTextLiterals.get(trimmed) || { raw, count: 0 };
      existing.count += 1;
      rawJsonishTextLiterals.set(trimmed, existing);
    } else if (isRootCompile && (/^-?[0-9]+$/.test(trimmed) || trimmed === 'true' || trimmed === 'false' || trimmed === 'null')) {
      const existing = primitiveTextLiterals.get(trimmed) || { raw, count: 0 };
      existing.count += 1;
      primitiveTextLiterals.set(trimmed, existing);
    }
  } catch {}
}
let rawNcb = fs.readFileSync(outPath, 'utf8');
for (const [literal, info] of rawJsonishTextLiterals) {
  const escaped = JSON.stringify(info.raw);
  const needle = `["PUSH_CONST",${literal}]`;
  const replacement = `["PUSH_CONST",${escaped}]`;
  let remaining = info.count;
  while (remaining > 0 && rawNcb.includes(needle)) {
    rawNcb = rawNcb.replace(needle, replacement);
    remaining -= 1;
  }
}
fs.writeFileSync(outPath, rawNcb);
{
  const ncb = JSON.parse(fs.readFileSync(outPath, 'utf8'));
  const invalidJsonishQueue = invalidJsonishTextLiterals.slice();
  for (const fn of Object.values(ncb.functions || {})) {
    for (const ins of (fn.code || [])) {
      if (ins[0] === 'LOAD_NAME' && ins[1] === null) {
        ins[0] = 'PUSH_CONST';
      }
      if (ins[0] === 'PUSH_CONST' && (Array.isArray(ins[1]) || (ins[1] && typeof ins[1] === 'object'))) {
        const key = JSON.stringify(ins[1]);
        if (jsonTextLiterals.has(key)) {
          ins[1] = jsonTextLiterals.get(key);
        } else if (invalidJsonishQueue.length > 0) {
          ins[1] = invalidJsonishQueue.shift();
        }
      }
    }
    if (isRootCompile && (fn.module || '').startsWith('__main__')) {
      const code = fn.code || [];
      for (let i = 0; i < code.length; i += 1) {
        const ins = code[i];
        if (!Array.isArray(ins) || ins[0] !== 'PUSH_CONST') continue;
        const value = ins[1];
        if (!(value === null || typeof value === 'boolean' || typeof value === 'number')) continue;
        const next = code[i + 1] || [];
        const prev = code[i - 1] || [];
        const literal = String(value);
        const textAssertContext = prev[0] === 'INDEX_GET' || (
          prev[0] === 'CALL' && (
            String(prev[1]).endsWith('.query_text') ||
            String(prev[1]).endsWith('.hent_tekst') ||
            String(prev[1]).endsWith('.parse') ||
            String(prev[1]).endsWith('.stringify')
          )
        );
        if (sourceUsesJsonParse && next[0] === 'CALL' && next[1] === 'std.json.parse' && primitiveParseTextLiterals.has(literal)) {
          ins[1] = primitiveParseTextLiterals.get(literal);
        } else if (next[0] === 'CALL' && next[1] === 'builtin.assert_eq' && primitiveAssertTextLiterals.has(literal) && textAssertContext) {
          ins[1] = primitiveAssertTextLiterals.get(literal);
        } else if ((next[0] === 'INDEX_GET' || next[0] === 'INDEX_SET') && primitiveIndexTextLiterals.has(literal)) {
          ins[1] = primitiveIndexTextLiterals.get(literal);
        } else if (next[0] === 'CALL' && next[1] === 'builtin.assert_eq' && primitiveTextLiterals.has(literal) && textAssertContext) {
          ins[1] = primitiveTextLiterals.get(literal).raw;
        } else if (sourceUsesJsonParse && (next[0] === 'COMPARE_EQ' || next[0] === 'COMPARE_NE') && primitiveTextLiterals.has(literal)) {
          ins[1] = primitiveTextLiterals.get(literal).raw;
        }
      }
      for (let i = 0; i < code.length; i += 1) {
        const ins = code[i];
        if (!Array.isArray(ins) || ins[0] !== 'BUILD_MAP') continue;
        const count = Number(ins[1]) || 0;
        const start = i - (count * 2);
        if (start < 0) continue;
        for (let j = start + 1; j < i; j += 2) {
          const valueIns = code[j];
          if (!Array.isArray(valueIns) || valueIns[0] !== 'PUSH_CONST') continue;
          const value = valueIns[1];
          if (!(value === null || typeof value === 'boolean' || typeof value === 'number')) continue;
          const literal = String(value);
          if (primitiveMapValueTextLiterals.has(literal)) {
            valueIns[1] = primitiveMapValueTextLiterals.get(literal);
          }
        }
      }
    }
  }
  fs.writeFileSync(outPath, JSON.stringify(ncb));
}
NODE

if [ "$NC_IMPORT_BUNDLE_DEPTH" -lt "$NC_IMPORT_BUNDLE_MAX_DEPTH" ] && [ -f "$OUT" ]; then
  mkdir -p "$ROOT/build/nc-import-tmp"
  TMP_IMPORT_DIR="$(mktemp -d "$ROOT/build/nc-import-tmp/norscode_import_bundle.XXXXXX")"
  cleanup_imports() {
    rm -rf "$TMP_IMPORT_DIR"
  }
  trap cleanup_imports EXIT

  import_specs="$(
    sed -n \
      -e 's/^[[:space:]]*bruk[[:space:]][[:space:]]*\([^[:space:]]*\).*/\1/p' \
      -e 's/^[[:space:]]*import[[:space:]][[:space:]]*\([^[:space:]]*\).*/\1/p' \
      "$SRC" | sort -u
  )"

  import_jsons=()
  if [ -n "$import_specs" ]; then
    while IFS= read -r import_mod; do
      [ -n "$import_mod" ] || continue
      import_path="${import_mod//.//}.no"
      import_abs="$PROJECT_ROOT/$import_path"
      if [ ! -f "$import_abs" ]; then
        import_abs="$ROOT/$import_path"
      fi
      if [ ! -f "$import_abs" ]; then
        import_path="${import_mod//.//}.nors"
        import_abs="$PROJECT_ROOT/$import_path"
      fi
      if [ ! -f "$import_abs" ]; then
        import_abs="$ROOT/$import_path"
      fi
      if [ ! -f "$import_abs" ]; then
        package_dir="$PROJECT_ROOT/packages/${import_mod//.//}"
        package_manifest="$package_dir/norsklang.toml"
        if [ -f "$package_manifest" ]; then
          package_entry="$(
            sed -n "s/^[[:space:]]*entry[[:space:]]*=[[:space:]]*[\"']\\([^\"']*\\)[\"'].*/\\1/p" "$package_manifest" | head -n 1
          )"
          if [ -n "$package_entry" ] && [ -f "$package_dir/$package_entry" ]; then
            import_abs="$package_dir/$package_entry"
          fi
        fi
      fi
      if [ ! -f "$import_abs" ]; then
        package_dir="$ROOT/packages/${import_mod//.//}"
        package_manifest="$package_dir/norsklang.toml"
        if [ -f "$package_manifest" ]; then
          package_entry="$(
            sed -n "s/^[[:space:]]*entry[[:space:]]*=[[:space:]]*[\"']\\([^\"']*\\)[\"'].*/\\1/p" "$package_manifest" | head -n 1
          )"
          if [ -n "$package_entry" ] && [ -f "$package_dir/$package_entry" ]; then
            import_abs="$package_dir/$package_entry"
          fi
        fi
      fi
      [ -f "$import_abs" ] || continue
      import_out="$TMP_IMPORT_DIR/${import_mod//./_}.ncb.json"
      recursive_args=()
      if [ "$FROM_NO" = "1" ]; then
        recursive_args+=(--from-no)
      fi
      NC_IMPORT_BUNDLE_DEPTH=$((NC_IMPORT_BUNDLE_DEPTH + 1)) NC_IMPORT_BUNDLE_MAX_DEPTH="$NC_IMPORT_BUNDLE_MAX_DEPTH" "$0" "${recursive_args[@]}" "$ROOT" "$import_abs" "$import_out" "$import_mod" "$PROJECT_ROOT" >/dev/null
      if [ -f "$import_out" ]; then
        import_jsons+=("$import_out")
      fi
    done <<EOF_IMPORTS
$import_specs
EOF_IMPORTS
  fi

  if [ "${#import_jsons[@]}" -gt 0 ]; then
    env OUT="$OUT" "$NODE_BIN" - "${import_jsons[@]}" <<'NODE'
const fs = require('fs');
const out = process.env.OUT;
const merged = JSON.parse(fs.readFileSync(out, 'utf8'));
for (const file of process.argv.slice(2)) {
  const j = JSON.parse(fs.readFileSync(file, 'utf8'));
  for (const [name, fn] of Object.entries(j.functions || {})) {
    merged.functions[name] = fn;
  }
  for (const rh of (j.route_handlers || [])) {
    (merged.route_handlers || (merged.route_handlers = [])).push(rh);
  }
}
fs.writeFileSync(out, JSON.stringify(merged));
NODE
  fi
fi

env OUT="$OUT" MODUL="$MODUL" "$NODE_BIN" <<'NODE'
const fs = require('fs');
const out = process.env.OUT;
const modul = process.env.MODUL || '__main__';
const j = JSON.parse(fs.readFileSync(out, 'utf8'));
const fns = j.functions || {};
if (fns['std.json.parse']) {
  if (!fns['std.json.parse_contract']) {
    const clone = JSON.parse(JSON.stringify(fns['std.json.parse']));
    clone.name = 'parse_contract';
    fns['std.json.parse_contract'] = clone;
  }
  for (const fn of Object.values(fns)) {
    for (const ins of (fn.code || [])) {
      if (ins[0] === 'CALL' && ins[1] === 'std.json.parse') ins[1] = 'std.json.parse_contract';
    }
  }
}
const wanted = `${modul}.start`;
if (!fns[wanted]) {
  const candidates = [`${modul}.test`, `${modul}.main`];
  const picked = candidates.find((name) => fns[name]) || Object.keys(fns).find((name) => name.startsWith(`${modul}.`));
  if (picked) j.entry = picked;
}
fs.writeFileSync(out, JSON.stringify(j));
NODE
