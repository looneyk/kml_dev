add_custom_target(quetzal_model SOURCES
    geometry.hpp
    helix_anticylinder.hpp
    helix_pyramid.hpp
    mesh_attributes.hpp
    mesh_diff.hpp
    obj_io.hpp
    primitives.hpp
    primitives_specification.hpp
    primitives_util.hpp
    stl_io.hpp
    transforms.hpp
    vertices_attributes.hpp
)

set_property(TARGET quetzal_model PROPERTY FOLDER "library")
