#if !defined(QUETZAL_BREP_INTERSECTION_SPLITTER2_HPP)
#define QUETZAL_BREP_INTERSECTION_SPLITTER2_HPP
//------------------------------------------------------------------------------
// brep
// IntersectionSplitter2.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "id.hpp"
#include <array>
#include <optional>
#include <set>
#include <vector>

namespace quetzal::brep
{

    // This class produces a set of intersection traces which are identified by matched pairs of border halfedges, each of which split the submesh into two bordered fragments.
    // These border traces can then be used to produce any CSG operation, union, intersection, difference, and symmetric difference by stitching them back together in different ways.
    // A connection is a matched pair of submesh borders identified by the first border halfedges of each submesh's exterior and interior fragments.

    // New halfedge pairs are created coupled as normal so that all linkage is available if necessary. Then uncouple is called just before returning.
    // Intersection points are clamped to vertex positions so that exact position comparisons can be made.

    //--------------------------------------------------------------------------
    template<typename M>
    class IntersectionSplitter2
    {
    public:

        using connection_edge_type = std::array<id_type, 4>;
        using connection_node_type = std::vector<connection_edge_type>; // inplace_vector, either 1 or 2 edges leaving this connection vertex position
        using connection_type = std::vector<connection_node_type>;
        using connections_type = std::vector<connection_type>;

        IntersectionSplitter2(M& mesh);
        IntersectionSplitter2(const IntersectionSplitter2&) = default;
        IntersectionSplitter2(IntersectionSplitter2&&) noexcept = default;
        ~IntersectionSplitter2() = default;

        IntersectionSplitter2& operator=(const IntersectionSplitter2&) = default;
        IntersectionSplitter2& operator=(IntersectionSplitter2&&) noexcept = default;

        // A connection is an independent intersection trace that splits each of the two submeshes into exterior and interior fragments by uncoupling sequential partner halfedges
        // Since submeshes are not necessarily convex, there may be several such connections
        // Returns a vector of vectors of submesh border node starting point halfedge ids {A exterior, A interior, B exterior, B interior}
        // Thre may be one or two edges leaving each node depending on whether the connection section is non-coplanar (single) or coplanar (double)
        // Only the positions and texcoords of newly created vertices are valid, normals (currectly set to face normal) will need to be calculated/recalculated for the border vertices
        connections_type connections(const id_type submesh_id_a, const id_type submesh_id_b);

    private:

        using value_type = M::value_type;
        using point_type = M::point_type;
        using vector_type = point_type::vector_type;
//        using ray_type = geometry::Ray<typename M::vector_traits>;
//        using segment_type = geometry::Segment<typename M::vector_traits>;
//        using plane_type = geometry::Plane<typename M::vector_traits>;
//        using polygon_type = geometry::Polygon<typename M::vector_traits>;
//        using intersection_type = geometry::Intersection<typename M::vector_traits>;
//        using halfedge_type = M::halfedge_type;
//        using face_type = M::face_type;

        std::optional<connection_type> split_intersection_trace(const id_type face_id_a, const id_type face_id_b);

        M& m_mesh;
        std::set<std::array<id_type, 2>> m_face_id_pairs_checked;
        point_type m_connection_position0; // The position of the start point of the current intersection connection
        // Only one of these will ever be valid at a given time
//        Floating m_floating_a;
//        Floating m_floating_b;

        std::set<id_type> m_connection_halfedge_ids; // All of the halfedge ids that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        std::set<point_type> m_connection_positions; // All of the vertex positons that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        connections_type m_connections;

        size_t m_stage_count = 0; // logging ...
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::IntersectionSplitter2<M>::IntersectionSplitter2(M& m_mesh) :
    m_mesh(m_mesh),
    m_face_id_pairs_checked(),
    m_connection_position0(),
//    m_floating_a{},
//    m_floating_b{},
    m_connection_halfedge_ids(),
    m_connection_positions(),
    m_connections()
{
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::IntersectionSplitter2<M>::connections_type quetzal::brep::IntersectionSplitter2<M>::connections(const id_type submesh_id_a, const id_type submesh_id_b)
{
std::array<id_type, 2> submesh_ids = {submesh_id_a, submesh_id_b};
m_stage_count = 0;
assert(debug_log(m_mesh, std::format("model{}", m_stage_count++), submesh_ids).value() >= 0);
    m_face_id_pairs_checked.clear();
    m_connections.clear();

    // The outer loop is to ensure that any new connections from added faces are found
    for (;;)
    {
        size_t connection_count_orig = m_connections.size();

        // Use a snapshot of the face ids since these lists may change
        const auto face_ids_a = m_mesh.submesh(submesh_id_a).face_ids();
        const auto face_ids_b = m_mesh.submesh(submesh_id_b).face_ids();

        for (id_type face_id_a : face_ids_a)
        {
            for (id_type face_id_b : face_ids_b)
            {
                // Keep track of face pairs that have already been checked to avoid unnecessary intersection checks
                // The ids in the set are taken from two distinct sets of submesh face ids
                if (m_face_id_pairs_checked.contains({face_id_a, face_id_b}))
                {
//std::cout << std::format("m_face_id_pairs_checked {} contains {}, {}", m_face_id_pairs_checked.size(), id_string(face_id_a), id_string(face_id_b)) << std::endl;
                    continue;
                }

                m_face_id_pairs_checked.insert({face_id_a, face_id_b});
//std::cout << std::format("m_face_id_pairs_checked {} inserted {{{}, {}}}", m_face_id_pairs_checked.size(), id_string(face_id_a), id_string(face_id_b)) << std::endl;

                // Quickly but inconclusively reject if bounding boxes do not intersect
                if (disjoint(m_mesh.face(face_id_a).bounds(), m_mesh.face(face_id_b).bounds()))
                {
                    continue;
                }

                auto connection = split_intersection_trace(face_id_a, face_id_b);
                if (connection)
                {
                    m_connections.push_back(*connection);
                }
            }
        }

        if (m_connections.size() == connection_count_orig) // No new connections were found
        {
            break;
        }

        /*
        for (const auto& connection : m_connections)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                assert(m_connection_halfedge_ids.contains(connection[i]));
                assert(m_connection_positions.contains(vertex_position(m_mesh.halfedge(connection[i]))));
            }
        }
        */
    }

    return m_connections;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<typename quetzal::brep::IntersectionSplitter2<M>::connection_type> quetzal::brep::IntersectionSplitter2<M>::split_intersection_trace(const id_type face_id_a, const id_type face_id_b)
{
    face_id_a;
    face_id_b;
    return {}; // ...
}

#endif // QUETZAL_BREP_INTERSECTION_SPLITTER2_HPP
