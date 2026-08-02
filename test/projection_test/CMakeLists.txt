add_executable(projection_test
    main.cpp
)

set_property(TARGET projection_test PROPERTY FOLDER "test")

set_target_properties(projection_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(projection_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(projection_test PRIVATE
    quetzal_common
    quetzal_math
)

add_dependencies(projection_test
    quetzal_geometry
)
