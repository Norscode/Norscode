#ifndef MV_INTERNAL_H
#define MV_INTERNAL_H

#include "minimal_vm.h"

#define MV_ERR_STACK (-100)
#define MV_ERR_LOCALS (-101)
#define MV_ERR_DECODE (-102)
#define MV_ERR_LABEL (-103)
#define MV_ERR_STEPS (-104)
#define MV_ERR_DEPTH (-105)
#define MV_ERR_TYPE (-106)
#define MV_ERR_BUILTIN (-107)
#define MV_ERR_FN (-108)

void *mv_track(mv_runtime_t *rt, void *ptr);
mv_list_t *mv_list_new(mv_runtime_t *rt, size_t n);
int mv_list_append(mv_list_t *l, const mv_value_t *v);
char *mv_strdup(mv_runtime_t *rt, const char *s);

#endif
