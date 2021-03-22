add_executable(zend_abstract_interface zend_abstract_interface.cc)

target_link_libraries(zend_abstract_interface
  PUBLIC catch2_main Zai::ZendAbstractInterface
)

file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

catch_discover_tests(zend_abstract_interface)
