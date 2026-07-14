add_executable(helix_pyramid
    main.cpp
)

set_property(TARGET helix_pyramid PROPERTY FOLDER "model")

set_target_properties(helix_pyramid PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(helix_pyramid PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(helix_pyramid PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(helix_pyramid PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(helix_pyramid
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
