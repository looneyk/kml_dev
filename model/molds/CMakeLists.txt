add_executable(molds
    main.cpp
)

set_property(TARGET molds PROPERTY FOLDER "model")

set_target_properties(molds PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(molds PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(molds PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(molds PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(molds
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
