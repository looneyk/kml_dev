add_custom_target(quetzal_spatial_search SOURCES
    aabb.hpp
    Checker.hpp
    KdTree.hpp
    nkd.hpp
    PointContainer.hpp
    PooledAllocator.hpp
    RTree.hpp
    SeparatorPlane.hpp
    Splitter.hpp
    tree_util.hpp
)

set_property(TARGET quetzal_spatial_search PROPERTY FOLDER "library")
