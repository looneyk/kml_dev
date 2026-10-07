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

target_link_libraries(stratum_terminal PRIVATE
    quetzal_log
    quetzal_stratum_gen
    quetzal_stratum_view
)

# The development config, beside the executable in bin/<Config>, where an install has assets beside it instead. Copied on
# every build, so a clean or a deleted bin directory is put right by the next build
add_custom_command(TARGET stratum_terminal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/config.json" "$<TARGET_FILE_DIR:stratum_terminal>/config.json"
)
