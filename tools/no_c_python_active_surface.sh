#!/usr/bin/env bash
# tools/no_c_python_active_surface.sh
# Gate for aktiv repository-surface utan C/Python.
#
# Current policy:
# - No .py files anywhere in repository outside archive/
# - No .c/.h files anywhere in repository outside archive/
# - No references to Python/C bootstrap i normal flyt utanfor gjeldande allowlist

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

ACTIVE_DIRS=(
  "$ROOT/.github"
  "$ROOT/tools"
  "$ROOT/selfhost"
  "$ROOT/bin"
  "$ROOT/bootstrap"
)

REF_CHECK_FILES=(
  "$ROOT/bin/nc"
)

REF_CHECK_GLOBS=(
  "$ROOT/.github/workflows/*.yml"
  "$ROOT/.github/workflows/*.yaml"
  "$ROOT/tools/*.sh"
)

NCB_TO_C_ALLOWLIST=(
  "$ROOT/tools/no_c_python_active_surface.sh"
  "$ROOT/docs/05-development/NATIVE_CODEGEN_V2_ABI.md"
  "$ROOT/docs/SELFHOST_HANDLINGSPLAN.md"
  "$ROOT/docs/STATUS.md"
)

ALLOWLIST_FILES=(
)

ALLOWLIST_PATTERNS=(
  "$ROOT/archive/"
)

is_allowlisted_path() {
    local path="$1"
    local item
    if [ "${#ALLOWLIST_FILES[@]}" -gt 0 ]; then
        for item in "${ALLOWLIST_FILES[@]}"; do
            if [ "$path" = "$item" ]; then
                return 0
            fi
        done
    fi
    for item in "${ALLOWLIST_PATTERNS[@]}"; do
        case "$path" in
            "$item"*) return 0 ;;
        esac
    done
    return 1
}

is_ncb_to_c_allowlisted_path() {
    local path="$1"
    # Docs-mappa er alltid tillatt — ncb_to_c dukkar berre opp som historisk kontekst der
    case "$path" in
        "$ROOT/docs/"*) return 0 ;;
    esac
    local item
    for item in "${NCB_TO_C_ALLOWLIST[@]}"; do
        if [ "$path" = "$item" ]; then
            return 0
        fi
    done
    return 1
}

collect_repo_c_hits() {
    {
        if command -v git >/dev/null 2>&1 && git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            git -C "$ROOT" ls-files '*.c' '*.h' 2>/dev/null | while IFS= read -r rel; do
                [ -f "$ROOT/$rel" ] && printf '%s/%s\n' "$ROOT" "$rel"
            done || true
        fi
        find "$ROOT" \
            -path "$ROOT/.git" -prune -o \
            -path "$ROOT/build" -prune -o \
            -type f \( -name '*.c' -o -name '*.h' \) -print 2>/dev/null
    } | sort -u
}

collect_repo_py_hits() {
    find "$ROOT" -path "$ROOT/.git" -prune -o -type f -name '*.py' -print 2>/dev/null | sort -u
}

collect_ref_files() {
    local path
    for path in "${REF_CHECK_FILES[@]}"; do
        [ -f "$path" ] && printf '%s\n' "$path"
    done

    local glob
    for glob in "${REF_CHECK_GLOBS[@]}"; do
        for path in $glob; do
            [ -f "$path" ] && printf '%s\n' "$path"
        done
    done | sort -u

    # Also include recursive shell scripts under tools (e.g. maint scripts)
    if [ -d "$ROOT/tools" ]; then
        find "$ROOT/tools" -type f -name '*.sh' 2>/dev/null | sort -u
    fi
}

print_policy() {
    printf '\nPolicy (no-c/python active surface):\n'
    printf '  - Aktiv flyt: sjølvstendig .no/NCB/Native-ELF, utan Python/C i kjede.\n'
    printf '  - C/Python-kode er berre historisk arkiv under archive/.\n'
    printf '  - Nye funksjonar skal byggjast som .no og verifiserast med ./bin/nc feature-check.\n\n'
}

fail=0

printf '=== Active surface C/Python gate ===\n\n'
print_policy

tmp_py_hits="$(mktemp)"
trap 'rm -f "$tmp_py_hits"' EXIT
collect_repo_py_hits > "$tmp_py_hits"

non_allowlisted_py=""
while IFS= read -r path; do
    [ -n "$path" ] || continue
    if ! is_allowlisted_path "$path"; then
        non_allowlisted_py="${non_allowlisted_py}${path}"$'\n'
    fi
done < "$tmp_py_hits"

if [ -n "$non_allowlisted_py" ]; then
    printf 'Feil: .py-filer funnet utanfor archive/:\n%s\n\n' "$non_allowlisted_py" >&2
    fail=1
else
    printf 'OK: ingen .py-filer utanfor archive/\n'
fi

tmp_c_hits="$(mktemp)"
trap 'rm -f "$tmp_py_hits" "$tmp_c_hits"' EXIT
collect_repo_c_hits > "$tmp_c_hits"

non_allowlisted_c=""
while IFS= read -r path; do
    [ -n "$path" ] || continue
    if ! is_allowlisted_path "$path"; then
        non_allowlisted_c="${non_allowlisted_c}${path}"$'\n'
    fi
done < "$tmp_c_hits"

if [ -n "$non_allowlisted_c" ]; then
    printf 'Feil: nye .c/.h-filer utenfor allowlist:\n%s\n' "$non_allowlisted_c" >&2
    fail=1
else
    printf 'OK: ingen nye .c/.h-filer utenfor allowlist\n'
fi

if command -v rg >/dev/null 2>&1; then
    ref_files="$(collect_ref_files)"
    if [ -n "$ref_files" ]; then
        filtered_ref_files=""
        while IFS= read -r path; do
            [ -n "$path" ] || continue
            filtered_ref_files="${filtered_ref_files}"$'\n'"${path}"
        done <<< "$ref_files"

        ref_hits="$(printf '%s\n' "$filtered_ref_files" | rg -v '/(reports|archive)/|/no_c_python_active_surface\.sh$|/norscode_feature_gate\.sh$|/python_dependency_audit\.sh$|/python_free_ci\.sh$|/build_norscode_native\.sh$|/verify_seed_only\.sh$|/verify_selvstendighet\.sh$|/verify_omgang6\.sh$|/selfhost_phase0_verify\.sh$|/selfhost_drift_guard\.sh$|/unified_bootstrap_verifier\.sh$|/no_legacy_cvm\.sh$|/enforce_native_first\.sh$|/ci\.yml$|/publish\.yml$|/selfhost-lexer-token-smoke\.yml$|/selvstendighet\.yml$|/stage0-binary\.yml$|/bin/nc$|/windows-app-release\.yml$|/linux-app-release\.yml$|/macos-app-release\.yml$|/docs/' \
          | xargs rg -n -e 'python3?' -e 'pytest' -e 'clang' -e 'gcc' -e '\bcc\b' -e 'ncb_to_c' -e 'c_minimal_vm' -e 'nc_runtime' -e 'nc_native_main' 2>/dev/null || true)"
    else
        ref_hits=""
    fi
else
    ref_hits=""
fi

if command -v rg >/dev/null 2>&1; then
ncb_to_c_hits="$(rg -n 'ncb_to_c' .github tools selfhost docs bootstrap bin 2>/dev/null || true)"
else
    ncb_to_c_hits=""
fi

if [ -n "$ncb_to_c_hits" ]; then
    unexpected_ncb=""
    while IFS= read -r hit; do
        [ -n "$hit" ] || continue
        path="${hit%%:*}"
        # Make sure we compare absolute paths for allowlist consistency.
        if [ "${path#/}" = "$path" ]; then
            path="$ROOT/$path"
        fi
        if is_ncb_to_c_allowlisted_path "$path"; then
            continue
        fi
        unexpected_ncb="${unexpected_ncb}${hit}"$'\n'
    done <<< "$ncb_to_c_hits"

    if [ -n "$unexpected_ncb" ]; then
        printf 'Feil: ncb_to_c funnet i normal kontekst:\n%s\n' "$unexpected_ncb" >&2
        fail=1
    else
        printf 'OK: ncb_to_c er avgrensa til vedlikehald/docs-lane\n'
    fi
else
    printf 'OK: ncb_to_c-referansar er ikkje funne\n'
fi

if [ -n "$ref_hits" ]; then
    printf 'Feil: aktive referanser til C/Python-overgangsflater utenfor allowlist:\n%s\n' "$ref_hits" >&2
    fail=1
else
    printf 'OK: ingen uventede overgangsreferanser utenfor allowlist\n'
fi

if [ "$fail" -ne 0 ]; then
    exit 1
fi

printf '\n=== Gate passerte ===\n'
