add_executable(mesh_diff
    main.cpp
)

set_property(TARGET mesh_diff PROPERTY FOLDER "utility")

set_target_properties(mesh_diff PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(mesh_diff PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(mesh_diff PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
