#ifndef ZAI_FUNCTIONS_H
#define ZAI_FUNCTIONS_H

#include <main/php.h>
#include <stdbool.h>

#if PHP_VERSION_ID >= 70000
bool zai_call_function(zend_string *name, zval *retval, int argc, ...);
#endif

#endif  // ZAI_FUNCTIONS_H
