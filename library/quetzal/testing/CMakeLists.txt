add_library(quetzal_testing STATIC
    main.cpp
    named_checks.hpp
    reference_files.cpp
    reference_files.hpp
)

set_property(TARGET quetzal_testing PROPERTY FOLDER "library")

target_include_directories(quetzal_testing PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_testing PUBLIC doctest::doctest)
