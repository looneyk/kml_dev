add_library(quetzal_math STATIC
    math_xm.cpp
    CartesianTraits.hpp
    DiscreteVector.hpp
    floating_point.hpp
    HomogeneousTraits.hpp
    Interval.hpp
    math_util.hpp
    math_xm.hpp
    Matrix.hpp
    matrix_util.hpp
    parametric.hpp
    Quaternion.hpp
    Random.hpp
    Rotation.hpp
    spherical_harmonics.hpp
    Vector.hpp
    vector_util.hpp
    Xoshiro256StarStar.hpp
)

set_property(TARGET quetzal_math PROPERTY FOLDER "library")

set_target_properties(quetzal_math PROPERTIES OUTPUT_NAME math)

target_include_directories(quetzal_math PRIVATE
    ${DEV_ROOT}/library
)
