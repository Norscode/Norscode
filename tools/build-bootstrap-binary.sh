#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BIN_DIR="${ROOT_DIR}/bin"
BIN_PATH="${DIST_DIR}/norscode"
NC_PATH="${BIN_DIR}/nc"
SRC_PATH="${DIST_DIR}/norscode_launcher.c"

mkdir -p "${DIST_DIR}" "${BIN_DIR}"

cat > "${SRC_PATH}" <<'EOF'
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *message) {
    perror(message);
    exit(1);
}

static char *xstrdup(const char *value) {
    size_t len = strlen(value) + 1;
    char *copy = (char *)malloc(len);
    if (copy == NULL) {
        die("malloc");
    }
    memcpy(copy, value, len);
    return copy;
}

static char *path_dirname(const char *path) {
    char *copy = xstrdup(path);
    char *slash = strrchr(copy, '/');
    if (slash == NULL) {
        free(copy);
        return xstrdup(".");
    }
    if (slash == copy) {
        slash[1] = '\0';
        return copy;
    }
    *slash = '\0';
    return copy;
}

static char *join2(const char *left, const char *right) {
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    int need_slash = left_len > 0 && left[left_len - 1] != '/';
    size_t total = left_len + (size_t)need_slash + right_len + 1;
    char *out = (char *)malloc(total);
    if (out == NULL) {
        die("malloc");
    }
    memcpy(out, left, left_len);
    if (need_slash) {
        out[left_len] = '/';
        memcpy(out + left_len + 1, right, right_len + 1);
    } else {
        memcpy(out + left_len, right, right_len + 1);
    }
    return out;
}

static char *resolve_argv0(const char *argv0) {
    if (argv0 == NULL || argv0[0] == '\0') {
        fprintf(stderr, "norscode launcher: mangler argv[0]\n");
        exit(1);
    }
    if (argv0[0] == '/') {
        return xstrdup(argv0);
    }
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        die("getcwd");
    }
    char *resolved = join2(cwd, argv0);
    free(cwd);
    return resolved;
}

int main(int argc, char **argv) {
    char *resolved = resolve_argv0(argv[0]);
    char *dir = path_dirname(resolved);
    char *main_script = join2(dir, "../main.py");
    char **args = (char **)calloc((size_t)argc + 2, sizeof(char *));
    if (args == NULL) {
        die("calloc");
    }

    args[0] = (char *)"python3";
    args[1] = main_script;
    for (int i = 1; i < argc; i++) {
        args[i + 1] = argv[i];
    }
    args[argc + 1] = NULL;

    execvp("python3", args);
    die("execvp");
    return 1;
}
EOF

cc -O2 -Wall -Wextra -Werror -o "${BIN_PATH}" "${SRC_PATH}"
rm -f "${SRC_PATH}"

cp "${BIN_PATH}" "${NC_PATH}"
chmod +x "${BIN_PATH}" "${NC_PATH}"

printf 'Bygde bootstrap-binary: %s\n' "${BIN_PATH}"
printf 'Oppdaterte lokal CLI: %s\n' "${NC_PATH}"
