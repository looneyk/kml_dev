add_executable(mesh_diff_test
    main.cpp
)

set_property(TARGET mesh_diff_test PROPERTY FOLDER "test")

set_target_properties(mesh_diff_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(mesh_diff_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(mesh_diff_test PRIVATE
    quetzal_common
    quetzal_math
    quetzal_wavefront_obj
)

add_dependencies(mesh_diff_test
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
