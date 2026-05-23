#include "mv_bootstrap_host_exec.h"

#include "mv_io.h"
#include "mv_syscall.h"

#include <stdio.h>
#include <string.h>

#if defined(MV_BOOTSTRAP_HOST_EXEC)
#include <sys/wait.h>
#include <unistd.h>
#endif

#if MV_USE_SYSCALL_IO
#if defined(__x86_64__)
#define MV_SYS_FORK 57
#define MV_SYS_EXECVE 59
#define MV_SYS_WAIT4 61
#define MV_SYS_EXIT 60
#elif defined(__aarch64__)
#define MV_SYS_CLONE 220
#define MV_SYS_EXECVE 221
#define MV_SYS_WAIT4 260
#define MV_SYS_EXIT 93
#define MV_CLONE_VFORK 0x00004000
#define MV_SIGCHLD 0x00000011
#endif
#if defined(__x86_64__)
static long mv_raw6(long n, long a, long b, long c, long d, long e, long f) {
    long ret;
    register long r10 asm("r10") = d;
    register long r8 asm("r8") = e;
    register long r9 asm("r9") = f;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) : "rcx",
                     "r11", "memory");
    return ret;
}
#elif defined(__aarch64__)
static long mv_raw6(long n, long a, long b, long c, long d, long e, long f) {
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
#endif

static int mv_sy_exec_path_wait(const char *path, int *exit_code) {
    if (!path || !path[0] || !exit_code) return -1;
    char *argv0 = (char *)path;
    char *argv[] = {argv0, NULL};
    char *envp[] = {NULL};

#if defined(__x86_64__)
    long pid = mv_raw6(MV_SYS_FORK, 0, 0, 0, 0, 0, 0);
    if (pid < 0) return -1;
    if (pid == 0) {
        mv_raw6(MV_SYS_EXECVE, (long)path, (long)argv, (long)envp, 0, 0, 0);
        mv_raw6(MV_SYS_EXIT, 127, 0, 0, 0, 0, 0);
        return -1;
    }
    int status = 0;
    long wr = mv_raw6(MV_SYS_WAIT4, pid, (long)&status, 0, 0, 0, 0);
    if (wr < 0) return -1;
    *exit_code = (status >> 8) & 0xFF;
    return 0;
#elif defined(__aarch64__)
    long pid = mv_raw6(MV_SYS_CLONE, MV_SIGCHLD, 0, 0, 0, 0, 0);
    if (pid < 0) return -1;
    if (pid == 0) {
        mv_raw6(MV_SYS_EXECVE, (long)path, (long)argv, (long)envp, 0, 0, 0);
        mv_raw6(MV_SYS_EXIT, 127, 0, 0, 0, 0, 0);
        return -1;
    }
    int status = 0;
    long wr = mv_raw6(MV_SYS_WAIT4, pid, (long)&status, 0, 0, 0, 0);
    if (wr < 0) return -1;
    *exit_code = (status >> 8) & 0xFF;
    return 0;
#else
    (void)argv;
    (void)envp;
    return -1;
#endif
}
#endif /* MV_USE_SYSCALL_IO */

int mv_bootstrap_host_exec_path(const char *path, int *exit_code) {
    if (!path || !path[0] || !exit_code) return -1;

#if defined(MV_BOOTSTRAP_HOST_EXEC)
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        char *argv[] = {(char *)path, NULL};
        execv(path, argv);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
        return 0;
    }
    return -1;
#elif MV_USE_SYSCALL_IO
    return mv_sy_exec_path_wait(path, exit_code);
#else
    (void)path;
    *exit_code = 0;
    mv_write_stderr("bootstrap: execute_process unavailable (no host exec)\n");
    return -1;
#endif
}
