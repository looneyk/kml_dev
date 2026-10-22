add_executable(stratum_format_test
    main.cpp
)

set_property(TARGET stratum_format_test PROPERTY FOLDER "test")

set_target_properties(stratum_format_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(stratum_format_test PRIVATE
    quetzal_log
    quetzal_stratum_format
    quetzal_testing
)

doctest_discover_tests(stratum_format_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "stratum_format_test:"
    PROPERTIES LABELS "stratum"
)
