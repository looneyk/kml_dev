add_executable(settings_test
    main.cpp
)

set_property(TARGET settings_test PROPERTY FOLDER "test")

set_target_properties(settings_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(settings_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(settings_test PRIVATE
    quetzal_settings
    quetzal_testing
)

doctest_discover_tests(settings_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "settings_test:"
    PROPERTIES LABELS "quetzal"
)
