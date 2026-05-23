/* Linux raw syscall I/O for minimal VM builtins (A3 / libc-free path). */
#ifndef MV_SYSCALL_H
#define MV_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

#if defined(__linux__) && !defined(NORCODE_MV_NO_SYSCALL)
#define MV_USE_SYSCALL_IO 1
#else
#define MV_USE_SYSCALL_IO 0
#endif

/* Returns 1 when raw syscalls are compiled in for this target. */
int mv_sy_available(void);

/* Read entire file (NUL-terminated). Caller frees *out with free(). */
int mv_sy_read_file(const char *path, char **out, size_t *out_len);

/* Write raw bytes; creates/truncates. Returns bytes written or -1. */
int mv_sy_write_file(const char *path, const void *data, size_t len);

/* Lookup KEY in process environ via /proc/self/environ (Linux). */
int mv_sy_environ_get(const char *key, char *buf, size_t buf_len);

/* Raw write to an open fd (e.g. 2 = stderr). Returns bytes written or -1. */
long mv_sy_write_fd(int fd, const void *buf, size_t len);

#endif
