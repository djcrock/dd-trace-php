extern "C" {
#include "functions/functions.h"

#include "zai_sapi/zai_sapi.h"
}

#include <catch2/catch.hpp>

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
    REQUIRE(zai_sapi_spinup());

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

    zai_sapi_spindown();
}

TEST_CASE("call userland functions with the embed SAPI", "[zai]") {
    REQUIRE(zai_sapi_spinup());

    zend_first_try {
        if (!zai_test_execute_script("./stubs/basic.php")) {
            zai_sapi_spindown();
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

    zai_sapi_spindown();
}

/* The 'disable_functions' INI setting only disables internal functions.
 * https://www.php.net/manual/en/ini.core.php#ini.disable-functions
 */
TEST_CASE("call a function that has been disabled with disable_functions", "[zai]") {
    REQUIRE(zai_sapi_init_sapi());

    REQUIRE(zai_sapi_append_system_ini_entry("disable_functions", "mt_rand"));

    REQUIRE(zai_sapi_init_modules());
    REQUIRE(zai_sapi_init_request());

    zend_first_try {
        zend_string *name = zend_string_init(ZEND_STRL("mt_rand"), 0);
        zval retval;

        bool result = zai_call_function(name, &retval, 0);

        REQUIRE(result == false);
        REQUIRE(Z_TYPE(retval) == IS_UNDEF);

        zend_string_release(name);
    }
    zend_end_try();

    zai_sapi_shutdown_request();
    zai_sapi_shutdown_modules();
    zai_sapi_shutdown_sapi();
}

TEST_CASE("call function error cases", "[zai]") {
    REQUIRE(zai_sapi_spinup());

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

    zai_sapi_spindown();
}
