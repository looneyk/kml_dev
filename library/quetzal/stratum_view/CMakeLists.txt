add_library(quetzal_stratum_view STATIC
    ascii.cpp
    cells.cpp
    ascii.hpp
    cells.hpp
    Console.hpp
)

set_property(TARGET quetzal_stratum_view PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_view PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_stratum_view PUBLIC quetzal_stratum_sim)
