#!/usr/bin/env bash
# tools/build_stage0_release_assets.sh — Omgang 6b.4: pakk stage-0 ELF for GitHub Release
#
# Produserer compiler_stage0-<plattform>.elf i gjeldande katalog.
# Krev: dist/norscode_native, Linux x86-64 for full pipeline.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

platform_name() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Darwin)
            case "$ARCH" in arm64) printf 'macos-arm64' ;; x86_64) printf 'macos-x86_64' ;; *) return 1 ;; esac ;;
        Linux)
            case "$ARCH" in x86_64|amd64) printf 'linux-x86_64' ;; aarch64|arm64) printf 'linux-arm64' ;; *) return 1 ;; esac ;;
        *) return 1 ;;
    esac
}

platform="$(platform_name)" || {
    printf 'Feil: ukjent plattform\n' >&2
    exit 1
}

OUT_NAME="compiler_stage0-${platform}.elf"
NCB="$ROOT/build/6b/compiler_stage0.ncb.json"
ELF="$ROOT/build/6b/compiler_stage0.elf"

if [ ! -x "$ROOT/dist/norscode_native" ]; then
    bash "$ROOT/tools/build_norscode_native.sh"
fi

mkdir -p "$ROOT/build/6b"
bash "$ROOT/tools/build_omgang6b_compiler_ncb.sh" "$NCB"
bash "$ROOT/tools/ncb_to_elf.sh" "$NCB" "$ELF"

cp "$ELF" "$ROOT/${OUT_NAME}"
chmod +x "$ROOT/${OUT_NAME}"
sha256sum "$ROOT/${OUT_NAME}" > "$ROOT/${OUT_NAME}.sha256" 2>/dev/null \
    || shasum -a 256 "$ROOT/${OUT_NAME}" > "$ROOT/${OUT_NAME}.sha256"

printf '✓ %s (%d bytes)\n' "$OUT_NAME" "$(wc -c < "$ROOT/${OUT_NAME}" | tr -d ' ')"
