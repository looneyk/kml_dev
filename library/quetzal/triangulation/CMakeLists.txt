add_library(quetzal_triangulation STATIC
    triangulation.cpp
    triangulation.hpp
)

set_property(TARGET quetzal_triangulation PROPERTY FOLDER "library")

set_target_properties(quetzal_triangulation PROPERTIES OUTPUT_NAME triangulation)

target_include_directories(quetzal_triangulation PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_triangulation PRIVATE CDT)
