add_executable(mo_test test.cpp)
target_link_libraries(mo_test PRIVATE mo)

if(NOT MSVC)
    target_compile_options(mo_test PRIVATE -Wall -Wextra -Werror)
endif()

set_target_properties(mo_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME mo_test COMMAND mo_test)
