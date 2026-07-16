#if !defined(_WIN32)
#error "nc_windows_backend.c requires a Windows target"
#endif

#define WIN32_LEAN_AND_MEAN
#include "nc_windows_backend.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void ncw_error(char *error,size_t cap,const char *operation,DWORD code);

typedef HRESULT (WINAPI *NcwCreateAppContainerProfile)(PCWSTR,PCWSTR,PCWSTR,
    PSID_AND_ATTRIBUTES,DWORD,PSID *);
typedef HRESULT (WINAPI *NcwDeriveAppContainerSid)(PCWSTR,PSID *);

static int ncw_appcontainer_security(SECURITY_CAPABILITIES *security,PSID *owned_sid,
                                     char *error,size_t error_cap){
    HMODULE userenv=LoadLibraryW(L"userenv.dll");if(!userenv){ncw_error(error,error_cap,"LoadLibraryW(userenv)",GetLastError());return 0;}
    NcwCreateAppContainerProfile create_profile=(NcwCreateAppContainerProfile)(void *)GetProcAddress(userenv,"CreateAppContainerProfile");
    NcwDeriveAppContainerSid derive_sid=(NcwDeriveAppContainerSid)(void *)GetProcAddress(userenv,"DeriveAppContainerSidFromAppContainerName");
    if(!create_profile||!derive_sid){FreeLibrary(userenv);ncw_error(error,error_cap,"AppContainer API",ERROR_CALL_NOT_IMPLEMENTED);return 0;}
    const wchar_t *name=L"Norscode.Runtime.NoNetwork";PSID sid=NULL;HRESULT result=create_profile(name,L"Norscode Runtime",L"Norscode network-isolated process",NULL,0,&sid);
    if(FAILED(result)){result=derive_sid(name,&sid);if(FAILED(result)){FreeLibrary(userenv);ncw_error(error,error_cap,"AppContainer profile",(DWORD)result);return 0;}}
    memset(security,0,sizeof(*security));security->AppContainerSid=sid;security->Capabilities=NULL;security->CapabilityCount=0;security->Reserved=0;*owned_sid=sid;FreeLibrary(userenv);return 1;
}

static void ncw_error(char *error, size_t cap, const char *operation, DWORD code) {
    if (!error || cap == 0) return;
    char message[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, code, 0, message, (DWORD)sizeof(message), NULL);
    snprintf(error, cap, "%s failed (%lu): %s", operation,
             (unsigned long)code, message[0] ? message : "unknown Windows error");
}

static int ncw_utf8(const char *input, wchar_t *output, size_t cap,
                    char *error, size_t error_cap) {
    if (!input || !output || cap < 2) {
        ncw_error(error, error_cap, "UTF-8 conversion", ERROR_INVALID_PARAMETER);
        return 0;
    }
    int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1,
                                     output, (int)cap);
    if (needed <= 0) {
        ncw_error(error, error_cap, "UTF-8 conversion", GetLastError());
        return 0;
    }
    return 1;
}

static int ncw_relative_safe(const wchar_t *relative) {
    if (!relative || !*relative || relative[0] == L'/' || relative[0] == L'\\') return 0;
    if (wcslen(relative) >= NCW_PATH_CAP - 2 || wcschr(relative, L':')) return 0;
    const wchar_t *cursor = relative;
    while (*cursor) {
        const wchar_t *start = cursor;
        while (*cursor && *cursor != L'/' && *cursor != L'\\') cursor++;
        size_t length = (size_t)(cursor - start);
        if (length == 0 || (length == 1 && start[0] == L'.') ||
            (length == 2 && start[0] == L'.' && start[1] == L'.')) return 0;
        if (*cursor) cursor++;
    }
    return 1;
}

static void ncw_strip_namespace(wchar_t *path) {
    if (!wcsncmp(path, L"\\\\?\\UNC\\", 8)) {
        size_t length = wcslen(path + 8);
        memmove(path + 2, path + 8, (length + 1) * sizeof(wchar_t));
        path[0] = L'\\'; path[1] = L'\\';
    } else if (!wcsncmp(path, L"\\\\?\\", 4)) {
        memmove(path, path + 4, (wcslen(path + 4) + 1) * sizeof(wchar_t));
    }
}

static void ncw_trim_separator(wchar_t *path) {
    size_t length = wcslen(path);
    while (length > 3 && (path[length - 1] == L'\\' || path[length - 1] == L'/'))
        path[--length] = 0;
}

static void ncw_normalize_long_path(wchar_t *path) {
    wchar_t normalized[NCW_PATH_CAP];
    DWORD length = GetLongPathNameW(path, normalized, NCW_PATH_CAP);
    if (length && length < NCW_PATH_CAP) {
        wcsncpy(path, normalized, NCW_PATH_CAP - 1);
        path[NCW_PATH_CAP - 1] = 0;
    }
}

static int ncw_path_beneath(const wchar_t *root, const wchar_t *candidate) {
    size_t root_length = wcslen(root);
    if (_wcsnicmp(root, candidate, root_length)) return 0;
    return candidate[root_length] == 0 || candidate[root_length] == L'\\' ||
           candidate[root_length] == L'/';
}

static int ncw_resolve(const char *root_utf8, const char *relative_utf8,
                       wchar_t *root, wchar_t *candidate,
                       char *error, size_t error_cap) {
    wchar_t root_input[NCW_PATH_CAP], relative[NCW_PATH_CAP];
    if (!ncw_utf8(root_utf8, root_input, NCW_PATH_CAP, error, error_cap) ||
        !ncw_utf8(relative_utf8, relative, NCW_PATH_CAP, error, error_cap)) return 0;
    if (!ncw_relative_safe(relative)) {
        ncw_error(error, error_cap, "path validation", ERROR_INVALID_NAME);
        return 0;
    }
    DWORD root_length = GetFullPathNameW(root_input, NCW_PATH_CAP, root, NULL);
    if (!root_length || root_length >= NCW_PATH_CAP) {
        ncw_error(error, error_cap, "root resolution", GetLastError());
        return 0;
    }
    ncw_strip_namespace(root); ncw_normalize_long_path(root); ncw_trim_separator(root);
    wchar_t combined[NCW_PATH_CAP];
    int combined_length = _snwprintf(combined, NCW_PATH_CAP, L"%ls\\%ls", root, relative);
    if (combined_length < 0 || combined_length >= NCW_PATH_CAP) {
        ncw_error(error, error_cap, "path join", ERROR_BUFFER_OVERFLOW);
        return 0;
    }
    DWORD candidate_length = GetFullPathNameW(combined, NCW_PATH_CAP, candidate, NULL);
    if (!candidate_length || candidate_length >= NCW_PATH_CAP) {
        ncw_error(error, error_cap, "path resolution", GetLastError());
        return 0;
    }
    ncw_strip_namespace(candidate); ncw_normalize_long_path(candidate); ncw_trim_separator(candidate);
    if (!ncw_path_beneath(root, candidate)) {
        ncw_error(error, error_cap, "path scope", ERROR_ACCESS_DENIED);
        return 0;
    }
    return 1;
}

static int ncw_verify_open_handle(HANDLE handle, const wchar_t *root,
                                  char *error, size_t error_cap) {
    FILE_ATTRIBUTE_TAG_INFO tag;
    if (!GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag, sizeof(tag))) {
        ncw_error(error, error_cap, "file attribute query", GetLastError());
        return 0;
    }
    if (tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        ncw_error(error, error_cap, "reparse point rejection", ERROR_REPARSE_TAG_INVALID);
        return 0;
    }
    wchar_t final_path[NCW_PATH_CAP];
    DWORD length = GetFinalPathNameByHandleW(handle, final_path, NCW_PATH_CAP,
                                             FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (!length || length >= NCW_PATH_CAP) {
        ncw_error(error, error_cap, "final path query", GetLastError());
        return 0;
    }
    ncw_strip_namespace(final_path); ncw_normalize_long_path(final_path); ncw_trim_separator(final_path);
    if (!ncw_path_beneath(root, final_path)) {
        ncw_error(error, error_cap, "final path scope", ERROR_ACCESS_DENIED);
        return 0;
    }
    return 1;
}

int ncw_file_open(NcwFile *file, const char *root_utf8, const char *relative_utf8,
                  const char *mode, char *error, size_t error_cap) {
    if (!file || !mode) {
        ncw_error(error, error_cap, "file open", ERROR_INVALID_PARAMETER);
        return 0;
    }
    memset(file, 0, sizeof(*file)); file->handle = INVALID_HANDLE_VALUE;file->async_handle=INVALID_HANDLE_VALUE;
    wchar_t candidate[NCW_PATH_CAP];
    if (!ncw_resolve(root_utf8, relative_utf8, file->root, candidate, error, error_cap)) return 0;
    DWORD access = 0, creation = OPEN_EXISTING;
    if (!strcmp(mode, "read")) { access = GENERIC_READ; file->readable = 1; }
    else if (!strcmp(mode, "write")) { access = GENERIC_WRITE; creation = CREATE_ALWAYS; file->writable = 1; }
    else if (!strcmp(mode, "append")) { access = FILE_APPEND_DATA | SYNCHRONIZE; creation = OPEN_ALWAYS; file->writable = 1; }
    else if (!strcmp(mode, "readwrite")) { access = GENERIC_READ | GENERIC_WRITE; creation = OPEN_ALWAYS; file->readable = file->writable = 1; }
    else { ncw_error(error, error_cap, "file mode", ERROR_INVALID_PARAMETER); return 0; }
    HANDLE handle = CreateFileW(candidate, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creation,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (handle == INVALID_HANDLE_VALUE) { ncw_error(error, error_cap, "CreateFileW", GetLastError()); return 0; }
    if (!ncw_verify_open_handle(handle, file->root, error, error_cap)) { CloseHandle(handle); return 0; }
    if (!strcmp(mode, "append")) SetFilePointer(handle, 0, NULL, FILE_END);
    file->handle = handle;
    wcsncpy(file->path,candidate,NCW_PATH_CAP-1);file->path[NCW_PATH_CAP-1]=0;
    return 1;
}

int64_t ncw_file_read(NcwFile *file, void *buffer, size_t size,
                      char *error, size_t error_cap) {
    if (!file || file->handle == INVALID_HANDLE_VALUE || !file->readable || !buffer || size > 0xffffffffU) {
        ncw_error(error, error_cap, "file read", ERROR_INVALID_PARAMETER); return -1;
    }
    DWORD received = 0;
    if (!ReadFile(file->handle, buffer, (DWORD)size, &received, NULL)) {
        ncw_error(error, error_cap, "ReadFile", GetLastError()); return -1;
    }
    return (int64_t)received;
}

int64_t ncw_file_write(NcwFile *file, const void *buffer, size_t size,
                       char *error, size_t error_cap) {
    if (!file || file->handle == INVALID_HANDLE_VALUE || !file->writable || !buffer || size > 0xffffffffU) {
        ncw_error(error, error_cap, "file write", ERROR_INVALID_PARAMETER); return -1;
    }
    DWORD written = 0;
    if (!WriteFile(file->handle, buffer, (DWORD)size, &written, NULL)) {
        ncw_error(error, error_cap, "WriteFile", GetLastError()); return -1;
    }
    return (int64_t)written;
}

int64_t ncw_file_seek(NcwFile *file, int64_t offset,
                      char *error, size_t error_cap) {
    LARGE_INTEGER input, output; input.QuadPart = offset;
    if (!file || file->handle == INVALID_HANDLE_VALUE || offset < 0 ||
        !SetFilePointerEx(file->handle, input, &output, FILE_BEGIN)) {
        ncw_error(error, error_cap, "SetFilePointerEx", GetLastError()); return -1;
    }
    return output.QuadPart;
}

int64_t ncw_file_size(NcwFile *file, char *error, size_t error_cap) {
    LARGE_INTEGER size;
    if (!file || file->handle == INVALID_HANDLE_VALUE || !GetFileSizeEx(file->handle, &size)) {
        ncw_error(error, error_cap, "GetFileSizeEx", GetLastError()); return -1;
    }
    return size.QuadPart;
}

int ncw_file_flush(NcwFile *file, char *error, size_t error_cap) {
    if (!file || file->handle == INVALID_HANDLE_VALUE || !file->writable || !FlushFileBuffers(file->handle)) {
        ncw_error(error, error_cap, "FlushFileBuffers", GetLastError()); return 0;
    }
    return 1;
}

int ncw_file_close(NcwFile *file, char *error, size_t error_cap) {
    if (!file || file->handle == INVALID_HANDLE_VALUE) {
        ncw_error(error, error_cap, "file close", ERROR_INVALID_HANDLE); return 0;
    }
    HANDLE handle = file->handle,async_handle=file->async_handle; file->handle = INVALID_HANDLE_VALUE;file->async_handle=INVALID_HANDLE_VALUE;
    if(async_handle!=INVALID_HANDLE_VALUE&&!CloseHandle(async_handle)){ncw_error(error,error_cap,"async CloseHandle",GetLastError());CloseHandle(handle);return 0;}
    if (!CloseHandle(handle)) { ncw_error(error, error_cap, "CloseHandle", GetLastError()); return 0; }
    return 1;
}

int ncw_file_delete(const char *root_utf8, const char *relative_utf8,
                    char *error, size_t error_cap) {
    wchar_t root[NCW_PATH_CAP], candidate[NCW_PATH_CAP];
    if (!ncw_resolve(root_utf8, relative_utf8, root, candidate, error, error_cap)) return 0;
    HANDLE handle = CreateFileW(candidate, DELETE | FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (handle == INVALID_HANDLE_VALUE) { ncw_error(error, error_cap, "delete open", GetLastError()); return 0; }
    if (!ncw_verify_open_handle(handle, root, error, error_cap)) { CloseHandle(handle); return 0; }
    FILE_DISPOSITION_INFO disposition; disposition.DeleteFile = TRUE;
    int ok = SetFileInformationByHandle(handle, FileDispositionInfo,
                                        &disposition, sizeof(disposition)) != 0;
    if (!ok) ncw_error(error, error_cap, "file delete", GetLastError());
    CloseHandle(handle);
    return ok;
}

static int ncw_append_wide(wchar_t *output, size_t cap, size_t *length,
                           const wchar_t *value, size_t count) {
    if (*length + count + 1 > cap) return 0;
    memcpy(output + *length, value, count * sizeof(wchar_t));
    *length += count; output[*length] = 0; return 1;
}

static int ncw_quote_argument(wchar_t *output, size_t cap, size_t *length,
                              const wchar_t *argument) {
    if (!ncw_append_wide(output, cap, length, L"\"", 1)) return 0;
    size_t slashes = 0;
    for (const wchar_t *cursor = argument;; cursor++) {
        if (*cursor == L'\\') { slashes++; continue; }
        if (*cursor == L'\"') {
            for (size_t i = 0; i < slashes * 2 + 1; i++)
                if (!ncw_append_wide(output, cap, length, L"\\", 1)) return 0;
            if (!ncw_append_wide(output, cap, length, L"\"", 1)) return 0;
            slashes = 0; continue;
        }
        if (*cursor == 0) {
            for (size_t i = 0; i < slashes * 2; i++)
                if (!ncw_append_wide(output, cap, length, L"\\", 1)) return 0;
            break;
        }
        for (size_t i = 0; i < slashes; i++)
            if (!ncw_append_wide(output, cap, length, L"\\", 1)) return 0;
        slashes = 0;
        if (!ncw_append_wide(output, cap, length, cursor, 1)) return 0;
    }
    return ncw_append_wide(output, cap, length, L"\"", 1);
}

static int ncw_command_line(const char *const *argv, size_t argc,
                            wchar_t *output, size_t cap,
                            char *error, size_t error_cap) {
    size_t length = 0; output[0] = 0;
    if (!argv || argc == 0) { ncw_error(error, error_cap, "argv", ERROR_INVALID_PARAMETER); return 0; }
    for (size_t i = 0; i < argc; i++) {
        wchar_t argument[NCW_PATH_CAP];
        if (!ncw_utf8(argv[i], argument, NCW_PATH_CAP, error, error_cap)) return 0;
        if (i && !ncw_append_wide(output, cap, &length, L" ", 1)) goto overflow;
        if (!ncw_quote_argument(output, cap, &length, argument)) goto overflow;
    }
    return 1;
overflow:
    ncw_error(error, error_cap, "command line", ERROR_BUFFER_OVERFLOW); return 0;
}

static void ncw_close_handle(HANDLE *handle) {
    if (*handle && *handle != INVALID_HANDLE_VALUE) CloseHandle(*handle);
    *handle = NULL;
}

static int ncw_pipe_pair(HANDLE *parent, HANDLE *child, int parent_reads,
                         char *error, size_t error_cap) {
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    HANDLE read_handle = NULL, write_handle = NULL;
    if (!CreatePipe(&read_handle, &write_handle, &security, 0)) {
        ncw_error(error, error_cap, "CreatePipe", GetLastError()); return 0;
    }
    *parent = parent_reads ? read_handle : write_handle;
    *child = parent_reads ? write_handle : read_handle;
    if (!SetHandleInformation(*parent, HANDLE_FLAG_INHERIT, 0)) {
        ncw_error(error, error_cap, "SetHandleInformation", GetLastError());
        CloseHandle(read_handle); CloseHandle(write_handle); return 0;
    }
    return 1;
}

int ncw_process_spawn(NcwProcess *process, const char *executable_utf8,
                      const char *const *argv_utf8, size_t argc,
                      const char *cwd_utf8, const void *stdin_data, size_t stdin_size,
                      uint64_t timeout_ms, uint64_t max_memory_bytes,
                      const char *sandbox_profile,
                      char *error, size_t error_cap) {
    if (!process || !executable_utf8 || !*executable_utf8 || !argv_utf8 || argc == 0 ||
        timeout_ms == 0 || stdin_size > 0xffffffffU) {
        ncw_error(error, error_cap, "process spawn", ERROR_INVALID_PARAMETER); return 0;
    }
    memset(process, 0, sizeof(*process)); process->exit_code = STILL_ACTIVE;
    wchar_t executable[NCW_PATH_CAP], cwd[NCW_PATH_CAP], command[32768];
    if (!ncw_utf8(executable_utf8, executable, NCW_PATH_CAP, error, error_cap) ||
        !ncw_command_line(argv_utf8, argc, command, 32768, error, error_cap)) return 0;
    wchar_t *cwd_pointer = NULL;
    if (cwd_utf8 && *cwd_utf8) {
        if (!ncw_utf8(cwd_utf8, cwd, NCW_PATH_CAP, error, error_cap)) return 0;
        cwd_pointer = cwd;
    }

    HANDLE child_stdin = NULL, child_stdout = NULL, child_stderr = NULL;
    if (!ncw_pipe_pair(&process->stdin_write, &child_stdin, 0, error, error_cap) ||
        !ncw_pipe_pair(&process->stdout_read, &child_stdout, 1, error, error_cap) ||
        !ncw_pipe_pair(&process->stderr_read, &child_stderr, 1, error, error_cap)) goto failure;

    int use_appcontainer=sandbox_profile&&(!strcmp(sandbox_profile,"no-network")||!strcmp(sandbox_profile,"pure"));
    SECURITY_CAPABILITIES security_capabilities;PSID appcontainer_sid=NULL;
    if(use_appcontainer&&!ncw_appcontainer_security(&security_capabilities,&appcontainer_sid,error,error_cap))goto failure;
    SIZE_T attribute_size = 0;
    InitializeProcThreadAttributeList(NULL, use_appcontainer?2:1, 0, &attribute_size);
    LPPROC_THREAD_ATTRIBUTE_LIST attributes = HeapAlloc(GetProcessHeap(), 0, attribute_size);
    if (!attributes || !InitializeProcThreadAttributeList(attributes, use_appcontainer?2:1, 0, &attribute_size)) {
        ncw_error(error, error_cap, "process attribute list", GetLastError());
        if (attributes) HeapFree(GetProcessHeap(), 0, attributes);if(appcontainer_sid)FreeSid(appcontainer_sid); goto failure;
    }
    HANDLE inherited[3] = {child_stdin, child_stdout, child_stderr};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   inherited, sizeof(inherited), NULL, NULL)) {
        ncw_error(error, error_cap, "inherited handle list", GetLastError());
        DeleteProcThreadAttributeList(attributes); HeapFree(GetProcessHeap(), 0, attributes);if(appcontainer_sid)FreeSid(appcontainer_sid); goto failure;
    }
    if(use_appcontainer&&!UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
                                                    &security_capabilities,sizeof(security_capabilities),NULL,NULL)){
        ncw_error(error,error_cap,"AppContainer process attribute",GetLastError());DeleteProcThreadAttributeList(attributes);HeapFree(GetProcessHeap(),0,attributes);FreeSid(appcontainer_sid);goto failure;
    }

    STARTUPINFOEXW startup; memset(&startup, 0, sizeof(startup));
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = child_stdin;
    startup.StartupInfo.hStdOutput = child_stdout;
    startup.StartupInfo.hStdError = child_stderr;
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION info; memset(&info, 0, sizeof(info));
    DWORD flags = EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED | CREATE_NO_WINDOW;
    int created = CreateProcessW(executable, command, NULL, NULL, TRUE, flags, NULL,
                                 cwd_pointer, &startup.StartupInfo, &info) != 0;
    DWORD create_error = created ? ERROR_SUCCESS : GetLastError();
    DeleteProcThreadAttributeList(attributes); HeapFree(GetProcessHeap(), 0, attributes);
    if(appcontainer_sid)FreeSid(appcontainer_sid);
    ncw_close_handle(&child_stdin); ncw_close_handle(&child_stdout); ncw_close_handle(&child_stderr);
    if (!created) { ncw_error(error, error_cap, "CreateProcessW", create_error); goto failure; }

    process->job = CreateJobObjectW(NULL, NULL);
    if (!process->job) { ncw_error(error, error_cap, "CreateJobObjectW", GetLastError()); TerminateProcess(info.hProcess, 126); CloseHandle(info.hThread); CloseHandle(info.hProcess); goto failure; }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits; memset(&limits, 0, sizeof(limits));
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                              JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    limits.BasicLimitInformation.ActiveProcessLimit = 1;
    if (max_memory_bytes) {
        limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        limits.ProcessMemoryLimit = (SIZE_T)max_memory_bytes;
    }
    if (!SetInformationJobObject(process->job, JobObjectExtendedLimitInformation,
                                 &limits, sizeof(limits)) ||
        !AssignProcessToJobObject(process->job, info.hProcess)) {
        ncw_error(error, error_cap, "Job Object assignment", GetLastError());
        TerminateProcess(info.hProcess, 126); CloseHandle(info.hThread); CloseHandle(info.hProcess); goto failure;
    }
    process->process = info.hProcess; process->thread = info.hThread;
    process->appcontainer=use_appcontainer;
    process->pid = info.dwProcessId; process->deadline_ms = GetTickCount64() + timeout_ms;
    if (ResumeThread(process->thread) == (DWORD)-1) {
        ncw_error(error, error_cap, "ResumeThread", GetLastError()); goto failure;
    }
    ncw_close_handle(&process->thread);
    if (stdin_size) {
        DWORD written = 0;
        if (!WriteFile(process->stdin_write, stdin_data, (DWORD)stdin_size, &written, NULL) || written != stdin_size) {
            ncw_error(error, error_cap, "process stdin", GetLastError()); goto failure;
        }
    }
    ncw_close_handle(&process->stdin_write);
    return 1;

failure:
    ncw_close_handle(&child_stdin); ncw_close_handle(&child_stdout); ncw_close_handle(&child_stderr);
    ncw_close_handle(&process->stdin_write); ncw_close_handle(&process->stdout_read);
    ncw_close_handle(&process->stderr_read); ncw_close_handle(&process->thread);
    if (process->process) TerminateProcess(process->process, 126);
    ncw_close_handle(&process->process); ncw_close_handle(&process->job);
    return 0;
}

int ncw_process_poll(NcwProcess *process, char *error, size_t error_cap) {
    if (!process || !process->process) { ncw_error(error, error_cap, "process poll", ERROR_INVALID_HANDLE); return -1; }
    if (!process->exited && GetTickCount64() >= process->deadline_ms) {
        process->timed_out = 1;
        if (!TerminateJobObject(process->job, 124)) { ncw_error(error, error_cap, "process timeout", GetLastError()); return -1; }
    }
    DWORD code = STILL_ACTIVE;
    if (!GetExitCodeProcess(process->process, &code)) { ncw_error(error, error_cap, "GetExitCodeProcess", GetLastError()); return -1; }
    if (code != STILL_ACTIVE) { process->exited = 1; process->exit_code = code; return 1; }
    return 0;
}

int64_t ncw_process_read(NcwProcess *process, int stderr_stream,
                         void *buffer, size_t size, char *error, size_t error_cap) {
    HANDLE pipe = stderr_stream ? process->stderr_read : process->stdout_read;
    if (!process || !pipe || !buffer || size == 0 || size > 0xffffffffU) {
        ncw_error(error, error_cap, "process read", ERROR_INVALID_PARAMETER); return -1;
    }
    DWORD available = 0;
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &available, NULL)) {
        DWORD code = GetLastError();
        if (code == ERROR_BROKEN_PIPE) return 0;
        ncw_error(error, error_cap, "PeekNamedPipe", code); return -1;
    }
    if (!available) return 0;
    DWORD received = 0, wanted = available < size ? available : (DWORD)size;
    if (!ReadFile(pipe, buffer, wanted, &received, NULL)) {
        ncw_error(error, error_cap, "process pipe read", GetLastError()); return -1;
    }
    return (int64_t)received;
}

int ncw_process_wait(NcwProcess *process, uint64_t wait_ms,
                     char *error, size_t error_cap) {
    if (!process || !process->process) { ncw_error(error, error_cap, "process wait", ERROR_INVALID_HANDLE); return -1; }
    ULONGLONG now = GetTickCount64();
    ULONGLONG deadline_remaining = process->deadline_ms > now ? process->deadline_ms - now : 0;
    ULONGLONG requested = wait_ms < deadline_remaining ? wait_ms : deadline_remaining;
    DWORD timeout = requested > 0xfffffffeULL ? 0xfffffffeUL : (DWORD)requested;
    DWORD status = WaitForSingleObject(process->process, timeout);
    if (status == WAIT_FAILED) { ncw_error(error, error_cap, "WaitForSingleObject", GetLastError()); return -1; }
    if (status == WAIT_TIMEOUT) return ncw_process_poll(process, error, error_cap);
    return ncw_process_poll(process, error, error_cap);
}

int ncw_process_terminate(NcwProcess *process, DWORD exit_code,
                          char *error, size_t error_cap) {
    if (!process || !process->job || process->exited) {
        ncw_error(error, error_cap, "process terminate", ERROR_INVALID_HANDLE); return 0;
    }
    if (!TerminateJobObject(process->job, exit_code)) {
        ncw_error(error, error_cap, "TerminateJobObject", GetLastError()); return 0;
    }
    return 1;
}

int ncw_process_close(NcwProcess *process, char *error, size_t error_cap) {
    if (!process || !process->process) { ncw_error(error, error_cap, "process close", ERROR_INVALID_HANDLE); return 0; }
    if (!process->exited && ncw_process_poll(process, error, error_cap) <= 0) {
        ncw_error(error, error_cap, "process close", ERROR_BUSY); return 0;
    }
    ncw_close_handle(&process->stdin_write); ncw_close_handle(&process->stdout_read);
    ncw_close_handle(&process->stderr_read); ncw_close_handle(&process->thread);
    ncw_close_handle(&process->process); ncw_close_handle(&process->job);
    return 1;
}

static INIT_ONCE g_ncw_winsock_once = INIT_ONCE_STATIC_INIT;
static int g_ncw_winsock_ok = 0;
static BOOL CALLBACK ncw_winsock_initialize(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    (void)once; (void)parameter; (void)context;
    WSADATA data; g_ncw_winsock_ok = WSAStartup(MAKEWORD(2, 2), &data) == 0; return TRUE;
}
static int ncw_winsock(char *error, size_t error_cap) {
    InitOnceExecuteOnce(&g_ncw_winsock_once, ncw_winsock_initialize, NULL, NULL);
    if (!g_ncw_winsock_ok) ncw_error(error, error_cap, "WSAStartup", (DWORD)WSAGetLastError());
    return g_ncw_winsock_ok;
}

static int ncw_associate(NcwIocp *iocp, SOCKET socket, char *error, size_t error_cap) {
    if (!CreateIoCompletionPort((HANDLE)socket, iocp->port, 0, 0)) {
        ncw_error(error, error_cap, "CreateIoCompletionPort association", GetLastError()); return 0;
    }
    return 1;
}

static int ncw_ipv4(const char *host, uint16_t port, struct sockaddr_in *address) {
    memset(address, 0, sizeof(*address)); address->sin_family = AF_INET; address->sin_port = htons(port);
    if (!host || !strcmp(host, "0.0.0.0")) address->sin_addr.s_addr = htonl(INADDR_ANY);
    else if (!strcmp(host, "localhost")) address->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    else if (InetPtonA(AF_INET, host, &address->sin_addr) != 1) return 0;
    return 1;
}

static int ncw_extension(SOCKET socket, const GUID *guid, void **function,
                         char *error, size_t error_cap) {
    DWORD bytes = 0;
    if (WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (void *)guid, sizeof(*guid),
                 function, sizeof(*function), &bytes, NULL, NULL) == SOCKET_ERROR) {
        ncw_error(error, error_cap, "WSAIoctl extension", (DWORD)WSAGetLastError()); return 0;
    }
    return 1;
}

int ncw_iocp_open(NcwIocp *iocp, char *error, size_t error_cap) {
    if (!iocp || !ncw_winsock(error, error_cap)) return 0;
    iocp->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!iocp->port) { ncw_error(error, error_cap, "CreateIoCompletionPort", GetLastError()); return 0; }
    return 1;
}

int ncw_iocp_close(NcwIocp *iocp, char *error, size_t error_cap) {
    if (!iocp || !iocp->port) { ncw_error(error, error_cap, "IOCP close", ERROR_INVALID_HANDLE); return 0; }
    HANDLE port = iocp->port; iocp->port = NULL;
    if (!CloseHandle(port)) { ncw_error(error, error_cap, "IOCP CloseHandle", GetLastError()); return 0; }
    return 1;
}

static int ncw_file_async_handle(NcwIocp *iocp,NcwFile *file,
                                 char *error,size_t error_cap){
    if(!iocp||!iocp->port||!file||file->handle==INVALID_HANDLE_VALUE)return 0;if(file->async_handle!=INVALID_HANDLE_VALUE)return 1;DWORD access=(file->readable?GENERIC_READ:0)|(file->writable?GENERIC_WRITE:0);
    HANDLE handle=CreateFileW(file->path,access,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OVERLAPPED,NULL);if(handle==INVALID_HANDLE_VALUE){ncw_error(error,error_cap,"async CreateFileW",GetLastError());return 0;}if(!ncw_verify_open_handle(handle,file->root,error,error_cap)){CloseHandle(handle);return 0;}if(!CreateIoCompletionPort(handle,iocp->port,0,0)){ncw_error(error,error_cap,"file IOCP association",GetLastError());CloseHandle(handle);return 0;}file->async_handle=handle;return 1;
}

int ncw_file_read_async(NcwIocp *iocp,NcwFile *file,uint64_t offset,
                        void *buffer,size_t size,NcwIoOperation *operation,
                        char *error,size_t error_cap){
    if(!file||!file->readable||!buffer||!size||size>0xffffffffU||!operation||!ncw_file_async_handle(iocp,file,error,error_cap)){if(error&&error_cap&&!error[0])ncw_error(error,error_cap,"async file read",ERROR_INVALID_PARAMETER);return 0;}memset(operation,0,sizeof(*operation));operation->kind=5;operation->owner_handle=file->async_handle;operation->buffer.buf=buffer;operation->buffer.len=(ULONG)size;operation->overlapped.Offset=(DWORD)offset;operation->overlapped.OffsetHigh=(DWORD)(offset>>32);DWORD received=0;BOOL ok=ReadFile(file->async_handle,buffer,(DWORD)size,&received,&operation->overlapped);if(!ok&&GetLastError()!=ERROR_IO_PENDING){ncw_error(error,error_cap,"overlapped ReadFile",GetLastError());return 0;}return 1;
}

int ncw_file_write_async(NcwIocp *iocp,NcwFile *file,uint64_t offset,
                         const void *buffer,size_t size,NcwIoOperation *operation,
                         char *error,size_t error_cap){
    if(!file||!file->writable||!buffer||!size||size>0xffffffffU||!operation||!ncw_file_async_handle(iocp,file,error,error_cap)){if(error&&error_cap&&!error[0])ncw_error(error,error_cap,"async file write",ERROR_INVALID_PARAMETER);return 0;}memset(operation,0,sizeof(*operation));operation->kind=6;operation->owner_handle=file->async_handle;operation->buffer.buf=(CHAR *)buffer;operation->buffer.len=(ULONG)size;operation->overlapped.Offset=(DWORD)offset;operation->overlapped.OffsetHigh=(DWORD)(offset>>32);DWORD written=0;BOOL ok=WriteFile(file->async_handle,buffer,(DWORD)size,&written,&operation->overlapped);if(!ok&&GetLastError()!=ERROR_IO_PENDING){ncw_error(error,error_cap,"overlapped WriteFile",GetLastError());return 0;}return 1;
}

int ncw_socket_listen(NcwIocp *iocp, NcwSocket *listener,
                      const char *host, uint16_t port, uint16_t *actual_port,
                      char *error, size_t error_cap) {
    if (!iocp || !iocp->port || !listener) return 0;
    listener->socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listener->socket == INVALID_SOCKET) { ncw_error(error, error_cap, "WSASocketW", (DWORD)WSAGetLastError()); return 0; }
    struct sockaddr_in address;
    if (!ncw_ipv4(host, port, &address) || bind(listener->socket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR ||
        listen(listener->socket, SOMAXCONN) == SOCKET_ERROR) {
        ncw_error(error, error_cap, "socket listen", (DWORD)WSAGetLastError()); closesocket(listener->socket); listener->socket = INVALID_SOCKET; return 0;
    }
    if (!ncw_associate(iocp, listener->socket, error, error_cap)) { closesocket(listener->socket); listener->socket = INVALID_SOCKET; return 0; }
    int length = sizeof(address);
    if (getsockname(listener->socket, (struct sockaddr *)&address, &length) == SOCKET_ERROR) {
        ncw_error(error, error_cap, "getsockname", (DWORD)WSAGetLastError()); return 0;
    }
    if (actual_port) *actual_port = ntohs(address.sin_port);
    return 1;
}

int ncw_socket_accept_async(NcwIocp *iocp, NcwSocket *listener,
                            NcwSocket *accepted, NcwIoOperation *operation,
                            char *error, size_t error_cap) {
    if (!iocp || !listener || !accepted || !operation) return 0;
    accepted->socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (accepted->socket == INVALID_SOCKET) { ncw_error(error, error_cap, "accept socket", (DWORD)WSAGetLastError()); return 0; }
    LPFN_ACCEPTEX accept_ex = NULL; GUID guid = WSAID_ACCEPTEX;
    if (!ncw_extension(listener->socket, &guid, (void **)&accept_ex, error, error_cap)) return 0;
    memset(operation, 0, sizeof(*operation)); operation->kind = 1; operation->socket = accepted; operation->listener = listener;operation->owner_handle=(HANDLE)listener->socket;
    DWORD received = 0;
    BOOL ok = accept_ex(listener->socket, accepted->socket, operation->address_buffer, 0,
                        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                        &received, &operation->overlapped);
    if (!ok && WSAGetLastError() != WSA_IO_PENDING) {
        ncw_error(error, error_cap, "AcceptEx", (DWORD)WSAGetLastError()); closesocket(accepted->socket); accepted->socket = INVALID_SOCKET; return 0;
    }
    return 1;
}

int ncw_socket_connect_async(NcwIocp *iocp, NcwSocket *socket,
                             const char *host, uint16_t port,
                             NcwIoOperation *operation,
                             char *error, size_t error_cap) {
    if (!iocp || !socket || !operation) return 0;
    socket->socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket->socket == INVALID_SOCKET) { ncw_error(error, error_cap, "connect socket", (DWORD)WSAGetLastError()); return 0; }
    struct sockaddr_in local, remote;
    if (!ncw_ipv4("0.0.0.0", 0, &local) || !ncw_ipv4(host, port, &remote) ||
        bind(socket->socket, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR ||
        !ncw_associate(iocp, socket->socket, error, error_cap)) return 0;
    LPFN_CONNECTEX connect_ex = NULL; GUID guid = WSAID_CONNECTEX;
    if (!ncw_extension(socket->socket, &guid, (void **)&connect_ex, error, error_cap)) return 0;
    memset(operation, 0, sizeof(*operation)); operation->kind = 2; operation->socket = socket;operation->owner_handle=(HANDLE)socket->socket;
    BOOL ok = connect_ex(socket->socket, (struct sockaddr *)&remote, sizeof(remote),
                         NULL, 0, NULL, &operation->overlapped);
    if (!ok && WSAGetLastError() != WSA_IO_PENDING) {
        ncw_error(error, error_cap, "ConnectEx", (DWORD)WSAGetLastError()); return 0;
    }
    return 1;
}

int ncw_socket_read_async(NcwSocket *socket, void *buffer, size_t size,
                          NcwIoOperation *operation, char *error, size_t error_cap) {
    if (!socket || socket->socket == INVALID_SOCKET || !buffer || !size || size > 0xffffffffU || !operation) return 0;
    memset(operation, 0, sizeof(*operation)); operation->kind = 3; operation->socket = socket;operation->owner_handle=(HANDLE)socket->socket;
    operation->buffer.buf = buffer; operation->buffer.len = (ULONG)size;
    DWORD flags = 0, received = 0;
    int result = WSARecv(socket->socket, &operation->buffer, 1, &received, &flags, &operation->overlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        ncw_error(error, error_cap, "WSARecv", (DWORD)WSAGetLastError()); return 0;
    }
    return 1;
}

int ncw_socket_write_async(NcwSocket *socket, const void *buffer, size_t size,
                           NcwIoOperation *operation, char *error, size_t error_cap) {
    if (!socket || socket->socket == INVALID_SOCKET || !buffer || !size || size > 0xffffffffU || !operation) return 0;
    memset(operation, 0, sizeof(*operation)); operation->kind = 4; operation->socket = socket;operation->owner_handle=(HANDLE)socket->socket;
    operation->buffer.buf = (CHAR *)buffer; operation->buffer.len = (ULONG)size;
    DWORD sent = 0;
    int result = WSASend(socket->socket, &operation->buffer, 1, &sent, 0, &operation->overlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        ncw_error(error, error_cap, "WSASend", (DWORD)WSAGetLastError()); return 0;
    }
    return 1;
}

int ncw_iocp_wait(NcwIocp *iocp, NcwIoOperation **completed, uint32_t timeout_ms,
                  char *error, size_t error_cap) {
    if (!iocp || !iocp->port || !completed) return -1;
    DWORD transferred = 0; ULONG_PTR key = 0; OVERLAPPED *overlapped = NULL;
    BOOL ok = GetQueuedCompletionStatus(iocp->port, &transferred, &key, &overlapped, timeout_ms);
    (void)key;
    if (!overlapped) {
        if (!ok && GetLastError() == WAIT_TIMEOUT) { *completed = NULL; return 0; }
        ncw_error(error, error_cap, "GetQueuedCompletionStatus", GetLastError()); return -1;
    }
    NcwIoOperation *operation = (NcwIoOperation *)((char *)overlapped - offsetof(NcwIoOperation, overlapped));
    operation->transferred = transferred; operation->error_code = ok ? ERROR_SUCCESS : GetLastError();
    if (operation->kind == 1 && ok) {
        setsockopt(operation->socket->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                   (const char *)&operation->listener->socket, sizeof(operation->listener->socket));
        if (!ncw_associate(iocp, operation->socket->socket, error, error_cap)) return -1;
    } else if (operation->kind == 2 && ok) {
        setsockopt(operation->socket->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
    }
    *completed = operation;
    return 1;
}

int ncw_socket_close(NcwSocket *socket, char *error, size_t error_cap) {
    if (!socket || socket->socket == INVALID_SOCKET) { ncw_error(error, error_cap, "socket close", WSAENOTSOCK); return 0; }
    SOCKET value = socket->socket; socket->socket = INVALID_SOCKET;
    if (closesocket(value) == SOCKET_ERROR) { ncw_error(error, error_cap, "closesocket", (DWORD)WSAGetLastError()); return 0; }
    return 1;
}

static int ncw_socket_send_all(SOCKET socket,const unsigned char *data,size_t size,
                               char *error,size_t error_cap){
    size_t offset=0;while(offset<size){int chunk=(int)((size-offset)>INT_MAX?INT_MAX:size-offset);int sent=send(socket,(const char *)data+offset,chunk,0);if(sent==SOCKET_ERROR){int socket_error=WSAGetLastError();if(socket_error==WSAEWOULDBLOCK){WSAPOLLFD descriptor={socket,POLLWRNORM,0};if(WSAPoll(&descriptor,1,5000)>0)continue;}ncw_error(error,error_cap,"TLS send",(DWORD)socket_error);return 0;}if(sent==0){ncw_error(error,error_cap,"TLS send",WSAECONNRESET);return 0;}offset+=(size_t)sent;}return 1;
}

static int ncw_schannel_receive(NcwSChannelClient *tls,NcwSocket *socket,
                                char *error,size_t error_cap){
    if(tls->encrypted_length>=sizeof(tls->encrypted)){ncw_error(error,error_cap,"SChannel receive buffer",ERROR_BUFFER_OVERFLOW);return -1;}
    int received=recv(socket->socket,(char *)tls->encrypted+tls->encrypted_length,(int)(sizeof(tls->encrypted)-tls->encrypted_length),0);
    if(received==0)return 0;if(received==SOCKET_ERROR){int socket_error=WSAGetLastError();if(socket_error==WSAEWOULDBLOCK)return -2;ncw_error(error,error_cap,"TLS recv",(DWORD)socket_error);return -1;}tls->encrypted_length+=(size_t)received;return received;
}

static void ncw_schannel_keep_extra(NcwSChannelClient *tls,SecBuffer *buffers,size_t count){
    size_t extra=0;unsigned char *source=NULL;for(size_t i=0;i<count;i++)if(buffers[i].BufferType==SECBUFFER_EXTRA){extra=buffers[i].cbBuffer;source=(unsigned char *)buffers[i].pvBuffer;break;}
    if(extra&&source)memmove(tls->encrypted,source,extra);tls->encrypted_length=extra;
}

int ncw_schannel_client_handshake(NcwSChannelClient *tls,NcwSocket *socket,
                                  const char *hostname_utf8,
                                  const char *client_certificate_subject_utf8,
                                  char *error,size_t error_cap){
    if(!tls||!socket||socket->socket==INVALID_SOCKET||!hostname_utf8||!*hostname_utf8){ncw_error(error,error_cap,"SChannel handshake",ERROR_INVALID_PARAMETER);return 0;}
    memset(tls,0,sizeof(*tls));wchar_t hostname[256];if(!ncw_utf8(hostname_utf8,hostname,256,error,error_cap))return 0;
    SCHANNEL_CRED credentials;memset(&credentials,0,sizeof(credentials));credentials.dwVersion=SCHANNEL_CRED_VERSION;credentials.grbitEnabledProtocols=SP_PROT_TLS1_3_CLIENT;credentials.dwFlags=SCH_USE_STRONG_CRYPTO|SCH_CRED_AUTO_CRED_VALIDATION|SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    if(client_certificate_subject_utf8&&*client_certificate_subject_utf8){wchar_t subject[256];if(!ncw_utf8(client_certificate_subject_utf8,subject,256,error,error_cap))return 0;tls->client_store=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,0,0,CERT_SYSTEM_STORE_CURRENT_USER,L"MY");if(!tls->client_store){ncw_error(error,error_cap,"client CertOpenStore",GetLastError());return 0;}tls->client_certificate=CertFindCertificateInStore(tls->client_store,X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,0,CERT_FIND_SUBJECT_STR_W,subject,NULL);if(!tls->client_certificate){ncw_error(error,error_cap,"client certificate subject",CRYPT_E_NOT_FOUND);ncw_schannel_client_close(tls,error,error_cap);return 0;}credentials.cCreds=1;credentials.paCred=&tls->client_certificate;}
    TimeStamp expiry;SECURITY_STATUS status=AcquireCredentialsHandleW(NULL,UNISP_NAME_W,SECPKG_CRED_OUTBOUND,NULL,&credentials,NULL,NULL,&tls->credentials,&expiry);
    if(status!=SEC_E_OK){ncw_error(error,error_cap,"AcquireCredentialsHandleW",(DWORD)status);return 0;}tls->credentials_ready=1;
    u_long nonblocking=0;if(ioctlsocket(socket->socket,FIONBIO,&nonblocking)==SOCKET_ERROR){ncw_error(error,error_cap,"TLS blocking mode",(DWORD)WSAGetLastError());ncw_schannel_client_close(tls,error,error_cap);return 0;}
    ULONG requested=ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_CONFIDENTIALITY|ISC_REQ_EXTENDED_ERROR|ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_STREAM;ULONG attributes=0;int first=1;
    for(;;){
        SecBuffer output_buffer={0,SECBUFFER_TOKEN,NULL};SecBufferDesc output={SECBUFFER_VERSION,1,&output_buffer};SecBuffer input_buffers[2];SecBufferDesc input;SecBufferDesc *input_pointer=NULL;
        if(!first){input_buffers[0]=(SecBuffer){(unsigned long)tls->encrypted_length,SECBUFFER_TOKEN,tls->encrypted};input_buffers[1]=(SecBuffer){0,SECBUFFER_EMPTY,NULL};input=(SecBufferDesc){SECBUFFER_VERSION,2,input_buffers};input_pointer=&input;}
        status=InitializeSecurityContextW(&tls->credentials,first?NULL:&tls->context,hostname,requested,0,SECURITY_NATIVE_DREP,input_pointer,0,&tls->context,&output,&attributes,&expiry);tls->context_ready=1;first=0;
        if(output_buffer.cbBuffer&&output_buffer.pvBuffer){int sent=ncw_socket_send_all(socket->socket,output_buffer.pvBuffer,output_buffer.cbBuffer,error,error_cap);FreeContextBuffer(output_buffer.pvBuffer);if(!sent)goto failure;}
        if(status==SEC_E_OK){if(input_pointer)ncw_schannel_keep_extra(tls,input_buffers,2);break;}
        if(status==SEC_I_COMPLETE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){if(CompleteAuthToken(&tls->context,&output)!=SEC_E_OK){ncw_error(error,error_cap,"CompleteAuthToken",(DWORD)status);goto failure;}if(status==SEC_I_COMPLETE_NEEDED)break;status=SEC_I_CONTINUE_NEEDED;}
        if(status==SEC_E_INCOMPLETE_MESSAGE||status==SEC_I_CONTINUE_NEEDED){if(status==SEC_I_CONTINUE_NEEDED&&input_pointer)ncw_schannel_keep_extra(tls,input_buffers,2);int received=ncw_schannel_receive(tls,socket,error,error_cap);if(received<=0){if(received==0)ncw_error(error,error_cap,"SChannel handshake",WSAECONNRESET);goto failure;}continue;}
        ncw_error(error,error_cap,"InitializeSecurityContextW",(DWORD)status);goto failure;
    }
    if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_STREAM_SIZES,&tls->sizes)!=SEC_E_OK){ncw_error(error,error_cap,"SChannel stream sizes",GetLastError());goto failure;}
    SecPkgContext_ConnectionInfo connection;memset(&connection,0,sizeof(connection));if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_CONNECTION_INFO,&connection)==SEC_E_OK){tls->protocol=connection.dwProtocol;tls->cipher=connection.aiCipher;tls->cipher_strength=connection.dwCipherStrength;}
    PCCERT_CONTEXT peer=NULL;if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_REMOTE_CERT_CONTEXT,&peer)==SEC_E_OK&&peer){tls->peer_cert_present=1;CertFreeCertificateContext(peer);}
    if(tls->protocol!=SP_PROT_TLS1_3_CLIENT){ncw_error(error,error_cap,"SChannel TLS 1.3 required",SEC_E_ALGORITHM_MISMATCH);goto failure;}
    tls->handshake_done=1;nonblocking=1;(void)ioctlsocket(socket->socket,FIONBIO,&nonblocking);return 1;
failure:
    nonblocking=1;(void)ioctlsocket(socket->socket,FIONBIO,&nonblocking);ncw_schannel_client_close(tls,error,error_cap);return 0;
}

int64_t ncw_schannel_client_write(NcwSChannelClient *tls,NcwSocket *socket,
                                  const void *data,size_t size,
                                  char *error,size_t error_cap){
    if(!tls||!tls->handshake_done||!socket||socket->socket==INVALID_SOCKET||(!data&&size)){ncw_error(error,error_cap,"SChannel write",ERROR_INVALID_PARAMETER);return -1;}size_t offset=0;
    while(offset<size){size_t payload=size-offset;if(payload>tls->sizes.cbMaximumMessage)payload=tls->sizes.cbMaximumMessage;size_t record=tls->sizes.cbHeader+payload+tls->sizes.cbTrailer;unsigned char *buffer=malloc(record);if(!buffer){ncw_error(error,error_cap,"SChannel write",ERROR_OUTOFMEMORY);return -1;}memcpy(buffer+tls->sizes.cbHeader,(const unsigned char *)data+offset,payload);
        SecBuffer buffers[4]={{tls->sizes.cbHeader,SECBUFFER_STREAM_HEADER,buffer},{(unsigned long)payload,SECBUFFER_DATA,buffer+tls->sizes.cbHeader},{tls->sizes.cbTrailer,SECBUFFER_STREAM_TRAILER,buffer+tls->sizes.cbHeader+payload},{0,SECBUFFER_EMPTY,NULL}};SecBufferDesc message={SECBUFFER_VERSION,4,buffers};SECURITY_STATUS status=EncryptMessage(&tls->context,0,&message,0);if(status!=SEC_E_OK){free(buffer);ncw_error(error,error_cap,"EncryptMessage",(DWORD)status);return -1;}size_t encrypted=(size_t)buffers[0].cbBuffer+buffers[1].cbBuffer+buffers[2].cbBuffer;int sent=ncw_socket_send_all(socket->socket,buffer,encrypted,error,error_cap);free(buffer);if(!sent)return -1;offset+=payload;}
    return (int64_t)offset;
}

int64_t ncw_schannel_client_read(NcwSChannelClient *tls,NcwSocket *socket,
                                 void *data,size_t size,
                                 char *error,size_t error_cap){
    if(!tls||!tls->handshake_done||!socket||socket->socket==INVALID_SOCKET||!data||!size){ncw_error(error,error_cap,"SChannel read",ERROR_INVALID_PARAMETER);return -1;}
    if(tls->plaintext_offset<tls->plaintext_length){size_t available=tls->plaintext_length-tls->plaintext_offset,take=available<size?available:size;memcpy(data,tls->plaintext+tls->plaintext_offset,take);tls->plaintext_offset+=take;if(tls->plaintext_offset==tls->plaintext_length)tls->plaintext_offset=tls->plaintext_length=0;return (int64_t)take;}
    for(;;){
        if(!tls->encrypted_length){int received=ncw_schannel_receive(tls,socket,error,error_cap);if(received<=0)return received;}
        SecBuffer buffers[4]={{(unsigned long)tls->encrypted_length,SECBUFFER_DATA,tls->encrypted},{0,SECBUFFER_EMPTY,NULL},{0,SECBUFFER_EMPTY,NULL},{0,SECBUFFER_EMPTY,NULL}};SecBufferDesc message={SECBUFFER_VERSION,4,buffers};SECURITY_STATUS status=DecryptMessage(&tls->context,&message,0,NULL);
        if(status==SEC_E_INCOMPLETE_MESSAGE){int received=ncw_schannel_receive(tls,socket,error,error_cap);if(received<=0)return received;continue;}if(status==SEC_I_CONTEXT_EXPIRED)return 0;if(status==SEC_I_RENEGOTIATE){ncw_error(error,error_cap,"SChannel renegotiation",(DWORD)status);return -1;}if(status!=SEC_E_OK){ncw_error(error,error_cap,"DecryptMessage",(DWORD)status);return -1;}
        unsigned char *plain=NULL;size_t plain_size=0;for(int i=0;i<4;i++)if(buffers[i].BufferType==SECBUFFER_DATA){plain=buffers[i].pvBuffer;plain_size=buffers[i].cbBuffer;break;}if(plain_size>sizeof(tls->plaintext)){ncw_error(error,error_cap,"SChannel plaintext",ERROR_BUFFER_OVERFLOW);return -1;}if(plain_size)memcpy(tls->plaintext,plain,plain_size);tls->plaintext_length=plain_size;tls->plaintext_offset=0;ncw_schannel_keep_extra(tls,buffers,4);if(plain_size)break;
    }
    return ncw_schannel_client_read(tls,socket,data,size,error,error_cap);
}

int ncw_schannel_client_close(NcwSChannelClient *tls,char *error,size_t error_cap){
    if(!tls)return 1;SECURITY_STATUS status=SEC_E_OK;if(tls->context_ready){status=DeleteSecurityContext(&tls->context);tls->context_ready=0;}if(tls->credentials_ready){SECURITY_STATUS credential_status=FreeCredentialsHandle(&tls->credentials);if(status==SEC_E_OK)status=credential_status;tls->credentials_ready=0;}if(tls->client_certificate){CertFreeCertificateContext(tls->client_certificate);tls->client_certificate=NULL;}if(tls->client_store){CertCloseStore(tls->client_store,0);tls->client_store=NULL;}tls->handshake_done=0;tls->encrypted_length=tls->plaintext_length=tls->plaintext_offset=0;if(status!=SEC_E_OK){ncw_error(error,error_cap,"SChannel close",(DWORD)status);return 0;}return 1;
}

int ncw_schannel_server_credentials(NcwSChannelServerCredential *credentials,
                                    const char *certificate_subject_utf8,
                                    int require_client_cert,
                                    char *error,size_t error_cap){
    if(!credentials||!certificate_subject_utf8||!*certificate_subject_utf8){ncw_error(error,error_cap,"SChannel server credentials",ERROR_INVALID_PARAMETER);return 0;}memset(credentials,0,sizeof(*credentials));
    wchar_t subject[256];if(!ncw_utf8(certificate_subject_utf8,subject,256,error,error_cap))return 0;credentials->store=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,0,0,CERT_SYSTEM_STORE_CURRENT_USER,L"MY");if(!credentials->store){ncw_error(error,error_cap,"CertOpenStore",GetLastError());return 0;}
    credentials->certificate=CertFindCertificateInStore(credentials->store,X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,0,CERT_FIND_SUBJECT_STR_W,subject,NULL);if(!credentials->certificate){ncw_error(error,error_cap,"server certificate subject",CRYPT_E_NOT_FOUND);ncw_schannel_server_credentials_close(credentials,error,error_cap);return 0;}
    SCHANNEL_CRED credential_data;memset(&credential_data,0,sizeof(credential_data));credential_data.dwVersion=SCHANNEL_CRED_VERSION;credential_data.cCreds=1;credential_data.paCred=&credentials->certificate;credential_data.grbitEnabledProtocols=SP_PROT_TLS1_3_SERVER;credential_data.dwFlags=SCH_USE_STRONG_CRYPTO|SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    TimeStamp expiry;SECURITY_STATUS status=AcquireCredentialsHandleW(NULL,UNISP_NAME_W,SECPKG_CRED_INBOUND,NULL,&credential_data,NULL,NULL,&credentials->credentials,&expiry);if(status!=SEC_E_OK){ncw_error(error,error_cap,"AcquireCredentialsHandleW(server)",(DWORD)status);ncw_schannel_server_credentials_close(credentials,error,error_cap);return 0;}credentials->ready=1;credentials->require_client_cert=require_client_cert;credentials->references=1;return 1;
}

static int ncw_schannel_server_receive(NcwSChannelServer *tls,NcwSocket *socket,
                                       char *error,size_t error_cap){
    if(tls->encrypted_length>=sizeof(tls->encrypted)){ncw_error(error,error_cap,"SChannel server receive buffer",ERROR_BUFFER_OVERFLOW);return -1;}int received=recv(socket->socket,(char *)tls->encrypted+tls->encrypted_length,(int)(sizeof(tls->encrypted)-tls->encrypted_length),0);if(received==0)return 0;if(received==SOCKET_ERROR){int socket_error=WSAGetLastError();if(socket_error==WSAEWOULDBLOCK)return -2;ncw_error(error,error_cap,"TLS server recv",(DWORD)socket_error);return -1;}tls->encrypted_length+=(size_t)received;return received;
}

static void ncw_schannel_server_keep_extra(NcwSChannelServer *tls,SecBuffer *buffers,size_t count){
    size_t extra=0;unsigned char *source=NULL;for(size_t i=0;i<count;i++)if(buffers[i].BufferType==SECBUFFER_EXTRA){extra=buffers[i].cbBuffer;source=buffers[i].pvBuffer;break;}if(extra&&source)memmove(tls->encrypted,source,extra);tls->encrypted_length=extra;
}

int ncw_schannel_server_handshake(NcwSChannelServer *tls,
                                  NcwSChannelServerCredential *credentials,
                                  NcwSocket *socket,char *error,size_t error_cap){
    if(!tls||!credentials||!credentials->ready||!socket||socket->socket==INVALID_SOCKET){ncw_error(error,error_cap,"SChannel server handshake",ERROR_INVALID_PARAMETER);return 0;}memset(tls,0,sizeof(*tls));u_long nonblocking=0;if(ioctlsocket(socket->socket,FIONBIO,&nonblocking)==SOCKET_ERROR){ncw_error(error,error_cap,"TLS server blocking mode",(DWORD)WSAGetLastError());return 0;}
    ULONG requested=ASC_REQ_SEQUENCE_DETECT|ASC_REQ_REPLAY_DETECT|ASC_REQ_CONFIDENTIALITY|ASC_REQ_EXTENDED_ERROR|ASC_REQ_ALLOCATE_MEMORY|ASC_REQ_STREAM;if(credentials->require_client_cert)requested|=ASC_REQ_MUTUAL_AUTH;ULONG attributes=0;TimeStamp expiry;int first=1;
    for(;;){
        if(!tls->encrypted_length){int received=ncw_schannel_server_receive(tls,socket,error,error_cap);if(received<=0){if(received==0)ncw_error(error,error_cap,"SChannel server handshake",WSAECONNRESET);goto failure;}}
        SecBuffer input_buffers[2]={{(unsigned long)tls->encrypted_length,SECBUFFER_TOKEN,tls->encrypted},{0,SECBUFFER_EMPTY,NULL}};SecBufferDesc input={SECBUFFER_VERSION,2,input_buffers};SecBuffer output_buffer={0,SECBUFFER_TOKEN,NULL};SecBufferDesc output={SECBUFFER_VERSION,1,&output_buffer};SECURITY_STATUS status=AcceptSecurityContext(&credentials->credentials,first?NULL:&tls->context,&input,requested,SECURITY_NATIVE_DREP,&tls->context,&output,&attributes,&expiry);tls->context_ready=1;first=0;
        if(output_buffer.cbBuffer&&output_buffer.pvBuffer){int sent=ncw_socket_send_all(socket->socket,output_buffer.pvBuffer,output_buffer.cbBuffer,error,error_cap);FreeContextBuffer(output_buffer.pvBuffer);if(!sent)goto failure;}
        if(status==SEC_E_OK){ncw_schannel_server_keep_extra(tls,input_buffers,2);break;}
        if(status==SEC_I_COMPLETE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){if(CompleteAuthToken(&tls->context,&output)!=SEC_E_OK){ncw_error(error,error_cap,"CompleteAuthToken(server)",(DWORD)status);goto failure;}if(status==SEC_I_COMPLETE_NEEDED)break;status=SEC_I_CONTINUE_NEEDED;}
        if(status==SEC_E_INCOMPLETE_MESSAGE||status==SEC_I_CONTINUE_NEEDED){ncw_schannel_server_keep_extra(tls,input_buffers,2);int received=ncw_schannel_server_receive(tls,socket,error,error_cap);if(received<=0){if(received==0)ncw_error(error,error_cap,"SChannel server handshake",WSAECONNRESET);goto failure;}continue;}
        ncw_error(error,error_cap,"AcceptSecurityContext",(DWORD)status);goto failure;
    }
    if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_STREAM_SIZES,&tls->sizes)!=SEC_E_OK){ncw_error(error,error_cap,"SChannel server stream sizes",GetLastError());goto failure;}SecPkgContext_ConnectionInfo connection;memset(&connection,0,sizeof(connection));if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_CONNECTION_INFO,&connection)==SEC_E_OK){tls->protocol=connection.dwProtocol;tls->cipher=connection.aiCipher;tls->cipher_strength=connection.dwCipherStrength;}
    PCCERT_CONTEXT peer=NULL;if(QueryContextAttributesW(&tls->context,SECPKG_ATTR_REMOTE_CERT_CONTEXT,&peer)==SEC_E_OK&&peer){tls->peer_cert_present=1;CertFreeCertificateContext(peer);}if(credentials->require_client_cert&&!tls->peer_cert_present){ncw_error(error,error_cap,"SChannel client certificate required",SEC_E_CERT_UNKNOWN);goto failure;}if(tls->protocol!=SP_PROT_TLS1_3_SERVER){ncw_error(error,error_cap,"SChannel TLS 1.3 required",SEC_E_ALGORITHM_MISMATCH);goto failure;}
    tls->handshake_done=1;nonblocking=1;(void)ioctlsocket(socket->socket,FIONBIO,&nonblocking);return 1;
failure:
    nonblocking=1;(void)ioctlsocket(socket->socket,FIONBIO,&nonblocking);ncw_schannel_server_close(tls,error,error_cap);return 0;
}

int64_t ncw_schannel_server_write(NcwSChannelServer *tls,NcwSocket *socket,
                                  const void *data,size_t size,char *error,size_t error_cap){
    if(!tls||!tls->handshake_done||!socket||socket->socket==INVALID_SOCKET||(!data&&size)){ncw_error(error,error_cap,"SChannel server write",ERROR_INVALID_PARAMETER);return -1;}size_t offset=0;while(offset<size){size_t payload=size-offset;if(payload>tls->sizes.cbMaximumMessage)payload=tls->sizes.cbMaximumMessage;size_t record=tls->sizes.cbHeader+payload+tls->sizes.cbTrailer;unsigned char *buffer=malloc(record);if(!buffer){ncw_error(error,error_cap,"SChannel server write",ERROR_OUTOFMEMORY);return -1;}memcpy(buffer+tls->sizes.cbHeader,(const unsigned char *)data+offset,payload);SecBuffer buffers[4]={{tls->sizes.cbHeader,SECBUFFER_STREAM_HEADER,buffer},{(unsigned long)payload,SECBUFFER_DATA,buffer+tls->sizes.cbHeader},{tls->sizes.cbTrailer,SECBUFFER_STREAM_TRAILER,buffer+tls->sizes.cbHeader+payload},{0,SECBUFFER_EMPTY,NULL}};SecBufferDesc message={SECBUFFER_VERSION,4,buffers};SECURITY_STATUS status=EncryptMessage(&tls->context,0,&message,0);if(status!=SEC_E_OK){free(buffer);ncw_error(error,error_cap,"EncryptMessage(server)",(DWORD)status);return -1;}size_t encrypted=(size_t)buffers[0].cbBuffer+buffers[1].cbBuffer+buffers[2].cbBuffer;int sent=ncw_socket_send_all(socket->socket,buffer,encrypted,error,error_cap);free(buffer);if(!sent)return -1;offset+=payload;}return (int64_t)offset;
}

int64_t ncw_schannel_server_read(NcwSChannelServer *tls,NcwSocket *socket,
                                 void *data,size_t size,char *error,size_t error_cap){
    if(!tls||!tls->handshake_done||!socket||socket->socket==INVALID_SOCKET||!data||!size){ncw_error(error,error_cap,"SChannel server read",ERROR_INVALID_PARAMETER);return -1;}if(tls->plaintext_offset<tls->plaintext_length){size_t available=tls->plaintext_length-tls->plaintext_offset,take=available<size?available:size;memcpy(data,tls->plaintext+tls->plaintext_offset,take);tls->plaintext_offset+=take;if(tls->plaintext_offset==tls->plaintext_length)tls->plaintext_offset=tls->plaintext_length=0;return (int64_t)take;}
    for(;;){if(!tls->encrypted_length){int received=ncw_schannel_server_receive(tls,socket,error,error_cap);if(received<=0)return received;}SecBuffer buffers[4]={{(unsigned long)tls->encrypted_length,SECBUFFER_DATA,tls->encrypted},{0,SECBUFFER_EMPTY,NULL},{0,SECBUFFER_EMPTY,NULL},{0,SECBUFFER_EMPTY,NULL}};SecBufferDesc message={SECBUFFER_VERSION,4,buffers};SECURITY_STATUS status=DecryptMessage(&tls->context,&message,0,NULL);if(status==SEC_E_INCOMPLETE_MESSAGE){int received=ncw_schannel_server_receive(tls,socket,error,error_cap);if(received<=0)return received;continue;}if(status==SEC_I_CONTEXT_EXPIRED)return 0;if(status==SEC_I_RENEGOTIATE){ncw_error(error,error_cap,"SChannel server renegotiation",(DWORD)status);return -1;}if(status!=SEC_E_OK){ncw_error(error,error_cap,"DecryptMessage(server)",(DWORD)status);return -1;}unsigned char *plain=NULL;size_t plain_size=0;for(int i=0;i<4;i++)if(buffers[i].BufferType==SECBUFFER_DATA){plain=buffers[i].pvBuffer;plain_size=buffers[i].cbBuffer;break;}if(plain_size>sizeof(tls->plaintext)){ncw_error(error,error_cap,"SChannel server plaintext",ERROR_BUFFER_OVERFLOW);return -1;}if(plain_size)memcpy(tls->plaintext,plain,plain_size);tls->plaintext_length=plain_size;tls->plaintext_offset=0;ncw_schannel_server_keep_extra(tls,buffers,4);if(plain_size)break;}return ncw_schannel_server_read(tls,socket,data,size,error,error_cap);
}

int ncw_schannel_server_close(NcwSChannelServer *tls,char *error,size_t error_cap){
    if(!tls)return 1;SECURITY_STATUS status=SEC_E_OK;if(tls->context_ready){status=DeleteSecurityContext(&tls->context);tls->context_ready=0;}tls->handshake_done=0;tls->encrypted_length=tls->plaintext_length=tls->plaintext_offset=0;if(status!=SEC_E_OK){ncw_error(error,error_cap,"SChannel server close",(DWORD)status);return 0;}return 1;
}

int ncw_schannel_server_credentials_close(NcwSChannelServerCredential *credentials,
                                          char *error,size_t error_cap){
    if(!credentials)return 1;SECURITY_STATUS status=SEC_E_OK;if(credentials->ready){status=FreeCredentialsHandle(&credentials->credentials);credentials->ready=0;}if(credentials->certificate){CertFreeCertificateContext(credentials->certificate);credentials->certificate=NULL;}if(credentials->store){CertCloseStore(credentials->store,0);credentials->store=NULL;}if(status!=SEC_E_OK){ncw_error(error,error_cap,"SChannel server credentials close",(DWORD)status);return 0;}return 1;
}
