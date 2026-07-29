#ifndef NORSCODE_WINDOWS_COMPAT_H
#define NORSCODE_WINDOWS_COMPAT_H

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

typedef SSIZE_T ssize_t;
typedef int socklen_t;
typedef SOCKET nc_socket_handle_t;
typedef struct { HANDLE handle; DWORD id; } pthread_t;
typedef SRWLOCK pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef struct { SIZE_T stack_size; } pthread_attr_t;
typedef INIT_ONCE pthread_once_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT
#define R_OK 4
static int nc_windows_access(const char *path,int mode){
    if(!path){errno=EINVAL;return -1;}int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);if(length<=0){errno=EINVAL;return -1;}
    wchar_t *wide=malloc((size_t)length*sizeof(wchar_t));if(!wide){errno=ENOMEM;return -1;}if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,length)){free(wide);errno=EINVAL;return -1;}
    int result=_waccess(wide,mode);free(wide);return result;
}
#define access(path, mode) nc_windows_access(path, mode)
#define mkdir(path, mode) _mkdir(path)
#define popen _popen
#define pclose _pclose
static int nc_windows_setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value);
}
#define setenv(name, value, overwrite) nc_windows_setenv(name, value, overwrite)
static int nc_windows_unsetenv(const char *name) {
    return _putenv_s(name, "");
}
#define unsetenv(name) nc_windows_unsetenv(name)

static int pthread_mutex_init(pthread_mutex_t *mutex, const void *attributes) {
    (void)attributes;
    InitializeSRWLock(mutex);
    return 0;
}
static int pthread_mutex_destroy(pthread_mutex_t *mutex) { (void)mutex; return 0; }
static int pthread_mutex_lock(pthread_mutex_t *mutex) { AcquireSRWLockExclusive(mutex); return 0; }
static int pthread_mutex_trylock(pthread_mutex_t *mutex) { return TryAcquireSRWLockExclusive(mutex) ? 0 : EBUSY; }
static int pthread_mutex_unlock(pthread_mutex_t *mutex) { ReleaseSRWLockExclusive(mutex); return 0; }
static int pthread_cond_init(pthread_cond_t *condition, const void *attributes) {
    (void)attributes;
    InitializeConditionVariable(condition);
    return 0;
}
static int pthread_cond_destroy(pthread_cond_t *condition) { (void)condition; return 0; }
static int pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex) {
    return SleepConditionVariableSRW(condition, mutex, INFINITE, 0) ? 0 : EINVAL;
}
static int pthread_cond_timedwait(pthread_cond_t *condition, pthread_mutex_t *mutex,
                                  const struct timespec *deadline) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    long long remaining = ((long long)deadline->tv_sec - now.tv_sec) * 1000LL +
                          ((long long)deadline->tv_nsec - now.tv_nsec) / 1000000LL;
    if (remaining < 0) remaining = 0;
    if (SleepConditionVariableSRW(condition, mutex,
                                  remaining > 0xfffffffeLL ? 0xfffffffeUL : (DWORD)remaining, 0)) return 0;
    return GetLastError() == ERROR_TIMEOUT ? ETIMEDOUT : EINVAL;
}
static int pthread_cond_broadcast(pthread_cond_t *condition) { WakeAllConditionVariable(condition); return 0; }
static int pthread_cond_signal(pthread_cond_t *condition) { WakeConditionVariable(condition); return 0; }
static pthread_t pthread_self(void) { pthread_t thread = {NULL, GetCurrentThreadId()}; return thread; }
static int pthread_equal(pthread_t left, pthread_t right) { return left.id == right.id; }

typedef struct {
    void *(*start)(void *);
    void *argument;
} NcwThreadStart;

static DWORD WINAPI ncw_thread_entry(LPVOID raw) {
    NcwThreadStart *start = (NcwThreadStart *)raw;
    void *(*function)(void *) = start->start;
    void *argument = start->argument;
    free(start);
    (void)function(argument);
    return 0;
}

static int pthread_attr_init(pthread_attr_t *attributes) { attributes->stack_size = 0; return 0; }
static int pthread_attr_destroy(pthread_attr_t *attributes) { (void)attributes; return 0; }
static int pthread_attr_setstacksize(pthread_attr_t *attributes, size_t stack_size) {
    if (!attributes || stack_size < 65536) return EINVAL;
    attributes->stack_size = (SIZE_T)stack_size;
    return 0;
}
static int pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
                          void *(*start)(void *), void *argument) {
    if (!thread || !start) return EINVAL;
    NcwThreadStart *context = (NcwThreadStart *)malloc(sizeof(*context));
    if (!context) return ENOMEM;
    context->start = start; context->argument = argument;
    SIZE_T stack_size = attributes ? attributes->stack_size : 0;
    thread->handle = CreateThread(NULL, stack_size, ncw_thread_entry, context,
                                  STACK_SIZE_PARAM_IS_A_RESERVATION, &thread->id);
    if (!thread->handle) { free(context); thread->id = 0; return EAGAIN; }
    return 0;
}
static int pthread_join(pthread_t thread, void **result) {
    (void)result;
    if (!thread.handle || WaitForSingleObject(thread.handle, INFINITE) != WAIT_OBJECT_0) return EINVAL;
    return CloseHandle(thread.handle) ? 0 : EINVAL;
}

static BOOL CALLBACK ncw_once_entry(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    (void)once; (void)context;
    void (*routine)(void) = (void (*)(void))parameter;
    routine();
    return TRUE;
}
static int pthread_once(pthread_once_t *once, void (*routine)(void)) {
    return InitOnceExecuteOnce(once, ncw_once_entry, (PVOID)routine, NULL) ? 0 : EINVAL;
}

static INIT_ONCE g_nc_windows_socket_once = INIT_ONCE_STATIC_INIT;
static int g_nc_windows_socket_status = 0;
static BOOL CALLBACK nc_windows_socket_initialize(PINIT_ONCE init_once, PVOID parameter, PVOID *context) {
    (void)init_once; (void)parameter; (void)context;
    WSADATA data;
    g_nc_windows_socket_status = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    return TRUE;
}
static int nc_windows_socket_startup(void) {
    InitOnceExecuteOnce(&g_nc_windows_socket_once, nc_windows_socket_initialize, NULL, NULL);
    return g_nc_windows_socket_status;
}

#endif
#endif
