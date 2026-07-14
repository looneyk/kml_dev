add_executable(wimble
    main.cpp
)

set_property(TARGET wimble PROPERTY FOLDER "model")

set_target_properties(wimble PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(wimble PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(wimble PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(wimble PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(wimble
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
