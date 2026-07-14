add_executable(spoctahedron
    main.cpp
)

set_property(TARGET spoctahedron PROPERTY FOLDER "model")

set_target_properties(spoctahedron PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(spoctahedron PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(spoctahedron PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(spoctahedron PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(spoctahedron
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
