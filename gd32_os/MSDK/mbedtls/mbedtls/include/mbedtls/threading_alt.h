#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#include "wrapper_os.h"

typedef struct mbedtls_threading_mutex_t {
    os_mutex_t mutex;
    char is_valid;
} mbedtls_threading_mutex_t;

#endif /* threading_alt.h */
