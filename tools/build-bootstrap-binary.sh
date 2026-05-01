#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
BIN_PATH="${DIST_DIR}/norscode"
SRC_PATH="${DIST_DIR}/norscode_launcher.c"

mkdir -p "${DIST_DIR}"

cat > "${SRC_PATH}" <<'EOF'
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *message) {
    perror(message);
    exit(1);
}

int main(int argc, char **argv) {
    char resolved[PATH_MAX];
    char launcher_path[PATH_MAX];
    char main_script[PATH_MAX];
    char *dircopy;
    char *dir;
    char *args[argc + 2];

    if (argv[0] == NULL) {
        fprintf(stderr, "norscode launcher: mangler argv[0]\n");
        return 1;
    }

    if (argv[0][0] == '/') {
        snprintf(resolved, sizeof(resolved), "%s", argv[0]);
    } else {
        if (getcwd(resolved, sizeof(resolved)) == NULL) {
            die("getcwd");
        }
        size_t len = strlen(resolved);
        if (len + 1 + strlen(argv[0]) + 1 >= sizeof(resolved)) {
            fprintf(stderr, "norscode launcher: sti er for lang\n");
            return 1;
        }
        resolved[len] = '/';
        resolved[len + 1] = '\0';
        strncat(resolved, argv[0], sizeof(resolved) - strlen(resolved) - 1);
    }

    snprintf(launcher_path, sizeof(launcher_path), "%s", resolved);
    dircopy = strdup(launcher_path);
    if (dircopy == NULL) {
        die("strdup");
    }
    dir = dirname(dircopy);
    if (snprintf(main_script, sizeof(main_script), "%s/../main.py", dir) >= (int)sizeof(main_script)) {
        fprintf(stderr, "norscode launcher: main.py-sti er for lang\n");
        free(dircopy);
        return 1;
    }
    free(dircopy);

    args[0] = "python3";
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
printf 'Bygde bootstrap-binary: %s\n' "${BIN_PATH}"
