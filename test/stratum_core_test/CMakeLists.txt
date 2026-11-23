add_executable(stratum_core_test
    main.cpp
)

set_property(TARGET stratum_core_test PROPERTY FOLDER "test")

set_target_properties(stratum_core_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(stratum_core_test PRIVATE
    quetzal_log
    quetzal_stratum_core
    quetzal_testing
)

doctest_discover_tests(stratum_core_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "stratum_core_test:"
    PROPERTIES LABELS "stratum"
)
