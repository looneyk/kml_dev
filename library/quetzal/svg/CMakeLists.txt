add_library(quetzal_svg STATIC
    Document.cpp
    Document.hpp
)

set_property(TARGET quetzal_svg PROPERTY FOLDER "library")

set_target_properties(quetzal_svg PROPERTIES OUTPUT_NAME svg)

target_include_directories(quetzal_svg PRIVATE
    ${DEV_ROOT}/library
)
