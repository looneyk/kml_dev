add_executable(coils
    main.cpp
)

set_property(TARGET coils PROPERTY FOLDER "model")

set_target_properties(coils PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(coils PRIVATE
    ${DEV_ROOT}/library
)

target_link_options(coils PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(coils PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
