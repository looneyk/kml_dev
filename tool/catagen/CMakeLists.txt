add_executable(catagen
    main.cpp
)

set_property(TARGET catagen PROPERTY FOLDER "tool")

set_target_properties(catagen PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(catagen PRIVATE
    quetzal_catagen
)
