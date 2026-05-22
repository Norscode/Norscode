#ifndef MV_BUILTINS_H
#define MV_BUILTINS_H

#include "minimal_vm.h"

int mv_call_builtin(mv_runtime_t *rt, const char *bname, mv_value_t *args, size_t argc, mv_value_t *out);

#endif
