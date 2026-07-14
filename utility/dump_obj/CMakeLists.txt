add_executable(dump_obj
    main.cpp
)

set_property(TARGET dump_obj PROPERTY FOLDER "utility")

set_target_properties(dump_obj PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(dump_obj PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(dump_obj PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
