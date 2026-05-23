#include "mv_env.h"

#include "mv_syscall.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mv_env_get(const char *key, char *buf, size_t buf_len) {
    if (!key || !buf || buf_len == 0) return -1;
    if (mv_sy_available()) {
        return mv_sy_environ_get(key, buf, buf_len);
    }
    const char *v = getenv(key);
    if (!v || !v[0]) return -1;
    snprintf(buf, buf_len, "%s", v);
    return 0;
}

int mv_env_flag(const char *key) {
    char buf[16];
    if (mv_env_get(key, buf, sizeof(buf)) != 0) return 0;
    if (buf[0] == '1') return 1;
    if (strncmp(buf, "true", 4) == 0 || strncmp(buf, "yes", 3) == 0) return 1;
    return 0;
}

int mv_env_arg(int index, char *buf, size_t buf_len) {
    if (index < 0 || !buf || buf_len == 0) return -1;
    char key[24];
    snprintf(key, sizeof(key), "NORCODE_ARG%d", index);
    return mv_env_get(key, buf, buf_len);
}

int mv_env_argc(void) {
    char buf[16];
    if (mv_env_get("NORCODE_ARGC", buf, sizeof(buf)) != 0) return -1;
    int n = 0;
    for (const char *p = buf; *p; p++) {
        if (*p < '0' || *p > '9') return -1;
        n = n * 10 + (*p - '0');
    }
    return n;
}
