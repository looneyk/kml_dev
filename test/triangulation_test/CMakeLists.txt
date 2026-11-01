add_executable(triangulation_test
    main.cpp
)

set_property(TARGET triangulation_test PROPERTY FOLDER "test")

set_target_properties(triangulation_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(triangulation_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(triangulation_test PRIVATE
    quetzal_testing
    quetzal_triangulation
)

doctest_discover_tests(triangulation_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "triangulation_test:"
    PROPERTIES LABELS "quetzal"
)
