add_executable(keyvaluestore_test
    main.cpp
)

set_property(TARGET keyvaluestore_test PROPERTY FOLDER "test")

set_target_properties(keyvaluestore_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(keyvaluestore_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(keyvaluestore_test PRIVATE
    quetzal_common
    quetzal_testing
)

doctest_discover_tests(keyvaluestore_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "keyvaluestore_test:"
    PROPERTIES LABELS "quetzal"
)
