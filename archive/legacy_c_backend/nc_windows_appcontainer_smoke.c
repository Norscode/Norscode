#include "nc_windows_backend.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--child")) {
        fputs("appcontainer-ok", stdout);
        return 0;
    }
    wchar_t executable_wide[NCW_PATH_CAP];
    char executable[NCW_PATH_CAP], error[NCW_ERROR_CAP] = "";
    if (!GetModuleFileNameW(NULL, executable_wide, NCW_PATH_CAP) ||
        !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, executable_wide, -1,
                             executable, NCW_PATH_CAP, NULL, NULL)) return 2;
    const char *child_argv[] = {executable, "--child"};
    NcwProcess process;
    if (!ncw_process_spawn(&process, executable, child_argv, 2, "", NULL, 0,
                           5000, 128ULL * 1024ULL * 1024ULL, "no-network",
                           NULL, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        return 3;
    }
    if (!process.appcontainer || ncw_process_wait(&process, 5000, error, sizeof(error)) != 1) return 4;
    char output[64] = "";
    int64_t length = ncw_process_read(&process, 0, output, sizeof(output) - 1,
                                      error, sizeof(error));
    if (length != 15 || strcmp(output, "appcontainer-ok") || process.exit_code != 0) return 5;
    if (!ncw_process_close(&process, error, sizeof(error))) return 6;
    puts("nc_windows_appcontainer_smoke OK");
    return 0;
}
