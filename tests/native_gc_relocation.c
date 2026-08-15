#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

int main(void) {
    setenv("NORSCODE_GC_FORCE_COMPACT", "1", 1);
    NcVal *stack[16] = {0}, *vars[16] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);

    NcVal *root = nc_map_new();
    NcVal *children = nc_list_new();
    nc_builtin_legg_til(children, nc_str("relocated-child"));
    nc_index_set(root, nc_str("children"), children);
    vars[nvars++] = root;
    NcVal *old_root = root;
    NcVal *old_children = children;

    for (int i = 0; i < 5000; i++) (void)nc_str("fragmentation");
    nc_gc_collect();
    unsetenv("NORSCODE_GC_FORCE_COMPACT");

    root = vars[0];
    if (root == old_root || g_gc_relocated != 3 || g_gc_arena_page_count != 1) return 1;
    NcVal *key = nc_str("children");
    children = nc_index_get(root, key);
    if (!children || children == old_children || children->type != NC_LIST ||
        children->list->len != 1) return 2;
    NcVal *child = children->list->items[0];
    if (!child || child->type != NC_STR || strcmp(child->s, "relocated-child")) return 3;
    if (!root->gc_first_edge || root->gc_ref_count != 1 ||
        ((NcGcEdge *)root->gc_first_edge)->from != root ||
        ((NcGcEdge *)root->gc_first_edge)->to != children) return 4;

    nvars = 0;
    nc_gc_collect();
    if (g_gc_allocated != 0 || g_gc_arena_page_count != 0) return 5;
    nc_gc_frame_leave(&frame);
    puts("native moving GC relocation: ok");
    return 0;
}
