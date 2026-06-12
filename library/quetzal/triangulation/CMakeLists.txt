add_library(quetzal_triangulation STATIC
    cdt/AdvancingFront.cpp
    cdt/cdt.cpp
    cdt/shapes.cpp
    cdt/Sweep.cpp
    cdt/SweepContext.cpp
    cdt/AdvancingFront.hpp
    cdt/cdt.hpp
    cdt/shapes.hpp
    cdt/Sweep.hpp
    cdt/SweepContext.hpp
    triangulation.hpp
)

set_property(TARGET quetzal_triangulation PROPERTY FOLDER "library")

set_target_properties(quetzal_triangulation PROPERTIES OUTPUT_NAME triangulation)

target_include_directories(quetzal_triangulation PRIVATE
    ${DEV_ROOT}/library
)
