add_executable(random_test
    main.cpp
)

set_property(TARGET random_test PROPERTY FOLDER "test")

set_target_properties(random_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(random_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(random_test PRIVATE
    quetzal_math
)
