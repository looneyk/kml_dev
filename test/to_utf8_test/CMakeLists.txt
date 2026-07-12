add_executable(to_utf8_test
    main.cpp
    ${DEV_ROOT}/utility/to_utf8/to_utf8.cpp
    ${DEV_ROOT}/utility/to_utf8/to_utf8.hpp
)

set_property(TARGET to_utf8_test PROPERTY FOLDER "test")

set_target_properties(to_utf8_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(to_utf8_test PRIVATE
    ${DEV_ROOT}/utility/to_utf8
)
