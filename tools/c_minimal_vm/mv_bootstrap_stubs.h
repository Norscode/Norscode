/* Optional C stubs for bootstrap NCBB external CALL targets (probe / dual-bundle). */
#ifndef MV_BOOTSTRAP_STUBS_H
#define MV_BOOTSTRAP_STUBS_H

#include "minimal_vm.h"

/* Returns 0 if handled, else MV_ERR_FN. */
int mv_bootstrap_stub_call(
    mv_runtime_t *rt,
    const char *name,
    const mv_value_t *args,
    uint8_t argc,
    mv_value_t *out);

#endif
