add_executable(spinward
    main.cpp
)

set_property(TARGET spinward PROPERTY FOLDER "model")

set_target_properties(spinward PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(spinward PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(spinward PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(spinward PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
