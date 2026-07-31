add_custom_target(quetzal_brep SOURCES
    border.hpp
    BorderFragment.hpp
    clip.hpp
    connection.hpp
    connection2.hpp
    debug.hpp
    Face.hpp
    geometry.hpp
    Halfedge.hpp
    Hole.hpp
    id.hpp
    intersect.hpp
    IntersectionSplitter.hpp
    inversion.hpp
    Mesh.hpp
    MeshBuilder.hpp
    MeshTraits.hpp
    mesh_operations.hpp
    mesh_util.hpp
    normal.hpp
    Seam.hpp
    split.hpp
    Submesh.hpp
    Surface.hpp
    texcoord.hpp
    triangulation.hpp
    validation.hpp
    Vertex.hpp
    visualization.hpp
    Visualizer.hpp
)

set_property(TARGET quetzal_brep PROPERTY FOLDER "library")
