#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

static void throw_from_child_frame(void) {
    NcVal *stack[8] = {0}, *vars[8] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame child;
    nc_gc_frame_enter(&child, stack, &sp, vars, &nvars);
    vars[nvars++] = nc_str("child-root");
    nc_panic("expected exception");
}

int main(void) {
    NcVal *stack[8] = {0}, *vars[8] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame parent;
    nc_gc_frame_enter(&parent, stack, &sp, vars, &nvars);
    vars[nvars++] = nc_str("parent-root");

    g_err_gc_boundary = &parent;
    if (setjmp(g_err_jmp) == 0) {
        throw_from_child_frame();
        return 1;
    }
    if (g_gc_frames != &parent || parent.next != NULL) return 2;

    g_err_gc_boundary = NULL;
    nc_gc_frame_leave(&parent);
    if (g_gc_frames != NULL) return 3;
    nc_gc_collect();
    if (g_gc_allocated != 0) return 4;

    puts("native GC exception unwind: ok");
    return 0;
}
