add_executable(factory_test
    main.cpp
)

set_property(TARGET factory_test PROPERTY FOLDER "test")

set_target_properties(factory_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(factory_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(factory_test PRIVATE
    quetzal_common
    quetzal_testing
)

doctest_discover_tests(factory_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "factory_test:"
    PROPERTIES LABELS "quetzal"
)
