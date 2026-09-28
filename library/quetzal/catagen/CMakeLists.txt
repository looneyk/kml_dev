add_library(quetzal_catagen STATIC
    generate.cpp
    Parameters.cpp
    write_zone.cpp
    Zone.cpp
    generate.hpp
    Parameters.hpp
    types.hpp
    write_zone.hpp
    Zone.hpp
)

set_property(TARGET quetzal_catagen PROPERTY FOLDER "library")

target_include_directories(quetzal_catagen PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_catagen PUBLIC quetzal_common quetzal_math PRIVATE quetzal_stratum_format)
