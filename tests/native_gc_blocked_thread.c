#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int ready;
} BlockState;

static long long monotonic_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long long)now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;
}

static void *blocked_worker(void *raw) {
    BlockState *state = raw;
    NcVal *stack[8] = {0}, *vars[8] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);
    vars[nvars++] = nc_str("blocked-root");
    pthread_mutex_lock(&state->mutex);
    state->ready = 1;
    pthread_cond_broadcast(&state->condition);
    pthread_mutex_unlock(&state->mutex);
    usleep(250000);
    nc_gc_frame_leave(&frame);
    return NULL;
}

int main(void) {
    BlockState state = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};
    pthread_t thread;
    pthread_create(&thread, NULL, blocked_worker, &state);
    pthread_mutex_lock(&state.mutex);
    while (!state.ready) pthread_cond_wait(&state.condition, &state.mutex);
    pthread_mutex_unlock(&state.mutex);

    NcVal *stack[8] = {0}, *vars[8] = {0};
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);
    for (int i = 0; i < 5000; i++) (void)nc_str("deferred-value");
    size_t before = g_gc_collections;
    long long started = monotonic_ms();
    nc_gc_safepoint();
    long long elapsed = monotonic_ms() - started;
    if (g_gc_collections != before || elapsed < 75 || elapsed > 220) return 1;
    nc_gc_frame_leave(&frame);

    pthread_join(thread, NULL);
    nc_gc_collect();
    pthread_mutex_destroy(&state.mutex);
    pthread_cond_destroy(&state.condition);
    if (g_gc_allocated != 0) return 2;
    puts("native GC blocked-thread deferral: ok");
    return 0;
}
