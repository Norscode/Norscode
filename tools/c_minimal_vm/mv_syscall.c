/* Linux syscall I/O — no fopen/getenv when MV_USE_SYSCALL_IO. */
#include "mv_syscall.h"
#include "mv_arena.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#if MV_USE_ARENA
#define MV_ALLOC(n) mv_arena_alloc(n)
#define MV_CALLOC(n, s) mv_arena_calloc(n, s)
#define MV_FREE(p) ((void)(p))
#else
#define MV_ALLOC(n) malloc(n)
#define MV_CALLOC(n, s) calloc(n, s)
#define MV_FREE(p) free(p)
#endif

#if MV_USE_SYSCALL_IO

#if defined(__x86_64__)

static long mv_raw_syscall6(long n, long a, long b, long c, long d, long e, long f) {
    long ret;
    register long r10 asm("r10") = d;
    register long r8 asm("r8") = e;
    register long r9 asm("r9") = f;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) : "rcx",
                     "r11", "memory");
    return ret;
}

#define MV_SYS_OPEN 2
#define MV_SYS_READ 0
#define MV_SYS_WRITE 1
#define MV_SYS_CLOSE 3

static int mv_sy_open(const char *path, int flags, int mode) {
    long fd = mv_raw_syscall6(MV_SYS_OPEN, (long)path, flags, mode, 0, 0, 0);
    return (int)fd;
}

#elif defined(__aarch64__)

static long mv_raw_syscall6(long n, long a, long b, long c, long d, long e, long f) {
    register long x8 asm("x8") = n;
    register long x0 asm("x0") = a;
    register long x1 asm("x1") = b;
    register long x2 asm("x2") = c;
    register long x3 asm("x3") = d;
    register long x4 asm("x4") = e;
    register long x5 asm("x5") = f;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory", "cc");
    return x0;
}

#define MV_SYS_OPENAT 56
#define MV_SYS_READ 63
#define MV_SYS_WRITE 64
#define MV_SYS_CLOSE 57
#define MV_AT_FDCWD (-100)

static int mv_sy_open(const char *path, int flags, int mode) {
    long fd = mv_raw_syscall6(MV_SYS_OPENAT, MV_AT_FDCWD, (long)path, flags, mode, 0, 0);
    return (int)fd;
}

#else
#error "MV_USE_SYSCALL_IO requires __x86_64__ or __aarch64__ on Linux"
#endif

static long mv_sy_read(int fd, void *buf, size_t count) {
#if defined(__x86_64__)
    return mv_raw_syscall6(MV_SYS_READ, fd, (long)buf, (long)count, 0, 0, 0);
#else
    return mv_raw_syscall6(MV_SYS_READ, fd, (long)buf, (long)count, 0, 0, 0);
#endif
}

static long mv_sy_write(int fd, const void *buf, size_t count) {
#if defined(__x86_64__)
    return mv_raw_syscall6(MV_SYS_WRITE, fd, (long)buf, (long)count, 0, 0, 0);
#else
    return mv_raw_syscall6(MV_SYS_WRITE, fd, (long)buf, (long)count, 0, 0, 0);
#endif
}

long mv_sy_write_fd(int fd, const void *buf, size_t len) {
    if (fd < 0 || !buf) return -1;
    size_t off = 0;
    while (off < len) {
        long n = mv_sy_write(fd, (const char *)buf + off, len - off);
        if (n <= 0) return -1;
        off += (size_t)n;
    }
    return (long)off;
}

static int mv_sy_close(int fd) {
#if defined(__x86_64__)
    long r = mv_raw_syscall6(MV_SYS_CLOSE, fd, 0, 0, 0, 0, 0);
#else
    long r = mv_raw_syscall6(MV_SYS_CLOSE, fd, 0, 0, 0, 0, 0);
#endif
    return (int)r;
}

int mv_sy_available(void) {
    return 1;
}

int mv_sy_read_file(const char *path, char **out, size_t *out_len) {
    if (!path || !out || !out_len) return -1;
    int fd = mv_sy_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;

    size_t cap = 4096;
    size_t len = 0;
    char *buf = (char *)MV_ALLOC(cap);
    if (!buf) {
        mv_sy_close(fd);
        return -1;
    }

    for (;;) {
        if (len + 4096 > cap) {
            size_t nc = cap * 2;
            char *nb = (char *)MV_ALLOC(nc);
            if (!nb) {
                mv_sy_close(fd);
                return -1;
            }
            if (len) memcpy(nb, buf, len);
            buf = nb;
            cap = nc;
        }
        long n = mv_sy_read(fd, buf + len, cap - len);
        if (n < 0) {
            mv_sy_close(fd);
            return -1;
        }
        if (n == 0) break;
        len += (size_t)n;
    }
    mv_sy_close(fd);
    buf[len] = 0;
    *out = buf;
    *out_len = len;
    return 0;
}

int mv_sy_write_file(const char *path, const void *data, size_t len) {
    if (!path || !data) return -1;
    int fd = mv_sy_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;

    size_t off = 0;
    while (off < len) {
        long n = mv_sy_write(fd, (const char *)data + off, len - off);
        if (n <= 0) {
            mv_sy_close(fd);
            return -1;
        }
        off += (size_t)n;
    }
    mv_sy_close(fd);
    return (int)len;
}

static int mv_sy_prefix_match(const char *entry, const char *key) {
    size_t klen = strlen(key);
    return strncmp(entry, key, klen) == 0 && entry[klen] == '=';
}

int mv_sy_environ_get(const char *key, char *buf, size_t buf_len) {
    if (!key || !buf || buf_len == 0) return -1;
    buf[0] = 0;

    char *envbuf = NULL;
    size_t envlen = 0;
    if (mv_sy_read_file("/proc/self/environ", &envbuf, &envlen) != 0) return -1;
    (void)envlen;

    const char *p = envbuf;
    const char *end = envbuf + envlen;
    while (p < end) {
        if (mv_sy_prefix_match(p, key)) {
            const char *val = p + strlen(key) + 1;
            size_t vlen = strlen(val);
            if (vlen >= buf_len) vlen = buf_len - 1;
            memcpy(buf, val, vlen);
            buf[vlen] = 0;
            free(envbuf);
            return 0;
        }
        p += strlen(p) + 1;
    }
    free(envbuf);
    return 0; /* missing key -> empty string (matches getenv behavior) */
}

#else /* !MV_USE_SYSCALL_IO */

int mv_sy_available(void) {
    return 0;
}

long mv_sy_write_fd(int fd, const void *buf, size_t len) {
    (void)fd;
    (void)buf;
    (void)len;
    return -1;
}

int mv_sy_read_file(const char *path, char **out, size_t *out_len) {
    (void)path;
    (void)out;
    (void)out_len;
    return -1;
}

int mv_sy_write_file(const char *path, const void *data, size_t len) {
    (void)path;
    (void)data;
    (void)len;
    return -1;
}

int mv_sy_environ_get(const char *key, char *buf, size_t buf_len) {
    (void)key;
    (void)buf;
    (void)buf_len;
    return -1;
}

#endif
