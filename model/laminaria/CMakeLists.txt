add_executable(laminaria
    main.cpp
)

set_property(TARGET laminaria PROPERTY FOLDER "model")

set_target_properties(laminaria PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(laminaria PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(laminaria PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(laminaria PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
