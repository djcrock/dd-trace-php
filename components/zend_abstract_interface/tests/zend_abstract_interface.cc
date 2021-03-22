extern "C" {
#include "zend_abstract_interface/zend_abstract_interface.h"
}

#include <catch2/catch.hpp>
#include <sapi/embed/php_embed.h>

static void configure_embed_sapi(void) {
    /* Don't load any INI files (equivalent to running the CLI SAPI with '-n').
     * This will prevent inadvertently loading any shared-library extensions
     * that are not built with ASan. It also gives us a consistent clean slate
     * of INI settings.
     */
    php_embed_module.php_ini_ignore = 1;
}

TEST_CASE("call internal functions with the embed SAPI", "[zai]") {
    configure_embed_sapi();
    PHP_EMBED_START_BLOCK(0, NULL)

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

    PHP_EMBED_END_BLOCK()
}

TEST_CASE("call userland functions with the embed SAPI", "[zai]") {
    configure_embed_sapi();
    PHP_EMBED_START_BLOCK(0, NULL)

    zend_file_handle file_handle;
    zend_stream_init_filename(&file_handle, "./stubs/basic.php");

    if (php_execute_script(&file_handle) == FAILURE) {
        php_embed_shutdown();
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

    PHP_EMBED_END_BLOCK()
}

TEST_CASE("call function error cases", "[zai]") {
    configure_embed_sapi();
    PHP_EMBED_START_BLOCK(0, NULL)

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

    PHP_EMBED_END_BLOCK()
}
