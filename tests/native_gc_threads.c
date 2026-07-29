#define NC_RUNTIME_STANDALONE 1
#include "../archive/legacy_c_backend/nc_runtime_mini.c"

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int arrived;
} TestBarrier;

typedef struct {
    TestBarrier *barrier;
    int id;
    int failed;
} Worker;

static void test_barrier_wait(TestBarrier *barrier) {
    pthread_mutex_lock(&barrier->mutex);
    barrier->arrived++;
    if (barrier->arrived == 2) pthread_cond_broadcast(&barrier->condition);
    while (barrier->arrived < 2) pthread_cond_wait(&barrier->condition, &barrier->mutex);
    pthread_mutex_unlock(&barrier->mutex);
}

static void *gc_worker(void *raw) {
    Worker *worker = raw;
    NcVal **stack = calloc(512, sizeof(NcVal *));
    NcVal **vars = calloc(128, sizeof(NcVal *));
    int sp = 0, nvars = 0;
    NcGcFrame frame;
    nc_gc_frame_enter(&frame, stack, &sp, vars, &nvars);

    char expected[32];
    snprintf(expected, sizeof(expected), "worker-%d", worker->id);
    NcVal *root = nc_map_new();
    nc_index_set(root, nc_str("name"), nc_str(expected));
    vars[nvars++] = root;
    test_barrier_wait(worker->barrier);

    for (int i = 0; i < 75000; i++) {
        (void)nc_str("parallel-temporary-value");
        (void)nc_int(i);
        if ((i & 63) == 0) {
            nc_gc_safepoint();
            root = vars[0];
        }
        if ((i & 1023) == 0) {
            NcVal *name = nc_index_get(root, nc_str("name"));
            if (!name || name->type != NC_STR || strcmp(name->s, expected)) {
                worker->failed = 1;
                break;
            }
        }
    }

    nc_gc_frame_leave(&frame);
    free(vars);
    free(stack);
    return NULL;
}

int main(void) {
    TestBarrier barrier = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};
    Worker workers[2] = {{&barrier, 1, 0}, {&barrier, 2, 0}};
    pthread_t threads[2];
    for (int i = 0; i < 2; i++) pthread_create(&threads[i], NULL, gc_worker, &workers[i]);
    for (int i = 0; i < 2; i++) pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&barrier.mutex);
    pthread_cond_destroy(&barrier.condition);
    nc_gc_collect();
    if (workers[0].failed || workers[1].failed) return 1;
    if (g_gc_collections == 0) return 2;
    if (g_gc_allocated != 0) return 3;
    puts("native concurrent GC handshake: ok");
    return 0;
}
