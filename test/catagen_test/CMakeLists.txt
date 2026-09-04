add_executable(catagen_test
    main.cpp
)

set_property(TARGET catagen_test PROPERTY FOLDER "test")

set_target_properties(catagen_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(catagen_test PRIVATE
    quetzal_catagen
    quetzal_common
    quetzal_math
)
