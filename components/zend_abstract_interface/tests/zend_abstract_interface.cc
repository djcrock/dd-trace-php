extern "C" {
#include "zend_abstract_interface/zend_abstract_interface.h"
}

#include <sapi/embed/php_embed.h>

#include <catch2/catch.hpp>

static const char default_ini[] =
    "html_errors=0\n"
    "implicit_flush=1\n"
    "output_buffering=0\n"
    "max_execution_time=0\n"
    "max_input_time=-1\n"
    "memory_limit=-1\n"
    "\0";

static void zai_test_ini_alloc(void) {
    php_embed_module.ini_entries = (char *)malloc(sizeof default_ini);
    memcpy(php_embed_module.ini_entries, default_ini, sizeof default_ini);
}

static void zai_test_ini_free(void) {
    if (php_embed_module.ini_entries) {
        free(php_embed_module.ini_entries);
        php_embed_module.ini_entries = NULL;
    }
}

/* We have our own version of php_embed_init() for the following reasons:
 * 1) We are able to provide our own INI entries.
 * 2) We are able to provide our own SAPI functions.
 * https://github.com/php/php-src/blob/PHP-8.0.3/sapi/embed/php_embed.c#L151-L212
 */
static bool zai_test_embed_init(void) {
#ifdef ZTS
    php_tsrm_startup();
#endif

    zend_signal_startup();

    sapi_startup(&php_embed_module);

    zai_test_ini_alloc();

    /* Don't load any INI files (equivalent to running the CLI SAPI with '-n').
     * This will prevent inadvertently loading any shared-library extensions
     * that are not built with ASan. It also gives us a consistent clean slate
     * of INI settings.
     */
    php_embed_module.php_ini_ignore = 1;

    /* TODO: When we want to expose a function to userland for testing purposes
     * (e.g. DDTrace\Testing\trigger_error()), we can add them as custom SAPI
     * functions here. These functions will only exist in the embed SAPI for
     * testing at the C unit test level and will not be shipped as a public
     * userland API in the PHP tracer.
     */
    // php_embed_module.additional_functions = additional_functions;

    if (php_embed_module.startup(&php_embed_module) == FAILURE) {
        zai_test_ini_free();
        return false;
    }

    zend_llist global_vars;
    zend_llist_init(&global_vars, sizeof(char *), NULL, 0);

    /* Do not chdir to the script's directory (equivalent to running the CLI
     * SAPI with '-C').
     */
    SG(options) |= SAPI_OPTION_NO_CHDIR;

    if (php_request_startup() == FAILURE) {
        php_module_shutdown();
        zai_test_ini_free();
        return false;
    }

    SG(headers_sent) = 1;
    SG(request_info).no_headers = 1;
    php_register_variable("PHP_SELF", "-", NULL);

    return true;
}

/* Based on php_embed_shutdown()
 * https://github.com/php/php-src/blob/PHP-8.0.3/sapi/embed/php_embed.c#L214-L226
 */
static void zai_test_embed_shutdown(void) {
    php_request_shutdown((void *)0);
    php_module_shutdown();
    sapi_shutdown();
#ifdef ZTS
    tsrm_shutdown();
#endif
    zai_test_ini_free();
}

static bool zai_test_execute_script(const char *file) {
    bool result = false;
    zend_file_handle file_handle;

    zend_try {
        zend_stream_init_filename(&file_handle, file);
        result = zend_execute_scripts(ZEND_REQUIRE, NULL, 1, &file_handle) == SUCCESS;
    }
    zend_end_try();

    return result;
}

TEST_CASE("call internal functions with the embed SAPI", "[zai]") {
    if (!zai_test_embed_init()) REQUIRE(false);
    zend_first_try {
        SECTION("call an internal PHP function") {
            zend_string *name = zend_string_init(ZEND_STRL("mt_rand"), 0);
            zval retval;
            bool result = zai_call_function(name, &retval, 0);

            REQUIRE(result == true);
            REQUIRE(Z_TYPE(retval) == IS_LONG);

            zend_string_release(name);
        }

        SECTION("call a function that does not exist") {
            zend_string *name = zend_string_init(ZEND_STRL("i_do_not_exist"), 0);
            zval retval;
            bool result = zai_call_function(name, &retval, 0);

            REQUIRE(result == false);
            REQUIRE(Z_TYPE(retval) == IS_UNDEF);

            zend_string_release(name);
        }
    }
    zend_end_try();
    zai_test_embed_shutdown();
}

TEST_CASE("call userland functions with the embed SAPI", "[zai]") {
    if (!zai_test_embed_init()) REQUIRE(false);
    zend_first_try {
        if (!zai_test_execute_script("./stubs/basic.php")) {
            zai_test_embed_shutdown();
            REQUIRE(false);
        }

        SECTION("call an userland PHP function") {
            zend_string *name = zend_string_init(ZEND_STRL("zendabstractinterface\\returns_true"), 0);
            zval retval;
            bool result = zai_call_function(name, &retval, 0);

            REQUIRE(result == true);
            REQUIRE(Z_TYPE(retval) == IS_TRUE);

            zend_string_release(name);
        }

        SECTION("call a function that does not exist") {
            zend_string *name = zend_string_init(ZEND_STRL("zendabstractinterface\\i_do_not_exist"), 0);
            zval retval;
            bool result = zai_call_function(name, &retval, 0);

            REQUIRE(result == false);
            REQUIRE(Z_TYPE(retval) == IS_UNDEF);

            zend_string_release(name);
        }
    }
    zend_end_try();
    zai_test_embed_shutdown();
}

TEST_CASE("call function error cases", "[zai]") {
    if (!zai_test_embed_init()) REQUIRE(false);
    zend_first_try {
        SECTION("NULL function name") {
            zval retval;
            bool result = zai_call_function(NULL, &retval, 0);

            REQUIRE(result == false);
        }

        SECTION("NULL return value") {
            zend_string *name = zend_string_init(ZEND_STRL("foo_function"), 0);
            bool result = zai_call_function(name, NULL, 0);

            REQUIRE(result == false);

            zend_string_release(name);
        }
    }
    zend_end_try();
    zai_test_embed_shutdown();
}
