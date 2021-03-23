#include "functions.h"

#if PHP_VERSION_ID >= 70000
bool zai_call_function(zend_string *name, zval *retval, int argc, ...) {
    zend_fcall_info fci = {
        .size = sizeof(zend_fcall_info),
    };
    zend_fcall_info_cache fcc = {0};

    if (name == NULL || retval == NULL) {
        return false;
    }

    va_list argv;
    va_start(argv, argc);
    zend_fcall_info_argv(&fci, (uint32_t)argc, &argv);
    va_end(argv);

    zval fname;
    ZVAL_STR(&fname, name);
    zend_bool is_callable = zend_is_callable_ex(&fname, NULL, IS_CALLABLE_CHECK_SILENT, NULL, &fcc, NULL);

    /* Given that fname is always a string, this path is only possible if the
     * function does not exist.
     */
    if (UNEXPECTED(!is_callable)) {
        /* zend_call_function undef's the retval; as a wrapper for it, this
         * func should have the same invariant; a sigsegv occurred because of
         * this.
         */
        ZVAL_UNDEF(retval);
        zend_fcall_info_args_clear(&fci, 1);
        return false;
    }

    fci.retval = retval;
    ZEND_RESULT_CODE result = zend_call_function(&fci, &fcc);

    zend_fcall_info_args_clear(&fci, 1);

    return result == SUCCESS;
}
#endif
