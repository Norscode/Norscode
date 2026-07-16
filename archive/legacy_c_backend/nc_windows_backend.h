#ifndef NORSCODE_WINDOWS_BACKEND_H
#define NORSCODE_WINDOWS_BACKEND_H

#if !defined(_WIN32)
#error "nc_windows_backend.h requires _WIN32"
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <stdint.h>
#include <stddef.h>

#define NCW_PATH_CAP 32768
#define NCW_ERROR_CAP 512

typedef struct {
    HANDLE handle;
    HANDLE async_handle;
    wchar_t root[NCW_PATH_CAP];
    wchar_t path[NCW_PATH_CAP];
    int readable;
    int writable;
} NcwFile;

int ncw_file_open(NcwFile *file, const char *root_utf8, const char *relative_utf8,
                  const char *mode, char *error, size_t error_cap);
int64_t ncw_file_read(NcwFile *file, void *buffer, size_t size,
                      char *error, size_t error_cap);
int64_t ncw_file_write(NcwFile *file, const void *buffer, size_t size,
                       char *error, size_t error_cap);
int64_t ncw_file_seek(NcwFile *file, int64_t offset,
                      char *error, size_t error_cap);
int64_t ncw_file_size(NcwFile *file, char *error, size_t error_cap);
int ncw_file_flush(NcwFile *file, char *error, size_t error_cap);
int ncw_file_close(NcwFile *file, char *error, size_t error_cap);
int ncw_file_delete(const char *root_utf8, const char *relative_utf8,
                    char *error, size_t error_cap);

typedef struct {
    HANDLE process;
    HANDLE thread;
    HANDLE job;
    HANDLE stdin_write;
    HANDLE stdout_read;
    HANDLE stderr_read;
    DWORD pid;
    DWORD exit_code;
    ULONGLONG deadline_ms;
    int exited;
    int timed_out;
    int appcontainer;
} NcwProcess;

int ncw_process_spawn(NcwProcess *process, const char *executable_utf8,
                      const char *const *argv_utf8, size_t argc,
                      const char *cwd_utf8, const void *stdin_data, size_t stdin_size,
                      uint64_t timeout_ms, uint64_t max_memory_bytes,
                      const char *sandbox_profile,
                      char *error, size_t error_cap);
int64_t ncw_process_read(NcwProcess *process, int stderr_stream,
                         void *buffer, size_t size, char *error, size_t error_cap);
int ncw_process_poll(NcwProcess *process, char *error, size_t error_cap);
int ncw_process_wait(NcwProcess *process, uint64_t wait_ms,
                     char *error, size_t error_cap);
int ncw_process_terminate(NcwProcess *process, DWORD exit_code,
                          char *error, size_t error_cap);
int ncw_process_close(NcwProcess *process, char *error, size_t error_cap);

typedef struct { HANDLE port; } NcwIocp;
typedef struct { SOCKET socket; } NcwSocket;
typedef struct {
    OVERLAPPED overlapped;
    WSABUF buffer;
    NcwSocket *socket;
    NcwSocket *listener;
    DWORD transferred;
    DWORD error_code;
    int kind;
    HANDLE owner_handle;
    char address_buffer[(sizeof(struct sockaddr_in) + 16) * 2];
} NcwIoOperation;

int ncw_file_read_async(NcwIocp *iocp, NcwFile *file, uint64_t offset,
                        void *buffer, size_t size, NcwIoOperation *operation,
                        char *error, size_t error_cap);
int ncw_file_write_async(NcwIocp *iocp, NcwFile *file, uint64_t offset,
                         const void *buffer, size_t size, NcwIoOperation *operation,
                         char *error, size_t error_cap);

int ncw_iocp_open(NcwIocp *iocp, char *error, size_t error_cap);
int ncw_iocp_close(NcwIocp *iocp, char *error, size_t error_cap);
int ncw_socket_listen(NcwIocp *iocp, NcwSocket *listener,
                      const char *host, uint16_t port, uint16_t *actual_port,
                      char *error, size_t error_cap);
int ncw_socket_accept_async(NcwIocp *iocp, NcwSocket *listener,
                            NcwSocket *accepted, NcwIoOperation *operation,
                            char *error, size_t error_cap);
int ncw_socket_connect_async(NcwIocp *iocp, NcwSocket *socket,
                             const char *host, uint16_t port,
                             NcwIoOperation *operation,
                             char *error, size_t error_cap);
int ncw_socket_read_async(NcwSocket *socket, void *buffer, size_t size,
                          NcwIoOperation *operation, char *error, size_t error_cap);
int ncw_socket_write_async(NcwSocket *socket, const void *buffer, size_t size,
                           NcwIoOperation *operation, char *error, size_t error_cap);
int ncw_iocp_wait(NcwIocp *iocp, NcwIoOperation **completed, uint32_t timeout_ms,
                  char *error, size_t error_cap);
int ncw_socket_close(NcwSocket *socket, char *error, size_t error_cap);

#define NCW_TLS_BUFFER_CAP (1024 * 1024)
typedef struct {
    CredHandle credentials;
    CtxtHandle context;
    SecPkgContext_StreamSizes sizes;
    unsigned char encrypted[NCW_TLS_BUFFER_CAP];
    size_t encrypted_length;
    unsigned char plaintext[NCW_TLS_BUFFER_CAP];
    size_t plaintext_offset;
    size_t plaintext_length;
    int credentials_ready;
    int context_ready;
    int handshake_done;
    DWORD protocol;
    ALG_ID cipher;
    DWORD cipher_strength;
    int peer_cert_present;
    HCERTSTORE client_store;
    PCCERT_CONTEXT client_certificate;
} NcwSChannelClient;

int ncw_schannel_client_handshake(NcwSChannelClient *tls, NcwSocket *socket,
                                  const char *hostname_utf8,
                                  const char *client_certificate_subject_utf8,
                                  char *error, size_t error_cap);
int64_t ncw_schannel_client_write(NcwSChannelClient *tls, NcwSocket *socket,
                                  const void *data, size_t size,
                                  char *error, size_t error_cap);
int64_t ncw_schannel_client_read(NcwSChannelClient *tls, NcwSocket *socket,
                                 void *data, size_t size,
                                 char *error, size_t error_cap);
int ncw_schannel_client_close(NcwSChannelClient *tls,
                              char *error, size_t error_cap);

typedef struct {
    CredHandle credentials;
    HCERTSTORE store;
    PCCERT_CONTEXT certificate;
    int ready;
    int require_client_cert;
    volatile LONG references;
} NcwSChannelServerCredential;

typedef struct {
    CtxtHandle context;
    SecPkgContext_StreamSizes sizes;
    unsigned char encrypted[NCW_TLS_BUFFER_CAP];
    size_t encrypted_length;
    unsigned char plaintext[NCW_TLS_BUFFER_CAP];
    size_t plaintext_offset;
    size_t plaintext_length;
    int context_ready;
    int handshake_done;
    DWORD protocol;
    ALG_ID cipher;
    DWORD cipher_strength;
    int peer_cert_present;
} NcwSChannelServer;

int ncw_schannel_server_credentials(NcwSChannelServerCredential *credentials,
                                    const char *certificate_subject_utf8,
                                    int require_client_cert,
                                    char *error, size_t error_cap);
int ncw_schannel_server_handshake(NcwSChannelServer *tls,
                                  NcwSChannelServerCredential *credentials,
                                  NcwSocket *socket,
                                  char *error, size_t error_cap);
int64_t ncw_schannel_server_write(NcwSChannelServer *tls, NcwSocket *socket,
                                  const void *data, size_t size,
                                  char *error, size_t error_cap);
int64_t ncw_schannel_server_read(NcwSChannelServer *tls, NcwSocket *socket,
                                 void *data, size_t size,
                                 char *error, size_t error_cap);
int ncw_schannel_server_close(NcwSChannelServer *tls,
                              char *error, size_t error_cap);
int ncw_schannel_server_credentials_close(NcwSChannelServerCredential *credentials,
                                          char *error, size_t error_cap);

#endif
