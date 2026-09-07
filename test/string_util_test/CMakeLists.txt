add_executable(string_util_test
    main.cpp
)

set_property(TARGET string_util_test PROPERTY FOLDER "test")

set_target_properties(string_util_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(string_util_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(string_util_test PRIVATE
    quetzal_common
    quetzal_math
)
