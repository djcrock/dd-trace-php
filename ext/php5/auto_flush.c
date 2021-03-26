#include "auto_flush.h"

#include "engine_hooks.h"  // for ddtrace_backup_error_handling

ZEND_RESULT_CODE ddtrace_flush_tracer(TSRMLS_D) {
    bool success = true;

    zval *exception = NULL, *prev_exception = NULL;
    if (EG(exception)) {
        exception = EG(exception);
        EG(exception) = NULL;
        prev_exception = EG(prev_exception);
        EG(prev_exception) = NULL;
        zend_clear_exception();
    }

    ddtrace_error_handling eh;
    ddtrace_backup_error_handling(&eh, EH_SUPPRESS TSRMLS_CC);

    zend_bool orig_disable = DDTRACE_G(disable_in_current_request);
    DDTRACE_G(disable_in_current_request) = 1;

    // $tracer = \DDTrace\GlobalTracer::get();
    // $tracer->flush();
    // $tracer->reset();
    php_printf("[Placeholder] Flushing...\n");

    DDTRACE_G(disable_in_current_request) = orig_disable;

    ddtrace_restore_error_handling(&eh);
    ddtrace_maybe_clear_exception();
    if (exception) {
        EG(exception) = exception;
        EG(prev_exception) = prev_exception;
        // TODO PHP 5 version of zend_throw_exception_internal(NULL);
    }

    return success ? SUCCESS : FAILURE;
}
