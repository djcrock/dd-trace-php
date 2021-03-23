# Need CMake 3.18 for using 'REQUIRED' with find_library().
cmake_minimum_required(VERSION 3.18)

project(zend-abstract-interface
  VERSION 0.1.0
  LANGUAGES C
)

option(BUILD_ZAI_TESTING "Enable tests" OFF)
if (${BUILD_ZAI_TESTING})
  # Tests uses the C++ testing framework Catch2
  enable_language(CXX)

  # The Catch2::Catch2 target has been available since 2.1.2
  # We are unsure of the true minimum, but have tested 2.4
  find_package(Catch2 2.4 REQUIRED)

  #[[ This file takes a while to build, so we do it once here and every test
      executable can link to it to save time.
  ]]
  add_library(catch2_main catch2_main.cc)
  target_link_libraries(catch2_main PUBLIC Catch2::Catch2)
  target_compile_features(catch2_main PUBLIC cxx_std_11)

  include(Catch)
  enable_testing()
endif()

include(GNUInstallDirs)

add_library(zai_zend_abstract_interface INTERFACE)

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

add_subdirectory(zai_sapi)

# All tests depend on zai_sapi
add_subdirectory(functions)

install(EXPORT ZendAbstractInterfaceTargets
  FILE ZendAbstractInterfaceTargets.cmake
  NAMESPACE Zai::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
