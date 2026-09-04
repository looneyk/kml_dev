add_executable(dungen
    main.cpp
)

set_property(TARGET dungen PROPERTY FOLDER "tool")

set_target_properties(dungen PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(dungen PRIVATE
    quetzal_dungen
    quetzal_math
)
