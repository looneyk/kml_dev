add_executable(stratum_sim_test
    main.cpp
)

set_property(TARGET stratum_sim_test PROPERTY FOLDER "test")

set_target_properties(stratum_sim_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(stratum_sim_test PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(stratum_sim_test PRIVATE
    quetzal_stratum_sim
)
