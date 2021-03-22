add_library(zai_zend_abstract_interface zend_abstract_interface.c)

target_include_directories(zai_zend_abstract_interface
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(zai_zend_abstract_interface
  PUBLIC c_std_99
)

set_target_properties(zai_zend_abstract_interface PROPERTIES
  EXPORT_NAME ZendAbstractInterface
  VERSION ${PROJECT_VERSION}
)

find_library(PHP_LIB php
  PATHS "${PHP_PREFIX_PATH}/lib"
  NO_DEFAULT_PATH
  REQUIRED # Requires cmake v3.18+
)

include_directories(
  "${PHP_PREFIX_PATH}/include/php/"
  "${PHP_PREFIX_PATH}/include/php/TSRM"
  "${PHP_PREFIX_PATH}/include/php/Zend"
  "${PHP_PREFIX_PATH}/include/php/ext"
  "${PHP_PREFIX_PATH}/include/php/main"
  "${PHP_PREFIX_PATH}/include/php/sapi"
)

target_link_libraries(zai_zend_abstract_interface
  PUBLIC "${PHP_LIB}"
)

add_library(Zai::ZendAbstractInterface
  ALIAS zai_zend_abstract_interface
)

if (${BUILD_COMPONENTS_TESTING})
  add_subdirectory(tests)
endif()

# This copies the include files when `install` is ran
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/zend_abstract_interface.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zend_abstract_interface/
)

target_link_libraries(datadog_php_components
  INTERFACE zai_zend_abstract_interface
)

install(TARGETS zai_zend_abstract_interface
  EXPORT DatadogPhpComponentsTargets
)
