/* Dual embedded NCBB: native codegen bundle + bootstrap compiler bundle. */
#ifndef NCBB_DUAL_EMBEDDED_H
#define NCBB_DUAL_EMBEDDED_H

#include <stddef.h>
#include <stdint.h>

extern const uint8_t norcode_ncbb_data[];
extern const size_t norcode_ncbb_data_size;

extern const uint8_t norcode_bootstrap_ncbb_data[];
extern const size_t norcode_bootstrap_ncbb_data_size;

#endif
