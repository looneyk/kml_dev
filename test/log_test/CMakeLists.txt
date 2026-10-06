add_executable(log_test
    main.cpp
)

set_property(TARGET log_test PROPERTY FOLDER "test")

set_target_properties(log_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(log_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(log_test PRIVATE
    quetzal_log
)
