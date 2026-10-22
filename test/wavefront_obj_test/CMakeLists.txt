add_executable(wavefront_obj_test
    main.cpp
)

set_property(TARGET wavefront_obj_test PROPERTY FOLDER "test")

set_target_properties(wavefront_obj_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(wavefront_obj_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(wavefront_obj_test PRIVATE
    quetzal_common
    quetzal_math
    quetzal_svg
    quetzal_testing
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)

doctest_discover_tests(wavefront_obj_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_PREFIX "wavefront_obj_test:"
    PROPERTIES LABELS "mesh_operations"
)

add_dependencies(wavefront_obj_test
    quetzal_brep
    quetzal_geometry
    quetzal_model
)
