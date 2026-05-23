#include "mv_io.h"
#include "mv_syscall.h"

#include <string.h>

#if !defined(NORCODE_FREESTANDING)
#include <stdio.h>
#endif

void mv_write_stderr(const char *msg) {
    if (!msg) return;
    size_t n = strlen(msg);
#if MV_USE_SYSCALL_IO
    if (mv_sy_available() && mv_sy_write_fd(2, msg, n) >= 0) return;
#endif
#if !defined(NORCODE_FREESTANDING)
    fwrite(msg, 1, n, stderr);
#endif
}
