add_library(quetzal_catagen STATIC
    generate.cpp
    Level.cpp
    Parameters.cpp
    write_level.cpp
    generate.hpp
    Level.hpp
    Parameters.hpp
    types.hpp
    write_level.hpp
)

set_property(TARGET quetzal_catagen PROPERTY FOLDER "library")

target_include_directories(quetzal_catagen PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_catagen PUBLIC quetzal_common quetzal_math)
