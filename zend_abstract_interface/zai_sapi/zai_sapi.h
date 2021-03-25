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

/* SINIT: SAPI initialization
 * Characterized by 'sapi_startup()'.
 *   - Initialize SAPI globals
 *   - Allocate SAPI INI settings
 *   - Set startup behavior
 *   - Start up ZTS & signals
 */
bool zai_sapi_sinit(void);
void zai_sapi_sshutdown(void);

/* MINIT: Module initialization
 * Basically a wrapper for 'php_module_startup()'.
 */
bool zai_sapi_minit(void);
void zai_sapi_mshutdown(void);

/* RINIT: Request initialization.
 * Characterized by 'php_request_startup()'.
 *   - Set late-stage configuration
 *   - Send headers
 */
bool zai_sapi_rinit(void);
void zai_sapi_rshutdown(void);

/* Appends a ZEND_INI_SYSTEM entry to the existing settings.
 *
 *   zai_sapi_append_system_ini_entry("extension", "ddtrace.so");
 *
 * This must be called:
 * - After SAPI initialization 'zai_sapi_sinit()'
 * - Before module initialization 'zai_sapi_minit()'
 */
bool zai_sapi_append_system_ini_entry(const char *key, const char *value);

#endif  // ZAI_SAPI_H
