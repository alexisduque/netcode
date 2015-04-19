#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "netcode/c/detail/types.hh"
#else
#include <stdint.h>
#endif

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/

#ifndef __cplusplus
/// @brief The type that lets user transfer data to the encoder.
typedef struct ntc_data_t ntc_data_t;
#endif

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data(uint16_t sz);

/*------------------------------------------------------------------------------------------------*/

ntc_data_t*
ntc_new_data_copy(const char* src, uint16_t sz);

/*------------------------------------------------------------------------------------------------*/

void
ntc_delete_data(ntc_data_t* d);

/*------------------------------------------------------------------------------------------------*/

char*
ntc_data_buffer(ntc_data_t* d);

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_set_used_bytes(ntc_data_t* d, uint16_t sz);

/*------------------------------------------------------------------------------------------------*/

void
ntc_data_reset(ntc_data_t* d, uint16_t sz);

/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
} // extern "C"
#endif
