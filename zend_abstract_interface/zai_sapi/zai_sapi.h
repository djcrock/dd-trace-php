#ifndef ZAI_SAPI_H
#define ZAI_SAPI_H

#include <main/php.h>
#include <stdbool.h>

#ifdef ZTS
ZEND_TSRMLS_CACHE_EXTERN()
#endif

bool zai_sapi_spinup(void);
void zai_sapi_spindown(void);

#endif  // ZAI_SAPI_H
