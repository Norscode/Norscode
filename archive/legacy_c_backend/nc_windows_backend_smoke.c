#if !defined(_WIN32)
#error "Windows smoke requires a Windows target"
#endif

#include "nc_windows_backend.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *step, const char *error) {
    fprintf(stderr, "%s: %s\n", step, error ? error : "unknown error");
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "--child") && !strcmp(argv[2], "space and \"quote\"")) {
        fputs("process-ok", stdout);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--sleep")) { Sleep(5000); return 0; }
    char error[NCW_ERROR_CAP] = "";
    wchar_t temp[NCW_PATH_CAP], root_wide[NCW_PATH_CAP];
    DWORD temp_length = GetTempPathW(NCW_PATH_CAP, temp);
    if (!temp_length || temp_length >= NCW_PATH_CAP) return fail("GetTempPathW", "failed");
    if (_snwprintf(root_wide, NCW_PATH_CAP, L"%lsnorscode-windows-smoke-%lu",
                   temp, (unsigned long)GetCurrentProcessId()) < 0) return fail("root path", "overflow");
    if (!CreateDirectoryW(root_wide, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return fail("CreateDirectoryW", "failed");
    char root[NCW_PATH_CAP];
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, root_wide, -1,
                             root, NCW_PATH_CAP, NULL, NULL)) return fail("root UTF-8", "failed");

    NcwFile file;
    if (!ncw_file_open(&file, root, "data.bin", "write", error, sizeof(error)))
        return fail("file open write", error);
    const char payload[] = "norscode-windows-file";
    if (ncw_file_write(&file, payload, sizeof(payload) - 1, error, sizeof(error)) != (int64_t)(sizeof(payload) - 1))
        return fail("file write", error);
    if (!ncw_file_flush(&file, error, sizeof(error)) ||
        !ncw_file_close(&file, error, sizeof(error))) return fail("file close", error);
    if (ncw_file_open(&file, root, "..\\outside.bin", "read", error, sizeof(error)))
        return fail("traversal rejection", "unsafe path accepted");
    if (!ncw_file_open(&file, root, "data.bin", "read", error, sizeof(error)))
        return fail("file open read", error);
    char received[64] = "";
    if (ncw_file_read(&file, received, sizeof(received) - 1, error, sizeof(error)) != (int64_t)(sizeof(payload) - 1) ||
        strcmp(received, payload)) return fail("file read", error);
    if (!ncw_file_close(&file, error, sizeof(error)) ||
        !ncw_file_delete(root, "data.bin", error, sizeof(error))) return fail("file delete", error);
    error[0] = 0;

    wchar_t command_wide[NCW_PATH_CAP];
    if (!GetModuleFileNameW(NULL, command_wide, NCW_PATH_CAP))
        return fail("module path", "failed");
    char command[NCW_PATH_CAP];
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, command_wide, -1,
                             command, NCW_PATH_CAP, NULL, NULL)) return fail("module UTF-8", "failed");
    const char *child_argv[] = {command, "--child", "space and \"quote\""};
    NcwProcess process;
    if (!ncw_process_spawn(&process, command, child_argv, 3, root, NULL, 0,
                           5000, 128ULL * 1024ULL * 1024ULL,
                           "restricted",
                           error, sizeof(error))) return fail("process spawn", error);
    if (ncw_process_wait(&process, 5000, error, sizeof(error)) != 1)
        return fail("process wait", error);
    char process_output[64] = "";
    int64_t output_length = ncw_process_read(&process, 0, process_output,
                                             sizeof(process_output) - 1,
                                             error, sizeof(error));
    if (output_length != 10 || strcmp(process_output, "process-ok") || process.exit_code != 0) {
        char process_error[256] = "";
        int64_t error_length = ncw_process_read(&process, 1, process_error,
                                                sizeof(process_error) - 1,
                                                error, sizeof(error));
        fprintf(stderr, "process diagnostic: stdout_len=%lld stdout='%s' stderr_len=%lld stderr='%s' exit=%lu\n",
                (long long)output_length, process_output, (long long)error_length,
                process_error, (unsigned long)process.exit_code);
        return fail("process output", error);
    }
    if (!ncw_process_close(&process, error, sizeof(error))) return fail("process close", error);

    const char *sleep_argv[] = {command, "--sleep"};
    NcwProcess timeout_process;
    if (!ncw_process_spawn(&timeout_process, command, sleep_argv, 2, root, NULL, 0,
                           50, 128ULL * 1024ULL * 1024ULL,
                           "restricted",
                           error, sizeof(error))) return fail("timeout spawn", error);
    int timeout_status = 0;
    for (int i = 0; i < 100 && timeout_status == 0; i++) {
        timeout_status = ncw_process_wait(&timeout_process, 100, error, sizeof(error));
        if (timeout_status == 0) Sleep(10);
    }
    if (timeout_status != 1 || !timeout_process.timed_out || timeout_process.exit_code != 124)
        return fail("timeout enforcement", error);
    if (!ncw_process_close(&timeout_process, error, sizeof(error))) return fail("timeout close", error);

    NcwIocp iocp;
    NcwSocket listener = {INVALID_SOCKET}, client = {INVALID_SOCKET}, server = {INVALID_SOCKET};
    uint16_t port = 0;
    if (!ncw_iocp_open(&iocp, error, sizeof(error)) ||
        !ncw_socket_listen(&iocp, &listener, "127.0.0.1", 0, &port, error, sizeof(error)))
        return fail("IOCP listen", error);
    NcwIoOperation accept_operation, connect_operation;
    if (!ncw_socket_accept_async(&iocp, &listener, &server, &accept_operation, error, sizeof(error)) ||
        !ncw_socket_connect_async(&iocp, &client, "127.0.0.1", port, &connect_operation, error, sizeof(error)))
        return fail("IOCP connect/accept", error);
    int accept_done = 0, connect_done = 0;
    for (int i = 0; i < 4 && (!accept_done || !connect_done); i++) {
        NcwIoOperation *completed = NULL;
        if (ncw_iocp_wait(&iocp, &completed, 5000, error, sizeof(error)) != 1)
            return fail("IOCP handshake completion", error);
        if (completed == &accept_operation) accept_done = 1;
        if (completed == &connect_operation) connect_done = 1;
    }
    if (!accept_done || !connect_done) return fail("IOCP handshake", "missing completion");
    char network_buffer[16] = "";
    const char network_payload[] = "iocp-ping";
    NcwIoOperation read_operation, write_operation;
    if (!ncw_socket_read_async(&server, network_buffer, sizeof(network_buffer) - 1,
                               &read_operation, error, sizeof(error)) ||
        !ncw_socket_write_async(&client, network_payload, sizeof(network_payload) - 1,
                                &write_operation, error, sizeof(error)))
        return fail("IOCP read/write submit", error);
    int read_done = 0, write_done = 0;
    for (int i = 0; i < 4 && (!read_done || !write_done); i++) {
        NcwIoOperation *completed = NULL;
        if (ncw_iocp_wait(&iocp, &completed, 5000, error, sizeof(error)) != 1)
            return fail("IOCP data completion", error);
        if (completed == &read_operation) read_done = 1;
        if (completed == &write_operation) write_done = 1;
    }
    if (!read_done || !write_done || read_operation.transferred != sizeof(network_payload) - 1 ||
        write_operation.transferred != sizeof(network_payload) - 1 || strcmp(network_buffer, network_payload))
        return fail("IOCP payload", "payload mismatch");
    if (!ncw_socket_close(&client, error, sizeof(error)) ||
        !ncw_socket_close(&server, error, sizeof(error)) ||
        !ncw_socket_close(&listener, error, sizeof(error)) ||
        !ncw_iocp_close(&iocp, error, sizeof(error))) return fail("IOCP close", error);
    RemoveDirectoryW(root_wide);
    puts("nc_windows_backend_smoke OK");
    return 0;
}
