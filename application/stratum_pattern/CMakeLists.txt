add_executable(stratum_pattern
    Application.cpp
    key_input.cpp
    Terminal.cpp
    main.cpp
    Application.hpp
    key_input.hpp
    Terminal.hpp
)

set_property(TARGET stratum_pattern PROPERTY FOLDER "application")

set_target_properties(stratum_pattern PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(stratum_pattern PRIVATE
    quetzal_stratum_view
)
