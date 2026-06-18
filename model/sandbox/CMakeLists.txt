add_executable(sandbox
    main.cpp
)

set_property(TARGET sandbox PROPERTY FOLDER "model")

set_target_properties(sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(sandbox PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(sandbox PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(sandbox PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
