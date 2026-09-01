add_executable(flecs_test
    main.cpp
)

set_property(TARGET flecs_test PROPERTY FOLDER "test")

set_target_properties(flecs_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Plain name rather than flecs::flecs_static: library3 is added after test/ in the root
# CMakeLists, and a namespaced target must already exist when it is referenced
target_link_libraries(flecs_test PRIVATE
    flecs_static
)
