#ifndef ZEND_ABSTRACT_INTERFACE_H
#define ZEND_ABSTRACT_INTERFACE_H

#include <main/php.h>
#include <stdbool.h>

bool zai_call_function(zend_string *name, zval *retval, int argc, ...);

#endif  // ZEND_ABSTRACT_INTERFACE_H
