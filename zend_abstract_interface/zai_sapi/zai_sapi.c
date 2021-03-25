/* ZAI SAPI is a fork the the embed SAPI:
 * https://github.com/php/php-src/blob/PHP-8.0.3/sapi/embed/php_embed.c
 *
 * The ZAI SAPI is designed for running tests for components that are tightly
 * coupled to the Zend Engine.
 *
 * The current feature wishlist includes:
 *   [x] Independent control over SAPI startup, MINIT, and RINIT
 *   [x] Custom INI settings per process
 *   [ ] Custom INI settings per request
 *   [ ] Custom SAPI-level functions
 *   [ ] Fuzzing
 */
#include "zai_sapi.h"

#include <Zend/zend_signal.h>
#include <main/SAPI.h>
#include <main/php_main.h>
#include <main/php_variables.h>

#define UNUSED(x) (void)(x)

static int zs_startup(sapi_module_struct *sapi_module) { return php_module_startup(sapi_module, NULL, 0); }

static int zs_deactivate(void) {
    fflush(stdout);
    return SUCCESS;
}

/* This is supposed to be the "unbuffered write" function for the SAPI so using
 * a buffered function like fwrite() here I suppose is technically incorrect.
 * But fwrite()'s buffering gives a little performance boost over write() which
 * better serves the purposes of running many tests through the ZAI SAPI.
 */
static size_t zs_ub_write(const char *str, size_t str_length) {
    const char *ptr = str;
    size_t remaining = str_length;
    size_t ret;

    while (remaining > 0) {
        ret = fwrite(ptr, 1, MIN(remaining, BUFSIZ), stdout);
        if (!ret) {
            php_handle_aborted_connection();
        }
        ptr += ret;
        remaining -= ret;
    }

    return str_length;
}

static void zs_flush(void *server_context) {
    UNUSED(server_context);
    if (fflush(stdout) == EOF) {
        php_handle_aborted_connection();
    }
}

static void zs_send_header(sapi_header_struct *sapi_header, void *server_context) {
    UNUSED(sapi_header);
    UNUSED(server_context);
}

static char *zs_read_cookies(void) { return NULL; }

static void zs_register_variables(zval *track_vars_array) { php_import_environment_variables(track_vars_array); }

#if PHP_VERSION_ID >= 80000
static void zs_log_message(const char *message, int syslog_type_int) {
    UNUSED(syslog_type_int);
    fprintf(stderr, "%s\n", message);
}
#else
static void zs_log_message(char *message) { fprintf(stderr, "%s\n", message); }
#endif

static sapi_module_struct zai_module = {"zai",                     /* name */
                                        "Zend Abstract Interface", /* pretty name */

                                        zs_startup,                  /* startup */
                                        php_module_shutdown_wrapper, /* shutdown */

                                        NULL,          /* activate */
                                        zs_deactivate, /* deactivate */

                                        zs_ub_write, /* unbuffered write */
                                        zs_flush,    /* flush */
                                        NULL,        /* get uid */
                                        NULL,        /* getenv */

                                        php_error, /* error handler */

                                        NULL,           /* header handler */
                                        NULL,           /* send headers handler */
                                        zs_send_header, /* send header handler */

                                        NULL,            /* read POST data */
                                        zs_read_cookies, /* read Cookies */

                                        zs_register_variables, /* register server variables */
                                        zs_log_message,        /* Log message */
                                        NULL,                  /* Get request time */
                                        NULL,                  /* Child terminate */

                                        STANDARD_SAPI_MODULE_PROPERTIES};

static const char default_ini[] =
    "html_errors=0\n"
    "implicit_flush=1\n"
    "output_buffering=0\n"
    "max_execution_time=0\n"
    "max_input_time=-1\n"
    "memory_limit=-1\n"
    "\0";

static size_t ini_entries_len = 0;

// TODO Alloc memory with an arena
static size_t zs_default_ini_alloc(char **ini_entries) {
    size_t len = sizeof default_ini - 1;

    *ini_entries = (char *)malloc(len + 1);
    if (*ini_entries == NULL) {
        return 0;
    }
    memcpy(*ini_entries, default_ini, len + 1);

    return len;
}

static void zs_ini_free(char **ini_entries) {
    if (*ini_entries) {
        free(*ini_entries);
        *ini_entries = NULL;
    }
}

static size_t zs_ini_entries_realloc_append(char **ini_entries, size_t entries_len, const char *key,
                                            const char *value) {
    size_t len;

    if (*ini_entries == NULL) {
        return 0;
    }

    len = strlen(key) + strlen(value) + (sizeof("=\n") - 1);

    char *new_entries = (char *)realloc(*ini_entries, entries_len + len + 1);
    if (new_entries == NULL) {
        return 0;
    }

    *ini_entries = new_entries;

    char *ptr = new_entries + entries_len - 1;
    int written_len = snprintf(ptr, len + 1, "%s=%s\n", key, value);
    return (written_len != len) ? 0 : entries_len + len;
}

/* Ideally to modify INI settings we would take advantage of PHP's
 * zend_alter_ini_entry_*() API here, but unfortunately for PHP_INI_SYSTEM
 * settings, we cannot. The 'configuration_hash' hash table is allocated in
 * php_init_config() and the SAPI 'ini_entries' are parsed and added to the
 * hash table. This happens during php_module_startup() so if we want to set an
 * INI setting that is used very early in the startup process (i.e.
 * 'disable_functions' which is read as part of module startup), then we must
 * reallocate our C string of 'ini_entries' and append to it before module
 * startup occurs.
 */
bool zai_sapi_append_system_ini_entry(const char *key, const char *value) {
    size_t len = zs_ini_entries_realloc_append(&zai_module.ini_entries, ini_entries_len, key, value);
    if (len <= ini_entries_len) {
        /* Play it safe and free if writing failed. */
        zs_ini_free(&zai_module.ini_entries);
        return false;
    }
    ini_entries_len = len;
    return true;
}

bool zai_sapi_sinit(void) {
#ifdef ZTS
    php_tsrm_startup();
#endif

#if PHP_VERSION_ID >= 70000 && defined(ZEND_SIGNALS)
    zend_signal_startup();
#endif

    /* Initialize the SAPI globals (memset to '0'), and set up reentrancy. */
    sapi_startup(&zai_module);

    /* Do not chdir to the script's directory (equivalent to running the CLI
     * SAPI with '-C').
     */
    SG(options) |= SAPI_OPTION_NO_CHDIR;

    /* Allocate the initial SAPI INI settings. Append new INI settings to this
     * with zai_sapi_append_system_ini_entry() before MINIT is run.
     */
    ini_entries_len = zs_default_ini_alloc(&zai_module.ini_entries);
    if (ini_entries_len == 0) {
        return false;
    }

    /* Don't load any INI files (equivalent to running the CLI SAPI with '-n').
     * This will prevent inadvertently loading any shared-library extensions
     * that are not built with ASan. It also gives us a consistent clean slate
     * of INI settings.
     */
    zai_module.php_ini_ignore = 1;

    /* Show phpinfo()/module info as plain text. */
    zai_module.phpinfo_as_text = 1;

    /* TODO: When we want to expose a function to userland for testing purposes
     * (e.g. DDTrace\Testing\trigger_error()), we can add them as custom SAPI
     * functions here. These functions will only exist in the ZAI SAPI for
     * testing at the C unit test level and will not be shipped as a public
     * userland API in the PHP tracer.
     */
    // zai_module.additional_functions = additional_functions;

    return true;
}

void zai_sapi_sshutdown(void) {
    sapi_shutdown();
#ifdef ZTS
    tsrm_shutdown();
#endif
    zs_ini_free(&zai_module.ini_entries);
}

bool zai_sapi_minit(void) {
    if (zai_module.startup(&zai_module) == FAILURE) {
        zai_sapi_sshutdown();
        return false;
    }
    return true;
}

void zai_sapi_mshutdown(void) { php_module_shutdown(); }

bool zai_sapi_rinit(void) {
    if (php_request_startup() == FAILURE) {
        return false;
    }

    SG(headers_sent) = 1;
    SG(request_info).no_headers = 1;
    php_register_variable("PHP_SELF", "-", NULL);

    return true;
}

void zai_sapi_rshutdown(void) { php_request_shutdown((void *)0); }

bool zai_sapi_spinup(void) { return zai_sapi_sinit() && zai_sapi_minit() && zai_sapi_rinit(); }

void zai_sapi_spindown(void) {
    zai_sapi_rshutdown();
    zai_sapi_mshutdown();
    zai_sapi_sshutdown();
}
