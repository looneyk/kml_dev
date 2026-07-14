add_executable(spikosahedron
    main.cpp
)

set_property(TARGET spikosahedron PROPERTY FOLDER "model")

set_target_properties(spikosahedron PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(spikosahedron PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(spikosahedron PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(spikosahedron PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(spikosahedron
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
