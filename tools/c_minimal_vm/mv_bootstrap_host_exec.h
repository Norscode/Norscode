#ifndef MV_BOOTSTRAP_HOST_EXEC_H
#define MV_BOOTSTRAP_HOST_EXEC_H

/* Run ELF at path; write exit code to *exit_code. Returns 0 on success, -1 on error. */
int mv_bootstrap_host_exec_path(const char *path, int *exit_code);

#endif
