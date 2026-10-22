add_executable(expression_test
    main.cpp
)

set_property(TARGET expression_test PROPERTY FOLDER "test")

set_target_properties(expression_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(expression_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(expression_test PRIVATE
    quetzal_common
    quetzal_expression
    quetzal_testing
)

doctest_discover_tests(expression_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "expression_test:"
    PROPERTIES LABELS "quetzal"
)
