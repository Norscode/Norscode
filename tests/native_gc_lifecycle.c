#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

int main(void) {
    NcVal **stack = calloc(512, sizeof(NcVal *));
    NcVal **vars = calloc(128, sizeof(NcVal *));
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);

    NcVal *root = nc_list_new();
    nc_builtin_legg_til(root, nc_str("root-child"));
    vars[nvars++] = root;
    for (int i = 0; i < 200000; i++) {
        NcVal *temporary = nc_str("temporary-runtime-value");
        (void)temporary;
        if ((i & 255) == 0) nc_gc_safepoint();
    }
    nc_gc_collect();
    root = vars[0];
    nc_gc_collect();
    root = vars[0];
    if (g_gc_allocated != 2) {
        fprintf(stderr, "expected root graph with two values, got %zu\n", g_gc_allocated);
        return 1;
    }
    if (g_gc_arena_page_count != 1 || g_gc_arena_reused < 200000) return 5;
    if (root->gc_magic != NC_GC_MAGIC || root->gc_flags != NC_GC_ACTIVE ||
        root->gc_generation != 1 || root->gc_age < 2 || root->gc_ref_count != 1 ||
        root->gc_first_edge == 0 || g_gc_edge_count != 1 ||
        offsetof(NcVal, gc_next) != 32 || offsetof(NcGcEdge, from) != 32) {
        fprintf(stderr, "NCG1 header contract mismatch\n");
        return 3;
    }

    nvars = 0;
    nc_gc_collect();
    if (g_gc_allocated != 0) {
        fprintf(stderr, "expected empty heap, got %zu\n", g_gc_allocated);
        return 4;
    }
    if (g_gc_arena_page_count != 0) return 6;
    if (g_gc_edge_count != 0) return 7;
    nc_gc_frame_leave(&frame);
    free(vars);
    free(stack);
    puts("native GC lifecycle: ok");
    return 0;
}
