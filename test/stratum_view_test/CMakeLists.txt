add_executable(stratum_view_test
    main.cpp
)

set_property(TARGET stratum_view_test PROPERTY FOLDER "test")

set_target_properties(stratum_view_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(stratum_view_test PRIVATE KML_DEV_ASSETS_ROOT="${DEV_ROOT}/assets")

target_link_libraries(stratum_view_test PRIVATE
    quetzal_stratum_view
    quetzal_testing
)

doctest_discover_tests(stratum_view_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "stratum_view_test:"
    PROPERTIES LABELS "stratum"
)
