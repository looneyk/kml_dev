add_executable(dungen_test
    main.cpp
)

set_property(TARGET dungen_test PROPERTY FOLDER "application")

set_target_properties(dungen_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(dungen_test PRIVATE
    quetzal_dungen
    quetzal_math
)
