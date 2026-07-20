#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

int main(void) {
    NcVal *stack[8] = {0}, *vars[8] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);
    NcVal *old = nc_map_new();
    nc_index_set(old, nc_str("name"), nc_str("old-generation"));
    vars[nvars++] = old;
    nc_gc_collect();
    old = vars[0];
    nc_gc_collect();
    old = vars[0];
    if (old->gc_generation != 1 || g_gc_major_collections != 2) return 1;

    nvars = 0;
    for (int i = 0; i < 5000; i++) (void)nc_str("young-garbage");
    nc_gc_safepoint();
    old = vars[0];
    if (g_gc_minor_collections != 1 || g_gc_allocated != 2) return 2;
    NcVal *name = nc_index_get(old, nc_str("name"));
    if (!name || name->type != NC_STR || strcmp(name->s, "old-generation")) return 3;

    nc_gc_collect();
    if (g_gc_allocated != 0 || g_gc_major_collections != 3) return 4;

    NcVal *a = nc_map_new(), *b = nc_map_new();
    nc_index_set(a, nc_str("peer"), b);
    nc_index_set(b, nc_str("peer"), a);
    vars[nvars++] = a;
    nc_gc_collect();
    if (g_gc_allocated != 2 || g_gc_edge_count != 2) return 5;
    nvars = 0;
    nc_gc_collect();
    if (g_gc_allocated != 0 || g_gc_edge_count != 0 || g_gc_arena_page_count != 0) return 6;
    nc_gc_frame_leave(&frame);
    puts("native generational GC: ok");
    return 0;
}
