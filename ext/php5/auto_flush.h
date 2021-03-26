#ifndef DDTRACE_AUTO_FLUSH_H
#define DDTRACE_AUTO_FLUSH_H

#include <php.h>

#if PHP_VERSION_ID < 50500
#include "compatibility.h"  // For 'ZEND_RESULT_CODE'
#endif

ZEND_RESULT_CODE ddtrace_flush_tracer(TSRMLS_D);

#endif  // DDTRACE_AUTO_FLUSH_H
