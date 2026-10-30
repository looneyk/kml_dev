add_library(quetzal_stratum_gen STATIC
    generators.cpp
    generators.hpp
)

set_property(TARGET quetzal_stratum_gen PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_gen PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_stratum_gen PUBLIC quetzal_catagen quetzal_common quetzal_dungen quetzal_stratum_format)
