/* ZAI SAPI is a fork the the embed SAPI:
 * https://github.com/php/php-src/blob/PHP-8.0.3/sapi/embed/php_embed.c
 *
 * The ZAI SAPI is designed for running tests for components that are tightly
 * coupled to the Zend Engine.
 *
 * The current feature wishlist includes:
 *   [x] Independent control over SAPI startup, MINIT, and RINIT
 *   [ ] Custom INI settings per process or request
 *   [ ] Custom SAPI-level functions
 *   [ ] Fuzzing
 */
#include "zai_sapi.h"

#include <Zend/zend_signal.h>
#include <main/SAPI.h>
#include <main/php_main.h>
#include <main/php_variables.h>

#define UNUSED(x) (void)(x)

static int zai_startup(sapi_module_struct *sapi_module) { return php_module_startup(sapi_module, NULL, 0); }

static int zai_deactivate(void) {
    fflush(stdout);
    return SUCCESS;
}

/* This is supposed to be the "unbuffered write" function for the SAPI so using
 * a buffered function like fwrite() here I suppose is technically incorrect.
 * But fwrite()'s buffering gives a little performance boost over write() which
 * better serves the purposes of running many tests through the ZAI SAPI.
 */
static size_t zai_ub_write(const char *str, size_t str_length) {
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

static void zai_flush(void *server_context) {
    UNUSED(server_context);
    if (fflush(stdout) == EOF) {
        php_handle_aborted_connection();
    }
}

static void zai_send_header(sapi_header_struct *sapi_header, void *server_context) {
    UNUSED(sapi_header);
    UNUSED(server_context);
}

static char *zai_read_cookies(void) { return NULL; }

static void zai_register_variables(zval *track_vars_array) { php_import_environment_variables(track_vars_array); }

#if PHP_VERSION_ID >= 80000
static void zai_log_message(const char *message, int syslog_type_int) {
    UNUSED(syslog_type_int);
    fprintf(stderr, "%s\n", message);
}
#else
static void zai_log_message(char *message) { fprintf(stderr, "%s\n", message); }
#endif

static sapi_module_struct zai_module = {"zai",                     /* name */
                                        "Zend Abstract Interface", /* pretty name */

                                        zai_startup,                 /* startup */
                                        php_module_shutdown_wrapper, /* shutdown */

                                        NULL,           /* activate */
                                        zai_deactivate, /* deactivate */

                                        zai_ub_write, /* unbuffered write */
                                        zai_flush,    /* flush */
                                        NULL,         /* get uid */
                                        NULL,         /* getenv */

                                        php_error, /* error handler */

                                        NULL,            /* header handler */
                                        NULL,            /* send headers handler */
                                        zai_send_header, /* send header handler */

                                        NULL,             /* read POST data */
                                        zai_read_cookies, /* read Cookies */

                                        zai_register_variables, /* register server variables */
                                        zai_log_message,        /* Log message */
                                        NULL,                   /* Get request time */
                                        NULL,                   /* Child terminate */

                                        STANDARD_SAPI_MODULE_PROPERTIES};

static const char default_ini[] =
    "html_errors=0\n"
    "implicit_flush=1\n"
    "output_buffering=0\n"
    "max_execution_time=0\n"
    "max_input_time=-1\n"
    "memory_limit=-1\n"
    "\0";

static void zai_sapi_ini_alloc(void) {
    zai_module.ini_entries = (char *)malloc(sizeof default_ini);
    memcpy(zai_module.ini_entries, default_ini, sizeof default_ini);
}

static void zai_sapi_ini_free(void) {
    if (zai_module.ini_entries) {
        free(zai_module.ini_entries);
        zai_module.ini_entries = NULL;
    }
}

bool zai_sapi_init_sapi(void) {
#ifdef ZTS
    php_tsrm_startup();
#endif

    zend_signal_startup();

    sapi_startup(&zai_module);

    zai_sapi_ini_alloc();

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

void zai_sapi_shutdown_sapi(void) {
    sapi_shutdown();
#ifdef ZTS
    tsrm_shutdown();
#endif
    zai_sapi_ini_free();
}

bool zai_sapi_init_modules(void) {
    if (zai_module.startup(&zai_module) == FAILURE) {
        zai_sapi_shutdown_sapi();
        return false;
    }
    return true;
}

void zai_sapi_shutdown_modules(void) { php_module_shutdown(); }

bool zai_sapi_init_request(void) {
    /* Do not chdir to the script's directory (equivalent to running the CLI
     * SAPI with '-C').
     */
    SG(options) |= SAPI_OPTION_NO_CHDIR;

    if (php_request_startup() == FAILURE) {
        zai_sapi_shutdown_modules();
        zai_sapi_shutdown_sapi();
        return false;
    }

    SG(headers_sent) = 1;
    SG(request_info).no_headers = 1;
    php_register_variable("PHP_SELF", "-", NULL);

    return true;
}

void zai_sapi_shutdown_request(void) { php_request_shutdown((void *)0); }

bool zai_sapi_spinup(void) { return zai_sapi_init_sapi() && zai_sapi_init_modules() && zai_sapi_init_request(); }

void zai_sapi_spindown(void) {
    zai_sapi_shutdown_request();
    zai_sapi_shutdown_modules();
    zai_sapi_shutdown_sapi();
}
