add_executable(cage
    main.cpp
)

set_property(TARGET cage PROPERTY FOLDER "model")

set_target_properties(cage PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(cage PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(cage PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(cage PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
