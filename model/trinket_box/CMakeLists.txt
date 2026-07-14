add_executable(trinket_box
    main.cpp
)

set_property(TARGET trinket_box PROPERTY FOLDER "model")

set_target_properties(trinket_box PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(trinket_box PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(trinket_box PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(trinket_box PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(trinket_box
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
