add_executable(compare_precision_obj
    main.cpp
    Attributes_float_eq.hpp
    obj_io.hpp
)

set_property(TARGET compare_precision_obj PROPERTY FOLDER "utility")

set_target_properties(compare_precision_obj PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(compare_precision_obj PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(compare_precision_obj PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(compare_precision_obj
    quetzal_geometry
)
