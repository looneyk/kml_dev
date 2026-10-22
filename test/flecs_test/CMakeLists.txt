add_executable(flecs_test
    main.cpp
)

set_property(TARGET flecs_test PROPERTY FOLDER "test")

set_target_properties(flecs_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Not flecs::flecs_static, as library3 is added after test/ and a namespaced target must already exist
target_link_libraries(flecs_test PRIVATE
    flecs_static
    quetzal_testing
)

doctest_discover_tests(flecs_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "flecs_test:"
    PROPERTIES LABELS "stratum"
)
