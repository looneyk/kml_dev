add_executable(to_utf8
    main.cpp
    to_utf8.cpp
    to_utf8.hpp
)

set_property(TARGET to_utf8 PROPERTY FOLDER "utility")

set_target_properties(to_utf8 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
