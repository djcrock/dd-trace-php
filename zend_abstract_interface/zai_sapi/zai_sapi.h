#ifndef ZAI_SAPI_H
#define ZAI_SAPI_H

#include <main/php.h>
#include <stdbool.h>

#if PHP_VERSION_ID >= 70000 && defined(ZTS)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

/* "Spinup" will init the SAPI, modules, and request all in one go. */
bool zai_sapi_spinup(void);
/* "Spindown" will shut down the request, modules, and SAPI all in one go. */
void zai_sapi_spindown(void);

bool zai_sapi_init_sapi(void);
void zai_sapi_shutdown_sapi(void);

bool zai_sapi_init_modules(void);
void zai_sapi_shutdown_modules(void);

bool zai_sapi_init_request(void);
void zai_sapi_shutdown_request(void);

#endif  // ZAI_SAPI_H
