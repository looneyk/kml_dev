add_executable(mapping_test
    main.cpp
)

set_property(TARGET mapping_test PROPERTY FOLDER "test")

set_target_properties(mapping_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(mapping_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(mapping_test PRIVATE
    quetzal_common
    quetzal_math
)

add_dependencies(mapping_test
    quetzal_geometry
)
