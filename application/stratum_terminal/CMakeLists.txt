add_executable(stratum_terminal
    Application.cpp
    key_input.cpp
    Terminal.cpp
    main.cpp
    Application.hpp
    key_input.hpp
    Terminal.hpp
)

set_property(TARGET stratum_terminal PROPERTY FOLDER "application")

set_target_properties(stratum_terminal PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Shared data lives outside any one application, so its location is configured in rather than
# found relative to wherever the program happens to be started from
target_compile_definitions(stratum_terminal PRIVATE KML_DEV_ASSETS_ROOT="${DEV_ROOT}/assets")

target_link_libraries(stratum_terminal PRIVATE
    quetzal_stratum_view
)
