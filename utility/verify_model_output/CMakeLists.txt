add_executable(verify_model_output
    main.cpp
)

set_property(TARGET verify_model_output PROPERTY FOLDER "utility")

target_include_directories(verify_model_output PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(verify_model_output PRIVATE
    quetzal_common
)
