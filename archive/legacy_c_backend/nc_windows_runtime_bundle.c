// Single-translation-unit Windows release build.
// Keeping the generated runtime sources in one C input avoids a Zig
// CacheCheckFailed regression seen when several generated C inputs are passed
// to zig cc on windows-latest.
#include "../../build/v3009/native_candidate_gc.c"
#include "nc_windows_backend.c"
