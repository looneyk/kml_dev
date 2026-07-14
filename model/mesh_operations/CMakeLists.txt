add_executable(mesh_operations
    main.cpp
)

set_property(TARGET mesh_operations PROPERTY FOLDER "model")

set_target_properties(mesh_operations PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(mesh_operations PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(mesh_operations PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(mesh_operations PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(mesh_operations
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
