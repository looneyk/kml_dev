add_executable(primitives
    main.cpp
)

set_property(TARGET primitives PROPERTY FOLDER "model")

set_target_properties(primitives PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(primitives PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(primitives PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(primitives PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
