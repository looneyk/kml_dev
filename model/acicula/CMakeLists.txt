add_executable(acicula
    main.cpp
)

set_property(TARGET acicula PROPERTY FOLDER "model")

set_target_properties(acicula PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(acicula PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(acicula PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(acicula PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(acicula
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
