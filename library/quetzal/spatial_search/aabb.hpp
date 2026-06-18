
Side_of_triangle_mesh
{
    AABB_face_graph_triangle_primitive<TriangleMesh, VertexPointMap> Primitive;
    AABB_traits<GeomTraits, Primitive> Traits;
    AABB_tree<Traits>* ptree;
}

template <typename AABBTraits>
AABBTree
{
    unique_ptr<AABB_search_tree<AABBTraits>>;
}

template <class Traits>
AABB_search_tree
{
    typedef typename CGAL::Search_traits_3<Add_decorated_point<Traits, typename Traits::Primitive::Id>> TreeTraits;
    typedef typename CGAL::Orthogonal_k_neighbor_search<TreeTraits>::Tree Tree;

    // TreeTraits::Point_d is decorated

    Orthogonal_k_neighbor_search<TreeTraits>::Tree m_tree
}

template <SearchTraits, Tree>
Orthogonal_k_neighbor_search : internal::K_neighbor_search<SearchTraits, Tree = Kd_tree<SearchTraits, Splitter, Tag_true, Tag_false>>
{
    Kd_tree&
}

template <class SearchTraits, Tree>
K_neighbor_search<SearchTraits, Tree>
{
    Kd_tree&
}

Kd_tree
{
    Point_d: Decorated_point
}

For PointContainer, PointSetTraits that has members that access elements, ...
