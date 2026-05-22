#ifndef MV_ENV_H
#define MV_ENV_H

#include <stddef.h>

/* Read env var into buf (NUL-terminated). Returns 0 on success. */
int mv_env_get(const char *key, char *buf, size_t buf_len);

/* True when value is 1/true/yes (case-insensitive prefix). */
int mv_env_flag(const char *key);

/* Read NORCODE_ARG{i} (i >= 0). Returns 0 on success. */
int mv_env_arg(int index, char *buf, size_t buf_len);

/* Read NORCODE_ARGC as integer; returns -1 when unset/invalid. */
int mv_env_argc(void);

#endif
