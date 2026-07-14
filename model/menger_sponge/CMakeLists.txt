add_executable(menger_sponge
    main.cpp
)

set_property(TARGET menger_sponge PROPERTY FOLDER "model")

set_target_properties(menger_sponge PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(menger_sponge PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(menger_sponge PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(menger_sponge PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(menger_sponge
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
