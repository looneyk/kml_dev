add_executable(spherical_harmonics
    main.cpp
)

set_property(TARGET spherical_harmonics PROPERTY FOLDER "model")

set_target_properties(spherical_harmonics PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(spherical_harmonics PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(spherical_harmonics PRIVATE
    quetzal_common
    quetzal_math
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

add_dependencies(spherical_harmonics
    quetzal_brep
    quetzal_model
)
