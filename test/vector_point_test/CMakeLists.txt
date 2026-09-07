add_executable(vector_point_test
    main.cpp
)

set_property(TARGET vector_point_test PROPERTY FOLDER "test")

set_target_properties(vector_point_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(vector_point_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(vector_point_test PRIVATE
    quetzal_common
    quetzal_math
)
