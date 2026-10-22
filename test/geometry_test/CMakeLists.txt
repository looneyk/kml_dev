add_executable(geometry_test
    main.cpp
)

set_property(TARGET geometry_test PROPERTY FOLDER "test")

set_target_properties(geometry_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(geometry_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(geometry_test PRIVATE
    quetzal_common
    quetzal_math
    quetzal_testing
)

doctest_discover_tests(geometry_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "geometry_test:"
    PROPERTIES LABELS "mesh_operations"
)

add_dependencies(geometry_test
    quetzal_geometry
    quetzal_physics
)
