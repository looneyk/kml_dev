#if !defined(QUETZAL_BREP_INTERSECTION_SPLITTER_HPP)
#define QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
//------------------------------------------------------------------------------
// brep
// IntersectionSplitter.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "Submesh.hpp"
#include "Halfedge.hpp"
#include "id.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/math/floating_point.hpp"
#include <array>
#include <format>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

#include "quetzal/brep/Visualizer.hpp"
#include "quetzal/common/file_util.hpp"
#include "validation.hpp"
#include <span>
#include <iostream>

namespace quetzal::brep
{

    //--------------------------------------------------------------------------
    template<typename F>
    void test_halfege_face_iteration(const F& face)
    {
        std::cout << "test_halfedge_face_iteration face id " << face.id() << std::endl;
        const auto& halfedges = face.halfedges();
        std::cout << "    forward iterator" << std::endl;
        for (auto i = halfedges.begin(); i != halfedges.end(); ++i)
        {
            const auto& halfedge = *i;
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
        std::cout << "    reverse iterator" << std::endl;
        for (auto i = halfedges.rbegin(); i != halfedges.rend(); ++i)
        {
            const auto& halfedge = *i;
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
        std::cout << "    range for" << std::endl;
        for (const auto& halfedge : halfedges)
        {
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
        const auto& halfedges_ext = face.halfedges_ext();
        auto j = halfedges_ext.begin();
        std::cout << "    ext forward iterator" << std::endl;
        for (auto i = halfedges_ext.begin(); i != halfedges_ext.end(); ++i)
        {
            const auto& halfedge = *i;
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
        std::cout << "    ext reverse iterator" << std::endl;
        for (auto i = halfedges_ext.rbegin(); i != halfedges_ext.rend(); ++i)
        {
            const auto& halfedge = *i;
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
        std::cout << "    ext range for" << std::endl;
        for (const auto& halfedge : halfedges_ext)
        {
            std::cout << "        " << halfedge.id() << " " << vertex_position(halfedge) << std::endl;
        }
    }

    // This class produces a set of intersection traces which are identified by matched pairs of border halfedges, each of which split the submesh into two bordered fragments.
    // These border traces can then be used to produce any CSG operation, union, intersection, difference, and symmetric difference by stitching them back together in different ways.
    // A connection is a matched pair of submesh borders identified by the first border halfedges of each submesh's exterior and interior fragments.

    // New halfedge pairs are created coupled as normal so that all linkage is available if necessary. Then uncouple is called just before returning.
    // Intersection points are clamped to vertex positions so that exact position comparisons can be made.

    //--------------------------------------------------------------------------
    template<typename M>
    class IntersectionSplitter
    {
    public:

        using connections_type = std::vector<connection_type>;

        IntersectionSplitter(M& mesh);
        IntersectionSplitter(const IntersectionSplitter&) = default;
        IntersectionSplitter(IntersectionSplitter&&) noexcept = default;
        ~IntersectionSplitter() = default;

        IntersectionSplitter& operator=(const IntersectionSplitter&) = default;
        IntersectionSplitter& operator=(IntersectionSplitter&&) noexcept = default;

        // A connection is an independent intersection trace that splits each of the two submeshes into exterior and interior fragments by uncoupling sequential partner halfedges
        // Since submeshes are not necessarily convex, there may be several such connections
        // Returns a vector of submesh border starting point halfedge ids {A exterior, A interior, B exterior, B interior}
        // Only the positions and texcoords of newly created vertices are valid, normals (currectly set to face normal) will need to be calculated/recalculated for the border vertices
        connections_type connections(const id_type submesh_id_a, const id_type submesh_id_b);

    private:

        using value_type = M::value_type;
        using point_type = M::point_type;
        using vector_type = point_type::vector_type;
        using ray_type = geometry::Ray<typename M::vector_traits>;
        using segment_type = geometry::Segment<typename M::vector_traits>;
        using plane_type = geometry::Plane<typename M::vector_traits>;
        using polygon_type = geometry::Polygon<typename M::vector_traits>;
        using intersection_type = geometry::Intersection<typename M::vector_traits>;
        using halfedge_type = M::halfedge_type;
        using face_type = M::face_type;

        // find_first returns a pair of these
        // find_next returns one, it does not need to return a segment
        // these are used to construct the actual connection traces

        //----------------------------------------------------------------------
        struct HalfedgeIntersection
        {
            id_type m_halfedge_id_a = nullid;
            id_type m_halfedge_id_b = nullid;
            point_type m_position;
        };

        //----------------------------------------------------------------------
        struct Floating
        {
            // id0, id1; continuous partner chains, each is the other's end, id0 and id1 will either both be valid ids or both nullid
            id_type id0 = nullid;
            id_type id1 = nullid;
            void clear() { id0 = nullid; id1 = nullid; }
            bool valid() const { return  id0 == nullid && id1 == nullid || id0 != nullid && id1 != nullid; }
            operator bool() const { assert(valid()); return id0 != nullid; }
        };

        // update this to handle coplanar intersections and the resultant dual paths ...
        connection_type split_intersection_trace(const id_type face_id_a, const id_type face_id_b);

        // None of the following first or next intersection functions handles the degenerate case of a single point intersection, but may not need to
        // None of the following first or next intersection functions handles the case of a planar intersection

        // Try to find an intersection of the two faces, used when looking for a new connection trace
        // Returns {{id_a0, id_b0, position0}, {id_a1, id_b1, position1}} where the positions can be can be:
        //   no intersection (n == 0) (termination condition, no trace)
        //   a single point intersection (n == 1) (termination condition, no trace, currently single points are ignored, but maybe should split halfedges as required? ...)
        //   a virtual segment, two points on separate halfedges (n == 2) (id != nullid) or floating (ids == nullid)
        //   a segment of an existing halfedge (n == 2) (id0 == id1)
        // Note: only intersctions of convex faces are currently supported, so for now there should only be one segment of intersection ...
        std::vector<HalfedgeIntersection> find_first_trace_intersection_linear(id_type face_id_a, id_type face_id_b, const vector_type& trace_direction);

        // Try to find an intersection of the two faces, used when looking for a new connection trace and the two faces are coplanar
        // Returns elements of {id_a, id_b, position} where the positions can be:
        //   no intersection (n == 0) (termination condition, no trace)
        //   a single point intersection (n == 1) (ids != nullid) (termination condition, no trace, currently single points are ignored, but maybe should split halfedges as required? ...)
        //   face/trace inclusion intersection (n == 1) (one id == nullid, other is an internal vertex)
        //   a segment of an existing halfedge (n == 2) (id0 == id1)
        //   the a and b paths from the starting point to the single ending point of a polygonal intersection (n == 3)
        //       elements are {a path start point, b path start point, ending point}
        //       can be floating, completely internal (arbitrary id0 == id1)
        // Note: only intersctions of convex faces are currently supported, so for now there should only be one set of intersection paths ...
        std::vector<HalfedgeIntersection> find_first_trace_intersection_coplanar(id_type face_id_a, id_type face_id_b);

        // Follow an intersection as it traverses the submeshes
        // Given a starting intersection point
        // Returns nullopt or {id_a1, id_b1, point} where the latter can be:
        //   a virtual segment from the starting point to id1 (id_a1 and/or id_b1)
        //   a segment of the current or prev halfedge (id1 == halfedge_id or id1 == halfedge_id prev)
        // The caller needs to check id1 to make the distinction
        // It should only return no intersection in certain corner cases (which are not being handled at this time)
        HalfedgeIntersection find_next_trace_intersection_linear(id_type halfedge_id_a, id_type halfedge_id_b, const vector_type& trace_direction);

        // Follow an intersection as it traverses the submeshes
        // Given a starting intersection point
        // Returns nullopt or {id_a1, id_b1, point} where the latter can be:
        //   the meeting point of the two alternative paths around a polygonal (coplanar) intersection
        // The caller needs to check id1 to make the distinction
        // It should only return no intersection in certain corner cases (which are not being handled at this time)
        HalfedgeIntersection find_next_trace_intersection_coplanar(id_type halfedge_id_a, id_type halfedge_id_b);

        // Returns exterior, impending border halfedge id, tracer halfedge id
        // tracer is nullid unless the edge endpoint is floating or there has been a split and we know that we can continue with the partner face (probably not the or...)
        // Segment endpoints should already be denoised
        std::array<id_type, 2> create_intersection_edge(id_type face_id, id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating);

        // Returns exterior, impending border halfedge id, tracer halfedge id
        // tracer is nullid unless the edge endpoint is floating or there has been a split and we know that we can continue with the partner face (probably not the or...)
        // Segment endpoints should already be denoised
        std::array<id_type, 2> create_intersection_edge(id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating);

        // Returns the ids of the halfedges at this vertex that form the next intersection point
        std::array<id_type, 2> find_next_intersection_faces(id_type halfedge_id_a, id_type halfedge_id_b, id_type vertex_id_a, id_type vertex_id_b, id_type halfedge_id_a_reject, id_type halfedge_id_b_reject);

        // Returns the ids of the halfedges at this vertex that form the next intersection point
        std::array<id_type, 2> find_next_intersection_faces(id_type vertex_id_a, id_type vertex_id_b, id_type halfedge_id_a_reject, id_type halfedge_id_b_reject);

        // Returns the id of the halfedge at this vertex which intersects with the opposite face, and the new face id in the other submesh
        id_type find_next_intersection_face(id_type vertex_id, id_type halfedge_id_other, id_type halfedge_id_reject);

        // Returns the first intersection of two non-convex polygons
        // The intersection starts at the common vertex at position and extends through the polygon interior to the closest intersection
        intersection_type polygon_intersection(const polygon_type& polygon_a, const polygon_type& polygon_b, const point_type& position);

        // Find the face intersection with the plane of the opposing face
        // Returns nullopt or {id0, id1, segment} where the latter can be:
        //   a virtual segment, two points on separate halfedges (id != nullid) or floating (id == nullid)
        //   a an existing halfedge (id1 == nullid, id0 next vertex position is implied)
        // Endpoints are ordered by trace_direction
        // Does not support non-convex faces ...
        std::optional<std::tuple<id_type, id_type, segment_type>> face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction);
        std::optional<std::tuple<id_type, id_type, segment_type>> face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction, id_type halfedge_id0, const Floating& floating);

        intersection_type halfedge_plane_intersection(const halfedge_type& halfedge, const plane_type& plane);

        // Create a pair of connected partner halfedges where neither originates at an existing vertex
        // Returns the id of the halfedge in the direction of the segment
        id_type create_edge_internal(id_type face_id, const segment_type& segment);

        // Create a pair of connected partner halfedges splitting the halfedge vertex and extending to a new vertex at position
        // Returns the id of the halfedge with its vertex at the original halfedge vertex position
        id_type create_edge_partial(id_type halfedge_id, const point_type& position);

        // Create a pair of connected partner halfedges between two existing vertices without the need for splitting the face
        // Used to connect an internal floating chain to an edge, thus removing the floating chain, but not creating a new face
        // Returns the id of the halfedge from the halfedge_id0 to the halfedge_id1 vertex position
        // compare this with mesh_util connect ...
        id_type create_edge_connect(id_type halfedge_id0, id_type halfedge_id1, bool coupled = true);

        void insert_trace_intersection(id_type halfedge_id_a_exterior, id_type halfedge_id_a_interior, id_type halfedge_id_b_exterior, id_type halfedge_id_b_interior);

        id_type exterior_halfedge(id_type halfedge_id, const vector_type& exterior_direction) const;

        id_type interior_partner(id_type halfedge_id_exterior, const vector_type& exterior_direction) const;

        id_type find_halfedge(id_type face_id, const point_type& position);

        // position_a is farther along than position_b in direction
        // position_a and position_b should both fall on a line with the given direction or else the result will be ambiguous
        bool farther(const point_type& position_a, const point_type& position_b, const vector_type& direction);

        M& m_mesh;
        std::set<std::array<id_type, 2>> m_face_id_pairs_checked;
        point_type m_connection_position0; // The position of the start point of the current intersection connection
        // Only one of these will ever be valid at a given time
        Floating m_floating_a;
        Floating m_floating_b;

        std::set<id_type> m_connection_halfedge_ids; // All of the halfedge ids that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        std::set<point_type> m_connection_positions; // All of the vertex positons that are part of any continuous connection (not just the start points) so that they are not mistaken for new intersections (for functionality)
        connections_type m_connections;

        size_t m_stage_count = 0; // logging ...
    };

    //--------------------------------------------------------------------------
    // Add parameter for right vector ...
    template<typename M>
    void visualize(const M& mesh, std::span<id_type> submesh_ids, const typename M::point_type& eye, const typename M::point_type& target, std::optional<typename M::value_type> fov, const std::string& filename)
    {
        using point_type = M::point_type;

        auto forward = normalize(target - eye);
        auto up = M::vector_type::z_direction();
        auto right = normalize(cross(forward, up));
        if (math::float_eq0(right.norm_squared()))
        {
            // brute force vertical view correction, should support continuous rotation through vertical, but we don't have enough information here ...
            // should at least be perpendicular, not fixed ...
            right = M::vector_type::x_direction(); // ...
        }
        quetzal::geometry::Orientation<typename M::vector_traits> orientation = {forward, right};
        quetzal::geometry::Projection<typename M::vector_traits> projection;
        if (fov)
        {
            quetzal::geometry::OrientedPoint<typename M::vector_traits> viewpoint = {eye, orientation};
            projection.init(viewpoint, *fov);
        }
        else
        {
            projection.init(orientation);
        }
        quetzal::brep::Visualizer<M> visualizer(projection, 0.5, 0.25);
        for (id_type submesh_id : submesh_ids)
        {
            visualizer.draw_submesh(mesh.submesh(submesh_id));
        }
        visualizer.write(filename);
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void visualize(const M& mesh, const std::string& name, std::array<id_type, 2> submesh_ids)
    {
        auto bounds_a = mesh.submesh(submesh_ids[0]).bounds();
        auto bounds_b = mesh.submesh(submesh_ids[1]).bounds();
        auto bounds = op_union(bounds_a, bounds_b);
        auto target = bounds.centroid();
        auto extent = bounds.extent();
        auto extent_max = extent[extent.longest_component()];

        typename M::point_type eye = {0.5f, -2.0f, 1.5f}; // norm about 2.5
        eye *= extent_max * M::val(0.5);

        // projection type selection ...
//        std::optional<typename M::value_type> fov = std::nullopt;
        std::optional<typename M::value_type> fov = math::PiHalf<typename M::value_type>;

        std::cout << std::format("[svg: {}]", name) << std::endl;
        auto submesh_ids_visualized = std::vector<id_type>{submesh_ids[0], submesh_ids[1]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}.svg", name));
        submesh_ids_visualized = {submesh_ids[0]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}_a.svg", name));
        submesh_ids_visualized = {submesh_ids[1]};
        visualize(mesh, submesh_ids_visualized, eye, target, fov, std::format("debug/{}_b.svg", name));
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    ValidationState validate_dump(const M& mesh, const std::string& name)
    {
        std::cout << std::format("[dump: {}]", name) << std::endl;
        std::ofstream ofs(std::format("debug/{}.txt", name), std::ios_base::binary);
        OstreamRedirect redirect(std::cout, ofs);
        ValidationState state(true);
        state = validate(mesh);
        dump_faces(mesh);
        dump_submeshes(mesh);
        return state;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    ValidationState debug_log(const M& mesh, const std::string& name, std::array<id_type, 2> submesh_ids)
    {
        ValidationState state;
        if constexpr (debug_validate_dump)
        {
            state = validate_dump(mesh, name);
        }
        if constexpr (debug_visualize)
        {
            visualize(mesh, name, submesh_ids);
        }
        return state;
    }

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::IntersectionSplitter<M>::IntersectionSplitter(M& m_mesh) :
    m_mesh(m_mesh),
    m_face_id_pairs_checked(),
    m_connection_position0(),
    m_floating_a{},
    m_floating_b{},
    m_connection_halfedge_ids(),
    m_connection_positions(),
    m_connections()
{
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::IntersectionSplitter<M>::connections_type quetzal::brep::IntersectionSplitter<M>::connections(const id_type submesh_id_a, const id_type submesh_id_b)
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
                if (!connection.empty())
                {
                    m_connections.push_back(connection);
                }
            }
        }

        if (m_connections.size() == connection_count_orig) // No new connections were found
        {
            break;
        }

        for (const auto& connection : m_connections)
        {
            for (const auto& connection_node : connection)
            {
                assert(connection_node.size() == 1 || connection_node.size() == 2);
                for (const auto& connection_borders : connection_node)
                {
                    for (id_type halfedge_id : connection_borders)
                    {
                        assert(m_connection_halfedge_ids.contains(halfedge_id));
                        assert(m_connection_positions.contains(vertex_position(m_mesh.halfedge(halfedge_id))));
                    }
                }
            }
        }
    }

    return m_connections;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::connection_type quetzal::brep::IntersectionSplitter<M>::split_intersection_trace(const id_type face_id_a, const id_type face_id_b)
{
std::cout << std::format("split_intersection_trace fa {}, fb {}", id_string(face_id_a), id_string(face_id_b)) << std::endl;
std::array<id_type, 2> submesh_ids = {m_mesh.face(face_id_a).submesh_id(), m_mesh.face(face_id_b).submesh_id()};

    assert(!m_floating_a);
    assert(!m_floating_b);

    // coplanar? ...
    // if they are parallel
    //     if no point contains return nullopt
    // they are coplanar, find the surface intersection
    // open question how to store this in the intersection trace context and then use it in the operation ...
    //     intersection_trace can no longer be simply halfedge_id based, but we need to allow for a series of sections, single and double path ...
    //     as is we have a single path only
    //     we need a vector of intersection_trace_sections, each of which is a single path (4 halfedge_ids's) or a pair of them (second one optional?) ...
    //     build out this new intersection trace and use/test if with the single path case ...
    // with test 1, the surface intersection is the entirety of the trace, so that will work fine?
    //     union - inner replaced by face(s) in outer
    //     intersection - just inner
    //     difference - connect inner inverted and delete intersection face(s); or empty result, depending on order of operands ...
    //     symmetric difference - same as difference because of empty result ...
    // with test 2, partial surface overlap
    //     union and intersection - need to choose which intersection polygon to keep, really pick either one
    //     difference - remove both overlapping polygon areas
    // what about adjacent coplanar face intersections, the trace should not be limited to planar sections ...

std::cout << "first " << m_stage_count << std::endl;
    std::optional<vector_type> trace_direction;
    std::vector<HalfedgeIntersection> ixs0;

    // Try to find an intersection to start
    if (parallel(m_mesh.face(face_id_a), m_mesh.face(face_id_b)))
    {
        if (coplanar(m_mesh.face(face_id_a), m_mesh.face(face_id_b)))
        {
            ixs0 = find_first_trace_intersection_coplanar(face_id_a, face_id_b);
        }
    }
    else
    {
        // This is the direction of the submesh_a interior and submesh_b exterior halfedges
        // We will use this to trace the intersection along the submesh_b exterior
        trace_direction = normalize(cross(m_mesh.face(face_id_a).attributes().normal(), m_mesh.face(face_id_b).attributes().normal()));
std::cout << std::format("trace_direction {}", *trace_direction) << std::endl;

        ixs0 = find_first_trace_intersection_linear(face_id_a, face_id_b, *trace_direction);
    }

    if (ixs0.empty() || ixs0.size() == 1)
    {
        return {};
    }

    assert(ixs0.size() == 2); // Limited to convex faces for now, no more than a single segment of intersection ...

// if the segment was discarded because of reversed direction, we can just jump to the partner and start there instead of iterating through the rest of the faces ...
    // this would have to make changes in face_plane_intersection however ...

    const auto [halfedge_id_a0, halfedge_id_b0, position0] = ixs0[0];
    const auto [halfedge_id_a1, halfedge_id_b1, position1] = ixs0[1];
    segment_type segment0{position0, position1};
std::cout << std::format("find_first_trace_intersection_linear returned ha0 {}, ha1 {}, hb0 {}, hb1 {}, segment0 {}", id_string(halfedge_id_a0), id_string(halfedge_id_a1), id_string(halfedge_id_b0), id_string(halfedge_id_b1), segment0) << std::endl;

    auto [halfedge_id_a_exterior, halfedge_id_a_tracer] = create_intersection_edge(face_id_a, halfedge_id_a0, halfedge_id_a1, segment0, -*trace_direction, m_floating_a);
    id_type halfedge_id_a_interior = interior_partner(halfedge_id_a_exterior, -*trace_direction);
std::cout << std::format("create_intersection_edge first returned a ext {}, int {}, ht {}", id_string(halfedge_id_a_exterior), id_string(halfedge_id_a_interior), id_string(halfedge_id_a_tracer)) << std::endl;

    auto [halfedge_id_b_exterior, halfedge_id_b_tracer] = create_intersection_edge(face_id_b, halfedge_id_b0, halfedge_id_b1, segment0, *trace_direction, m_floating_b);
    id_type halfedge_id_b_interior = interior_partner(halfedge_id_b_exterior, *trace_direction);
std::cout << std::format("create_intersection_edge first returned b ext {}, int {}, ht {}", id_string(halfedge_id_b_exterior), id_string(halfedge_id_b_interior), id_string(halfedge_id_b_tracer)) << std::endl;
/*
auto ae = vertex_position(m_mesh.halfedge(halfedge_id_a_exterior));
auto ai = vertex_position(m_mesh.halfedge(halfedge_id_a_interior));
auto be = vertex_position(m_mesh.halfedge(halfedge_id_b_exterior));
auto bi = vertex_position(m_mesh.halfedge(halfedge_id_b_interior));

std::cout << std::format("h ae {}", vertex_position(m_mesh.halfedge(halfedge_id_a_exterior))) << std::endl;
std::cout << std::format("h ai {}", vertex_position(m_mesh.halfedge(halfedge_id_a_interior))) << std::endl;
std::cout << std::format("h be {}", vertex_position(m_mesh.halfedge(halfedge_id_b_exterior))) << std::endl;
std::cout << std::format("h bi {}", vertex_position(m_mesh.halfedge(halfedge_id_b_interior))) << std::endl;
*/
assert(vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)) == vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));
assert(vertex_position(m_mesh.halfedge(halfedge_id_a_interior)) == vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)));
assert(halfedge_id_a_tracer == nullid || vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)) || vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_a_interior)));
assert(halfedge_id_b_tracer == nullid || vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)) || vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));

    insert_trace_intersection(halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior);
assert(debug_log(m_mesh, std::format("model{}", m_stage_count++), submesh_ids).value() >= 0);

    connection_borders_type connection_borders = {halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior};
    connection_type connection0 = {{connection_borders}};
    m_connection_position0 = segment0.endpoint(0);

    // Find next faces as required
    // This will update any tracer halfedge ids that were nullid in the create_intersection_edge return values
    id_type vertex_id_a = m_mesh.halfedge(halfedge_id_a_exterior).vertex_id();
    id_type vertex_id_b = m_mesh.halfedge(halfedge_id_b_interior).vertex_id();
    id_type halfedge_id_a_reject = halfedge_id_a_exterior;
    id_type halfedge_id_b_reject = halfedge_id_b_interior;
    std::tie(halfedge_id_a_tracer, halfedge_id_b_tracer) = find_next_intersection_faces(halfedge_id_a_tracer, halfedge_id_b_tracer, vertex_id_a, vertex_id_b, halfedge_id_a_reject, halfedge_id_b_reject);
std::cout << std::format("find_next_intersection_faces returned ha {}, hb {}, fa {}, fb {}", id_string(halfedge_id_a_tracer), id_string(halfedge_id_b_tracer), id_string(m_mesh.halfedge(halfedge_id_a_tracer).face_id()), id_string(m_mesh.halfedge(halfedge_id_b_tracer).face_id())) << std::endl;

    // Now we can uncouple the halfedges that will form the borders
    // (They need to remain coupled for the find_next_intersection_faces vertex halfedge iterator above)
    m_mesh.halfedge(halfedge_id_a_exterior).uncouple();
    m_mesh.halfedge(halfedge_id_b_exterior).uncouple();

    for (bool complete = false; !complete; )
    {
        std::cout << "next " << m_stage_count << std::endl;
        assert(vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)));
        std::optional<HalfedgeIntersection> ix;

        if (parallel(m_mesh.halfedge(halfedge_id_a_tracer).face(), m_mesh.halfedge(halfedge_id_b_tracer).face()))
        {
            if (coplanar(m_mesh.halfedge(halfedge_id_a_tracer).face(), m_mesh.halfedge(halfedge_id_b_tracer).face()))
            {
                ix = find_next_trace_intersection_coplanar(halfedge_id_a_tracer, halfedge_id_b_tracer);
            }
        }
        else
        {
            bool tracing = trace_direction.has_value();
            trace_direction = normalize(cross(m_mesh.halfedge(halfedge_id_a_tracer).face_attributes().normal(), m_mesh.halfedge(halfedge_id_b_tracer).face_attributes().normal()));
            std::cout << std::format("trace_direction {}", *trace_direction) << std::endl;

            if (!tracing)
            {
                // compaare trace_direction with the direction we've been using and reverse the trace and the active end if necessary ...
            }

            ix = find_next_trace_intersection_linear(halfedge_id_a_tracer, halfedge_id_b_tracer, *trace_direction);
        }
        assert(ix); // Only in certain degenerate cases (not handled yet) would this fail here ...

        const auto [halfedge_id_a_next, halfedge_id_b_next, position] = *ix;
        segment_type segment{vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)), position};
std::cout << std::format("find_next_trace_intersection_linear returned han {}, hbn {}, position {}", id_string(halfedge_id_a_next), id_string(halfedge_id_b_next), position) << std::endl;

// may not have trace_direction here yet, use effective trace direction (of this edge) instead ...
        std::tie(halfedge_id_a_exterior, halfedge_id_a_tracer) = create_intersection_edge(halfedge_id_a_tracer, halfedge_id_a_next, segment, -*trace_direction, m_floating_a);
        halfedge_id_a_interior = interior_partner(halfedge_id_a_exterior, -*trace_direction);
std::cout << std::format("create_intersection_edge next a returned ext {} {}, int {} {}, ht {}", id_string(halfedge_id_a_exterior), vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)), id_string(halfedge_id_a_interior), vertex_position(m_mesh.halfedge(halfedge_id_a_interior)), id_string(halfedge_id_a_tracer)) << std::endl;

// may not have trace_direction here yet, use effective trace direction (of this edge) instead ...
        std::tie(halfedge_id_b_exterior, halfedge_id_b_tracer) = create_intersection_edge(halfedge_id_b_tracer, halfedge_id_b_next, segment, *trace_direction, m_floating_b);
        halfedge_id_b_interior = interior_partner(halfedge_id_b_exterior, *trace_direction);
std::cout << std::format("create_intersection_edge next b returned ext {} {}, int {} {}, ht {}", id_string(halfedge_id_b_exterior), vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)), id_string(halfedge_id_b_interior), vertex_position(m_mesh.halfedge(halfedge_id_b_interior)), id_string(halfedge_id_b_tracer)) << std::endl;

std::cout << std::format("h ae {}", vertex_position(m_mesh.halfedge(halfedge_id_a_exterior))) << std::endl;
std::cout << std::format("h ai {}", vertex_position(m_mesh.halfedge(halfedge_id_a_interior))) << std::endl;
std::cout << std::format("h be {}", vertex_position(m_mesh.halfedge(halfedge_id_b_exterior))) << std::endl;
std::cout << std::format("h bi {}", vertex_position(m_mesh.halfedge(halfedge_id_b_interior))) << std::endl;

assert(vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)) == vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));
assert(vertex_position(m_mesh.halfedge(halfedge_id_a_interior)) == vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)));
assert(halfedge_id_a_tracer == nullid || vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)) || vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_a_interior)));
assert(halfedge_id_b_tracer == nullid || vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)) || vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));

        insert_trace_intersection(halfedge_id_a_exterior, halfedge_id_a_interior, halfedge_id_b_exterior, halfedge_id_b_interior);
assert(debug_log(m_mesh, std::format("model{}", m_stage_count++), submesh_ids).value() >= 0);

        assert(ne_or_exact(position, m_connection_position0));
        complete = position == m_connection_position0;
        if (!complete)
        {
            // Find next faces as required
            // This will update any tracer halfedge ids that were nullid in the create_intersection_edge return values
            vertex_id_a = m_mesh.halfedge(halfedge_id_a_exterior).vertex_id();
            vertex_id_b = m_mesh.halfedge(halfedge_id_b_interior).vertex_id();
            halfedge_id_a_reject = halfedge_id_a_exterior;
            halfedge_id_b_reject = halfedge_id_b_interior;
            std::tie(halfedge_id_a_tracer, halfedge_id_b_tracer) = find_next_intersection_faces(halfedge_id_a_tracer, halfedge_id_b_tracer, vertex_id_a, vertex_id_b, halfedge_id_a_reject, halfedge_id_b_reject);
std::cout << std::format("find_next_intersection_faces returned ha {}, hb {}, fa {}, fb {}", id_string(halfedge_id_a_tracer), id_string(halfedge_id_b_tracer), halfedge_id_a_tracer != nullid ? id_string(m_mesh.halfedge(halfedge_id_a_tracer).face_id()) : "nullid", halfedge_id_b_tracer != nullid ? id_string(m_mesh.halfedge(halfedge_id_b_tracer).face_id()) : "nullid") << std::endl;
            assert(halfedge_id_a_tracer != nullid);
            assert(halfedge_id_b_tracer != nullid);
            assert(vertex_position(m_mesh.halfedge(halfedge_id_a_tracer)) == vertex_position(m_mesh.halfedge(halfedge_id_b_tracer)));
        }
        else
        {
std::cout << std::format("split_intersection_trace complete") << std::endl;
        }
        
        // Now we can uncouple the halfedges that will form the borders
        // They needed to remain coupled until now for the find_next_intersection_faces vertex halfedge iterator above
        m_mesh.halfedge(halfedge_id_a_exterior).uncouple();
        m_mesh.halfedge(halfedge_id_b_exterior).uncouple();
    }

    return connection0;
}

//------------------------------------------------------------------------------
template<typename M>
std::vector<typename quetzal::brep::IntersectionSplitter<M>::HalfedgeIntersection> quetzal::brep::IntersectionSplitter<M>::find_first_trace_intersection_linear(id_type face_id_a, id_type face_id_b, const vector_type& trace_direction)
{
    using geometry::Locus;
std::cout << std::format("find_first_trace_intersection_linear fa {}, fb {}", face_id_a, face_id_b) << std::endl;

    const auto& face_a = m_mesh.face(face_id_a);
    const auto& face_b = m_mesh.face(face_id_b);

    const auto plane_a = to_plane(face_a);
    const auto plane_b = to_plane(face_b);

    auto ix_a = face_plane_intersection(face_a, plane_b, trace_direction);
    if (!ix_a)
    {
std::cout << "face_plane_intersection a returned nullopt" << std::endl;
        return {};
    }

    auto [id_a0, id_a1, segment_a] = *ix_a;
std::cout << std::format("face_plane_intersection a returned a0 {}, a1 {}, segment {}", id_string(id_a0), id_string(id_a1), segment_a) << std::endl;

    auto ix_b = face_plane_intersection(face_b, plane_a, trace_direction);
    if (!ix_b)
    {
std::cout << "face_plane_intersection b returned nullopt" << std::endl;
        return {};
    }

    auto [id_b0, id_b1, segment_b] = *ix_b;
std::cout << std::format("face_plane_intersection b returned b0 {}, b1 {}, segment {}", id_string(id_b0), id_string(id_b1), segment_b) << std::endl;

    if (!intersects(segment_a, segment_b)) // "skew" polygons
    {
        return {};
    }

    // Intersect the two resulting segments with endpoints ordered by trace_direction

    assert(float_eq(segment_a.direction(), trace_direction));
    assert(float_eq(segment_b.direction(), trace_direction));
/*
std::cout << std::format("h a0 {}", vertex_position(m_mesh.halfedge(id_a0))) << std::endl;
std::cout << std::format("h a1 {}", vertex_position(m_mesh.halfedge(id_a1))) << std::endl;
std::cout << std::format("h b0 {}", vertex_position(m_mesh.halfedge(id_b0))) << std::endl;
std::cout << std::format("h b1 {}", vertex_position(m_mesh.halfedge(id_b1))) << std::endl;
std::cout << std::format("s a0 {}", segment_a.endpoint(0)) << std::endl;
std::cout << std::format("s a1 {}", segment_a.endpoint(1)) << std::endl;
std::cout << std::format("s b0 {}", segment_b.endpoint(0)) << std::endl;
std::cout << std::format("s b1 {}", segment_b.endpoint(1)) << std::endl;
*/
//assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));
//assert(ne_or_exact(segment_a.endpoint(0), segment_b.endpoint(0)));
//assert(ne_or_exact(segment_a.endpoint(1), segment_b.endpoint(1)));

    segment_type segment;

    if (float_eq(segment_a.endpoint(0), segment_b.endpoint(0)))
    {
        if (id_a0 != nullid && vertex_position(m_mesh.halfedge(id_a0)) == segment_a.endpoint(0))
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
        else if (id_b0 != nullid && vertex_position(m_mesh.halfedge(id_b0)) == segment_b.endpoint(0))
        {
            segment.endpoint(0) = segment_b.endpoint(0);
        }
        else
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
    }
    else if (farther(segment_b.endpoint(0), segment_a.endpoint(0), trace_direction))
    {
        segment.endpoint(0) = segment_b.endpoint(0);
        id_a0 = nullid;
    }
    else
    {
        segment.endpoint(0) = segment_a.endpoint(0);
        id_b0 = nullid;
    }

    if (float_eq(segment_a.endpoint(1), segment_b.endpoint(1)))
    {
        if (id_a1 != nullid && vertex_position(m_mesh.halfedge(id_a1)) == segment_a.endpoint(1))
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
        else if (id_b1 != nullid && vertex_position(m_mesh.halfedge(id_b1)) == segment_b.endpoint(1))
        {
            segment.endpoint(1) = segment_b.endpoint(1);
        }
        else
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
    }
    else if (farther(segment_b.endpoint(1), segment_a.endpoint(1), trace_direction))
    {
        segment.endpoint(1) = segment_a.endpoint(1);
        id_b1 = nullid;
    }
    else
    {
        segment.endpoint(1) = segment_b.endpoint(1);
        id_a1 = nullid;
    }
//std::cout << std::format("s0 {}", segment.endpoint(0)) << std::endl;
//std::cout << std::format("s1 {}", segment.endpoint(1)) << std::endl;

    assert(float_eq(segment.direction(), trace_direction));
    HalfedgeIntersection ix0 = {id_a0, id_b0, segment.endpoint(0)};
    HalfedgeIntersection ix1 = {id_a1, id_b1, segment.endpoint(1)};
    return {ix0, ix1};
}

//------------------------------------------------------------------------------
template<typename M>
std::vector<typename quetzal::brep::IntersectionSplitter<M>::HalfedgeIntersection> quetzal::brep::IntersectionSplitter<M>::find_first_trace_intersection_coplanar(id_type face_id_a, id_type face_id_b)
{
    using geometry::Locus;
    std::cout << std::format("find_first_trace_intersection_linear coplanar fa {}, fb {}", face_id_a, face_id_b) << std::endl;

    const auto& face_a = m_mesh.face(face_id_a);
    const auto& face_b = m_mesh.face(face_id_b);

    const auto polygon_a = to_polygon(face_a);
    const auto polygon_b = to_polygon(face_b);

    // Find intersection of the two coplanar polygons
    auto ixs = intersection_non_convex(polygon_a, polygon_b);
    std::cout << std::format("intersection_non_convex returned {} intersections", ixs.size()) << std::endl;
    if (ixs.empty())
    {
        return {};
    }

    std::vector<HalfedgeIntersection> intersections;

    assert(ixs.size() == 1); // Only the convex face case is handled here for now (single intersection) ...
    for (const auto& ix : ixs)
    {
        switch (ix.locus())
        {
        case Locus::Point:
            // do nothing? create a split point? ...
            break;

        case Locus::Segment:
        {
            auto [position0, position1] = ix.segment().endpoints();
            intersections.push_back({ find_halfedge(face_id_a, position0), find_halfedge(face_id_b, position0), position0 });
            intersections.push_back({ find_halfedge(face_id_a, position1), find_halfedge(face_id_b, position1), position1 });
            break;
        }

        case Locus::Polygon:
            for (const auto& position : ix.polygon().vertices())
            {
                id_type id_a = find_halfedge(face_id_a, position);
                if (id_a == nullid)
                {
                    continue;
                }

                id_type id_b = find_halfedge(face_id_b, position);
                if (id_b == nullid)
                {
                    continue;
                }

                // arbitrarily use the first intersection found as the starting point for the trace, may need to be reversed later though ...
                if (intersections.empty()) // Only the convex face case is handled here for now (empty is not the correct condition for multiple connections ...
                {
                    // create the two paths around the polygon in the face a direction (reverse the b direction) ...
                    // is this the right/enough info for create edge? ...
                    // this won't create just one edge, but trace the paths ...
                    intersections.push_back({id_a, nullid, position});
                    id_b = m_mesh.halfedge(id_b).vertex_attributes().position() == position ? m_mesh.halfedge(id_b).prev().partner_id() : m_mesh.halfedge(id_b).partner_id();
                    intersections.push_back({nullid, id_b, position});
                    continue;
                }

                intersections.push_back({id_a, id_b, position});
            }

            if (intersections.empty())
            {
                id_type halfedge_id_a = nullid;
                id_type halfedge_id_b = nullid;
                id_type halfedge_id = m_mesh.face(face_id_a).halfedge_id();
                if (to_polygon(m_mesh.face(face_id_b)).contains(vertex_position(m_mesh.halfedge(halfedge_id))))
                {
                    halfedge_id_a = halfedge_id;
                }
                else
                {
                    halfedge_id = m_mesh.face(face_id_b).halfedge_id();
                    assert(to_polygon(m_mesh.face(face_id_b)).contains(vertex_position(m_mesh.halfedge(halfedge_id))));
                    halfedge_id_b = halfedge_id;
                }

                auto position = vertex_position(m_mesh.halfedge(halfedge_id));
                intersections.push_back({halfedge_id_a, halfedge_id_b, position});
            }
            else
            {
                assert(intersections.size() == 3); // Only the convex face case is handled here for now ...
            }

            break;

        default:
            assert(false);
        }
    }

    return intersections;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::HalfedgeIntersection quetzal::brep::IntersectionSplitter<M>::find_next_trace_intersection_linear(const id_type halfedge_id_a, const id_type halfedge_id_b, const vector_type& trace_direction)
{
    using geometry::Locus;

    auto position0 = vertex_position(m_mesh.halfedge(halfedge_id_a));
    assert(vertex_position(m_mesh.halfedge(halfedge_id_b)) == position0);
std::cout << std::format("find_next_trace_intersection_linear ha {}, hb {}, position {}, trace_direction {}, fa {}, fb {}", id_string(halfedge_id_a), id_string(halfedge_id_b), position0, trace_direction, m_mesh.halfedge(halfedge_id_a).face_id(), m_mesh.halfedge(halfedge_id_b).face_id()) << std::endl;

    const auto& face_a = m_mesh.halfedge(halfedge_id_a).face();
    const auto& face_b = m_mesh.halfedge(halfedge_id_b).face();

    const auto plane_a = to_plane(face_a);
    const auto plane_b = to_plane(face_b);

    auto ix_a = face_plane_intersection(face_a, plane_b, trace_direction, halfedge_id_a, m_floating_a);
    auto [id_a0, id_a1, segment_a] = *ix_a;
    assert(id_a0 == halfedge_id_a);
    assert(segment_a.endpoint(0) == position0);
std::cout << std::format("face_plane_intersection a returned a0 {}, a1 {}, segment {}", id_string(id_a0), id_string(id_a1), segment_a) << std::endl;

    auto ix_b = face_plane_intersection(face_b, plane_a, trace_direction, halfedge_id_b, m_floating_b);
    auto [id_b0, id_b1, segment_b] = *ix_b;
    assert(id_b0 == halfedge_id_b);
    assert(segment_b.endpoint(0) == position0);
std::cout << std::format("face_plane_intersection b returned b0 {}, b1 {}, segment {}", id_string(id_b0), id_string(id_b1), segment_b) << std::endl;

    assert(float_eq(segment_a.direction(), trace_direction));
    assert(float_eq(segment_b.direction(), trace_direction));

//std::cout << std::format("position0 {}", position0) << std::endl;
//std::cout << std::format("h a1 {}", vertex_position(m_mesh.halfedge(id_a1))) << std::endl;
//std::cout << std::format("h b1 {}", vertex_position(m_mesh.halfedge(id_b1))) << std::endl;
//std::cout << std::format("s a1 {}", segment_a.endpoint(1)) << std::endl;
//std::cout << std::format("s b1 {}", segment_b.endpoint(1)) << std::endl;

    point_type position = float_eq(segment_a.endpoint(1), segment_b.endpoint(1)) || farther(segment_b.endpoint(1), segment_a.endpoint(1), trace_direction) ? segment_a.endpoint(1) : segment_b.endpoint(1);
    segment_type segment = {position0, position};
    assert(float_eq(segment.direction(), trace_direction));
//std::cout << std::format("position {}", position) << std::endl;

    if (float_eq(segment_a.endpoint(1), segment_b.endpoint(1))) // denoise
    {
        if (vertex_position(m_mesh.halfedge(id_a1)) == segment_a.endpoint(1))
        {
            position = segment_a.endpoint(1);
        }
        else if (vertex_position(m_mesh.halfedge(id_b1)) == segment_b.endpoint(1))
        {
            position = segment_b.endpoint(1);
        }
    }
    else
    {
//        auto d = segment.vector().norm_squared();
        auto d = (position - position0).norm_squared();
        if (math::float_gt(segment_a.vector().norm_squared(), d))
        {
            if (to_segment(m_mesh.halfedge(id_a0)).contains(position))
            {
                id_a1 = id_a0;
            }
            else
            {
                id_a1 = nullid;
            }
        }
        if (math::float_gt(segment_b.vector().norm_squared(), d))
        {
            if (to_segment(m_mesh.halfedge(id_b0)).contains(position))
            {
                id_b1 = id_b0;
            }
            else
            {
                id_b1 = nullid;
            }
        }
    }

    // need to pass back indication that this segment crosses surface or is an edge ...
    // point_a_opt and point_b_opt convey this information ...
    // this may just be an optimization that allows create_intersection_edge to avoid segment endpoint comparisons ...
    return {id_a1, id_b1, position};
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::HalfedgeIntersection quetzal::brep::IntersectionSplitter<M>::find_next_trace_intersection_coplanar(const id_type halfedge_id_a, const id_type halfedge_id_b)
{
    using geometry::Locus;

    std::vector<HalfedgeIntersection> intersections;

    for (const auto& halfedge_a : m_mesh.halfedge(halfedge_id_a).face().halfedges())
    {
        for (const auto& halfedge_b : m_mesh.halfedge(halfedge_id_b).face().halfedges())
        {
            auto ix = intersection(to_segment(halfedge_a), to_segment(halfedge_b));
            if (ix.locus() == Locus::Empty)
            {
                continue;
            }

            if (ix.locus() == Locus::Point)
            {
                intersections.push_back({halfedge_a.id(), halfedge_b.id(), ix.point()});
                continue;
            }

            assert(ix.locus() == Locus::Segment);
            intersections.push_back({halfedge_a.id(), halfedge_b.id(), ix.segment().endpoint(0)});
            intersections.push_back({halfedge_a.id(), halfedge_b.id(), ix.segment().endpoint(1)});
        }
    }

    assert(intersections.size() == 2); // Only the convex face case is handled here for now ...
    assert(intersections[0].m_halfedge_id_a == halfedge_id_a && intersections[0].m_halfedge_id_b == halfedge_id_b);
    return intersections[1];
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::create_intersection_edge(const id_type face_id, id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating)
{
std::cout << std::format("create_intersection_edge first f {}, h0 {}, h1 {}, segment {}, exterior_direction {}", id_string(face_id), id_string(halfedge_id0), id_string(halfedge_id1), segment, exterior_direction) << std::endl;
    assert(halfedge_id0 == nullid || m_mesh.halfedge(halfedge_id0).face_id() == face_id);
    assert(halfedge_id1 == nullid || m_mesh.halfedge(halfedge_id1).face_id() == face_id);
    assert(halfedge_id0 == nullid || to_segment(m_mesh.halfedge(halfedge_id0)).contains(segment.endpoint(0)));
    assert(halfedge_id1 == nullid || to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
    assert(halfedge_id0 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id0)), segment.endpoint(0)));
    assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));

    // Both intersections floating, within face
    if (halfedge_id0 == nullid && halfedge_id1 == nullid)
    {
std::cout << "create_intersection_edge first floating" << std::endl;
        id_type halfedge_id = create_edge_internal(face_id, segment);
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);

        // This is the only place where floating gets initialized
        assert(!floating);
        id_type partner_id = m_mesh.halfedge(halfedge_id).partner_id();
        floating = {halfedge_id, partner_id};
//std::cout << std::format("create_intersection_edge first floating h {} {}", id_string(halfedge_id), to_segment(m_mesh.halfedge(halfedge_id))) << std::endl;
//std::cout << std::format("create_intersection_edge first floating p {} {}", id_string(partner_id), to_segment(m_mesh.halfedge(partner_id))) << std::endl;
//std::cout << std::format("create_intersection_edge first floating e {} {}", id_string(halfedge_id_exterior), to_segment(m_mesh.halfedge(halfedge_id_exterior))) << std::endl;
        m_mesh.face(face_id).create_hole(halfedge_id);
//        id_type halfedge_id_tracer = partner_id; // correct, there is no other halfedge at this vertex ...
//        return {halfedge_id_exterior, halfedge_id_tracer};
        return {halfedge_id_exterior, nullid};
    }

    // Intersections along edges require a split so that each then reference vertices instead

    id_type halfedge_id_tracer = nullid;

    if (halfedge_id0 != nullid)
    {
        if (segment.endpoint(0) != vertex_position(m_mesh.halfedge(halfedge_id0)))
        {
std::cout << std::format("create_intersection_edge first split_edge_0 h {} {}, pos {}", halfedge_id0, to_segment(m_mesh.halfedge(halfedge_id0)), segment.endpoint(0)) << std::endl;
            split_edge(m_mesh, halfedge_id0, segment.endpoint(0));
auto hn = m_mesh.halfedge(halfedge_id0).next();
auto hp = m_mesh.halfedge(halfedge_id0).partner();
std::cout << std::format("    created next {} {}, partner {} {}", hn.id(), to_segment(hn), hp.id(), to_segment(hp)) << std::endl;
            halfedge_id0 = m_mesh.halfedge(halfedge_id0).next_id();
        }
    }

    if (halfedge_id1 != nullid)
    {
        if (segment.endpoint(1) != vertex_position(m_mesh.halfedge(halfedge_id1)))
        {
            assert(to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
std::cout << std::format("create_intersection_edge first split_edge_1 h {}, segment {}, pos {}", halfedge_id1, to_segment(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)) << std::endl;
            split_edge(m_mesh, halfedge_id1, segment.endpoint(1));
auto hn = m_mesh.halfedge(halfedge_id1).next();
auto hp = m_mesh.halfedge(halfedge_id1).partner();
std::cout << std::format("    created next {} {}, partner {} {}", hn.id(), to_segment(hn), hp.id(), to_segment(hp)) << std::endl;
//            halfedge_id_tracer = m_mesh.halfedge(halfedge_id1).partner_id(); // this is the possible fold back edge case ...
            halfedge_id1 = m_mesh.halfedge(halfedge_id1).next_id();
        }
    }

    // Handle the case where one of the intersection endpoints is within the face

    // Initial intersection point is floating, within face
    if (halfedge_id0 == nullid)
    {
std::cout << "create_intersection_edge first initial floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id1, segment.endpoint(0));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    assert(segment.endpoint(0) == vertex_position(m_mesh.halfedge(halfedge_id0)));

    // Terminal intersection point is floating, within face
    if (halfedge_id1 == nullid)
    {
std::cout << "create_intersection_edge first terminal floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id0, segment.endpoint(1));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
//        halfedge_id_tracer = m_mesh.halfedge(halfedge_id).partner_id(); // correct, there is no other halfedge at this vertex ...
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    assert(segment.endpoint(1) == vertex_position(m_mesh.halfedge(halfedge_id1)));

    // We now have a vertex to vertex intersection

    id_type halfedge_id_exterior = nullid;

    if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).next_id()) // Intersection is this edge
    {
std::cout << "create_intersection_edge first vv intersection this edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(halfedge_id0, exterior_direction);
    }
    else if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).prev_id()) // Intersection is the previous edge
    {
std::cout << "create_intersection_edge first vv intersection prev edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id0).prev_id(), exterior_direction);
    }
    else // Intersection requires a vertex to vertex connection
    {
        split_face(m_mesh, halfedge_id0, halfedge_id1);
std::cout << std::format("create_intersection_edge first split_face vv returned f {} h0 {} h1 {} created h0p {} h1p {}", face_id, halfedge_id0, halfedge_id1, m_mesh.halfedge(halfedge_id0).prev_id(), m_mesh.halfedge(halfedge_id1).prev_id()) << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id1).prev_id(), exterior_direction);
    }

    return {halfedge_id_exterior, halfedge_id_tracer};
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::create_intersection_edge(const id_type halfedge_id0, id_type halfedge_id1, const segment_type& segment, const vector_type& exterior_direction, Floating& floating)
{
//std::cout << std::format("create_intersection_edge next assertion hid1 {}, position1 {}, endpoint1 {}", halfedge_id1, halfedge_id1 != nullid ? vertex_position(m_mesh.halfedge(halfedge_id1)) : point_type{}, segment.endpoint(1)) << std::endl;
    assert(halfedge_id0 != nullid);
    assert(halfedge_id1 == nullid || to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
    assert(vertex_position(m_mesh.halfedge(halfedge_id0)) == segment.endpoint(0));
    assert(halfedge_id1 == nullid || ne_or_exact(vertex_position(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)));

    // Split face according to the type of intersection

    const id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    assert(halfedge_id1 == nullid || m_mesh.halfedge(halfedge_id1).face_id() == face_id);
std::cout << std::format("create_intersection_edge next f {}, h0 {}, h1 {}, segment {}, exterior_direction {}", id_string(face_id), id_string(halfedge_id0), id_string(halfedge_id1), segment, exterior_direction) << std::endl;

    id_type halfedge_id_tracer = nullid;

    // Terminal intersection point is floating, within face
    if (halfedge_id1 == nullid)
    {
std::cout << "create_intersection_edge next floating" << std::endl;
        id_type halfedge_id = create_edge_partial(halfedge_id0, segment.endpoint(1));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
        id_type partner_id = m_mesh.halfedge(halfedge_id).partner_id();

        // This is the only place where floating chain gets extended
        if (halfedge_id0 == floating.id1)
        {
            floating.id1 = partner_id;
        }

//        halfedge_id_tracer = partner_id; // correct, there is no other halfedge at this vertex ...
        return {halfedge_id_exterior, halfedge_id_tracer};
    }

    if (halfedge_id1 == floating.id0)
    {
        // Close off the floating section
        // Add a hole with an uncoupled face
std::cout << std::format("create_intersection_edge next create_edge_connect floating f {} h0 {} h1 {} -> {} {}", face_id, id_string(halfedge_id0), id_string(halfedge_id1), m_mesh.halfedge(halfedge_id0).prev_id(), m_mesh.halfedge(halfedge_id1).prev_id()) << std::endl;
        id_type halfedge_id = create_edge_connect(halfedge_id0, halfedge_id1);
assert(vertex_position(m_mesh.halfedge(halfedge_id).partner()) == vertex_position(m_mesh.halfedge(halfedge_id).next()));
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);
std::cout << std::format("create_intersection_edge next partial h {} e {} {} {}", id_string(halfedge_id), id_string(halfedge_id_exterior), vertex_position(m_mesh.halfedge(halfedge_id)), vertex_position(m_mesh.halfedge(halfedge_id_exterior))) << std::endl;

        id_type face_halfedge_id = m_mesh.halfedge(halfedge_id_exterior).partner_id();
        id_type hole_halfedge_id = halfedge_id_exterior;

        id_type face_id1 = m_mesh.create_face(m_mesh.face(face_id).surface_id(), face_halfedge_id, m_mesh.face(face_id).attributes());
        for (auto& halfedge : m_mesh.face(face_id1).halfedges())
        {
            halfedge.set_face_id(face_id1);
        }

        m_mesh.face(face_id).create_hole(hole_halfedge_id);
        m_mesh.face(face_id).remove_hole(floating.id0);
        floating.clear();
        return {halfedge_id_exterior, nullid};
    }

    // Intersection along the edge requires a split
    if (segment.endpoint(1) != vertex_position(m_mesh.halfedge(halfedge_id1)))
    {
        assert(to_segment(m_mesh.halfedge(halfedge_id1)).contains(segment.endpoint(1)));
std::cout << std::format("create_intersection_edge next split_edge_1 h {} {}, pos {}", halfedge_id1, to_segment(m_mesh.halfedge(halfedge_id1)), segment.endpoint(1)) << std::endl;
        split_edge(m_mesh, halfedge_id1, segment.endpoint(1));
auto hn = m_mesh.halfedge(halfedge_id1).next();
auto hp = m_mesh.halfedge(halfedge_id1).partner();
std::cout << std::format("    created next {} {}, partner {} {}", hn.id(), to_segment(hn), hp.id(), to_segment(hp)) << std::endl;
//        halfedge_id_tracer = m_mesh.halfedge(halfedge_id1).partner_id();  // probably ok even if the intersection trace folds back into the same face (vertex position is correct) ...
//assert(vertex_position(m_mesh.halfedge(halfedge_id_tracer)) == segment.endpoint(1));
        halfedge_id1 = m_mesh.halfedge(halfedge_id1).next_id();
    }

    if (halfedge_id0 == floating.id1)
    {
        // Add the floating section to the face

        id_type halfedge_id = create_edge_connect(halfedge_id0, halfedge_id1);
        id_type halfedge_id_exterior = exterior_halfedge(halfedge_id, exterior_direction);

        m_mesh.face(face_id).remove_hole(floating.id0);
        floating.clear();
        return {halfedge_id_exterior, nullid};
    }

    // We now have a vertex to vertex intersection

    id_type halfedge_id_exterior = nullid;

    if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).next_id()) // Intersection is this edge
    {
std::cout << "create_intersection_edge next vv intersection this edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(halfedge_id0, exterior_direction);
    }
    else if (halfedge_id1 == m_mesh.halfedge(halfedge_id0).prev_id()) // Intersection is the previous edge
    {
std::cout << "create_intersection_edge next vv intersection prev edge" << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id0).prev_id(), exterior_direction);
    }
    else // Intersection requires a vertex to vertex connection
    {
std::cout << std::format("create_intersection_edge next split_face vv f {}, h0 {}, h1 {}", face_id, halfedge_id0, halfedge_id1) << std::endl;
//validate(m_mesh);
//dump_faces(m_mesh);
        split_face(m_mesh, halfedge_id0, halfedge_id1);
std::cout << std::format("create_intersection_edge next split_face vv returned created h0p {}, h1p {}", m_mesh.halfedge(halfedge_id0).prev_id(), m_mesh.halfedge(halfedge_id1).prev_id()) << std::endl;
        halfedge_id_exterior = exterior_halfedge(m_mesh.halfedge(halfedge_id1).prev_id(), exterior_direction);
    }

    return {halfedge_id_exterior, halfedge_id_tracer};
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::find_next_intersection_faces(id_type halfedge_id_a, id_type halfedge_id_b, const id_type vertex_id_a, const id_type vertex_id_b, const id_type halfedge_id_a_reject, const id_type halfedge_id_b_reject)
{
std::cout << std::format("find_next_intersection_faces ha {}, hb {}, va {}, vb {}, reject a {}, reject b {}", id_string(halfedge_id_a), id_string(halfedge_id_b), id_string(vertex_id_a), id_string(vertex_id_b), id_string(halfedge_id_a_reject), id_string(halfedge_id_b_reject)) << std::endl;
    assert(m_mesh.vertex(vertex_id_a).attributes().position() == m_mesh.vertex(vertex_id_b).attributes().position());
    assert(to_segment(m_mesh.halfedge(halfedge_id_a_reject)) == to_segment(m_mesh.halfedge(halfedge_id_b_reject)));

    if (halfedge_id_a == nullid)
    {
        if (halfedge_id_b == nullid)
        {
std::cout << "find both" << std::endl;
            std::tie(halfedge_id_a, halfedge_id_b) = find_next_intersection_faces(vertex_id_a, vertex_id_b, halfedge_id_a_reject, halfedge_id_b_reject);
        }
        else
        {
std::cout << "find a" << std::endl;
            halfedge_id_a = find_next_intersection_face(vertex_id_a, halfedge_id_b, halfedge_id_a_reject);
        }
    }
    else if (halfedge_id_b == nullid)
    {
std::cout << "find b" << std::endl;
        halfedge_id_b = find_next_intersection_face(vertex_id_b, halfedge_id_a, halfedge_id_b_reject);
    }

    assert(vertex_position(m_mesh.halfedge(halfedge_id_a)) == vertex_position(m_mesh.halfedge(halfedge_id_b)));
//std::cout << "    " << std::format("ha {}, hb {}", id_string(halfedge_id_a), id_string(halfedge_id_b)) << std::endl;
//std::cout << "    " << std::format("fa {}, fb {}", m_mesh.halfedge(halfedge_id_a).face_id(), m_mesh.halfedge(halfedge_id_b).face_id()) << std::endl;
    return {halfedge_id_a, halfedge_id_b};
}

//------------------------------------------------------------------------------
template<typename M>
std::array<quetzal::id_type, 2> quetzal::brep::IntersectionSplitter<M>::find_next_intersection_faces(const id_type vertex_id_a, const id_type vertex_id_b, const id_type halfedge_id_a_reject, const id_type halfedge_id_b_reject)
{
    halfedge_id_a_reject;
std::cout << std::format("find_next_intersection_faces va {}, vb {}, reject a {}, reject b {}", id_string(vertex_id_a), id_string(vertex_id_b), id_string(halfedge_id_a_reject), id_string(halfedge_id_b_reject)) << std::endl;

    for (const auto& halfedge_a : m_mesh.vertex(vertex_id_a).halfedges())
    {
std::cout << std::format("fnifs loop halfedge a {}, face a {}", id_string(halfedge_a.id()), id_string(halfedge_a.face_id())) << std::endl;
        id_type halfedge_id_a = halfedge_a.id();
        id_type halfedge_id_b = find_next_intersection_face(vertex_id_b, halfedge_id_a, halfedge_id_b_reject);
        if (halfedge_id_b != nullid)
        {
            return {halfedge_id_a, halfedge_id_b};
        }
std::cout << "    no face b found" << std::endl;
    }
/*
    for (const auto& halfedge_b : m_mesh.vertex(vertex_id_b).halfedges())
    {
std::cout << std::format("halfedge b {}, face b {}", id_string(halfedge_b.id()), id_string(halfedge_b.face_id())) << std::endl;
        id_type halfedge_id_b = halfedge_b.id();
        id_type halfedge_id_a = find_next_intersection_face(vertex_id_a, halfedge_id_b, halfedge_id_a_reject);
        if (halfedge_id_a == nullid)
        {
std::cout << "    no face a found" << std::endl;
            continue;
        }

        return {halfedge_id_a, halfedge_id_b};
    }
*/
    // may be a degenerate case that ends up here ...
    assert(false);
    return {nullid, nullid};
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::find_next_intersection_face(const id_type vertex_id, const id_type halfedge_id_other, const id_type halfedge_id_reject)
{
    using geometry::Locus;

    assert(m_mesh.vertex(vertex_id).halfedge().face_id() == m_mesh.halfedge(halfedge_id_reject).face_id());

    auto halfedge_other = m_mesh.halfedge(halfedge_id_other);
    auto position0 = m_mesh.vertex(vertex_id).attributes().position();
std::cout << std::format("find_next_intersection_face v {}, h other {}, f other {}, reject {}", id_string(vertex_id), id_string(halfedge_id_other), id_string(m_mesh.halfedge(halfedge_id_other).face_id()), id_string(halfedge_id_reject)) << std::endl;
    assert(vertex_position(halfedge_other) == position0);

    // Search for the next face intersection connected to this vertex in face_other

    const auto& face_other = halfedge_other.face();
    const auto polygon_other = to_polygon(face_other);

    for (const auto& halfedge : m_mesh.vertex(vertex_id).halfedges())
    {
std::cout << std::format("    fnif loop h {}, f {}, f other {}", id_string(halfedge.id()), id_string(halfedge.face_id()), face_other.id()) << std::endl;
        assert(vertex_position(halfedge) == position0);

        // doing it both ways, don't know which way here, this should be fixed ...
        m_face_id_pairs_checked.insert({halfedge.face_id(), face_other.id()});
        m_face_id_pairs_checked.insert({face_other.id(), halfedge.face_id()});

        const auto& face = halfedge.face();
        if (!intersects(polygon_other, to_plane(face))) // quick rejection
        {
std::cout << "    fnif quick rejection" << std::endl;
            continue;
        }

        const auto polygon = to_polygon(face);
//std::cout << polygon_other << '\n' << polygon << std::endl;
        auto ix = polygon_intersection(polygon, polygon_other, position0);
std::cout << std::format("    fnif polygon_intersection return ix {}", ix) << std::endl;
        if (ix.empty() || ix.locus() == Locus::Point)
        {
//if (ix.empty()) std::cout << "    fnif reject no intersection" << std::endl;
//else std::cout << std::format("    fnif reject point intersection {}", ix.point()) << std::endl;
            continue;
        }

//        assert(ix.locus() == Locus::Segment || ix.locus() == Locus::Polygon);
        assert(ix.locus() == Locus::Segment); // not currently handling planar face intersections ...
        auto segment = ix.segment();

        auto segment_reject = to_segment(m_mesh.halfedge(halfedge_id_reject));
std::cout << std::format("    fnif segment {}, direction {}, segment_reject {}", segment, segment.direction(), segment_reject) << std::endl;
        if (float_eq(segment, segment_reject))
        {
//            assert(halfedge.id() == halfedge_id_reject); // ok? should be, why not? ...
std::cout << std::format("    fnif reject original halfedge {}, segment {}", halfedge_id_reject, segment_reject) << std::endl;
            continue;
        }

        assert(segment.endpoint(0) == position0);

        if (to_segment(halfedge.prev()).contains(segment.endpoint(1))
            || to_segment(halfedge_other.prev()).contains(segment.endpoint(1))) // wrong direction ...
        {
std::cout << std::format("    fnif reject wrong direction {}", to_segment(halfedge.prev())) << std::endl;
            continue;
        }

std::cout << std::format("fnif returning face intersection h {}", halfedge.id()) << std::endl;
        return halfedge.id();
    }

    // There is actually a case where we get here, when the other submesh pulls away and this vertex is the last intersection ...
    // But this is a real outlier, the intersection border ending with an edge ...
    // but normal usage here is to indicate no intersection when used in find_next_intersection_faces ...
std::cout << "    fnif return no intersection" << std::endl;
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::intersection_type quetzal::brep::IntersectionSplitter<M>::polygon_intersection(const polygon_type& polygon_a, const polygon_type& polygon_b, const point_type& position0)
{
    using geometry::Locus;
std::cout << std::format("polygon_intersection position0 {}", position0) << std::endl;
//std::cout << "\n(original)" << std::endl;
//    auto ixx = intersection(polygon_a, polygon_b);
//std::cout << std::format("intersection pp returned ix {}", ixx) << std::endl;
//    return ixx;

    assert(polygon_a.plane().contains(position0));
    assert(polygon_b.plane().contains(position0));
/* why not? ...
    bool b = false;
    for (const auto& position : polygon_a.vertices())
    {
        if (float_eq(position, position0))
        {
            assert(position == position0);
            b = true;
            break;
        }
    }
    assert(b);
    b = false;
    for (const auto& position : polygon_b.vertices())
    {
        if (float_eq(position, position0))
        {
            assert(position == position0);
            b = true;
            break;
        }
    }
    assert(b);
*/
//std::cout << "\n(new)" << std::endl;
    auto ixs = intersection_non_convex(polygon_a, polygon_b);
std::cout << std::format("intersection_non_convex pp returned n {}", ixs.size()) << std::endl;
//for (const auto ix : ixs) std::cout << std::format("    ix {}", ix) << std::endl;
    if (ixs.empty())
    {
        return {};
    }
/*
    for (const auto& ix : ixs)
    {
        if (ix.locus() == Locus::Segment)
        {
            if (float_eq(ix.segment().endpoint(1), position0))
            {
                assert(ix.segment().endpoint(1) == position0);
                ix.segment().reverse();
            }
        }
    }
std::cout << std::format("    oriented n {}", ixs.size()) << std::endl;
for (const auto ix : ixs) std::cout << std::format("    ix {}", ix) << std::endl;
*/
    // All intersection positions fall on the line of intersection of the two planes

    // Arbitrary direction, all intersection points should be in that direction or opposite it
    // If it turns out that there can be intersections in both directions, then we'll need something more sophisticated ...
    auto direction = normalize(cross(polygon_a.plane().normal(), polygon_b.plane().normal()));
    std::vector<value_type> direction_indicators;

    value_type d_closest = std::numeric_limits<value_type>::max();
    std::optional<point_type> position_closest;
    bool found_position0 = false;

    for (const auto& ix : ixs)
    {
        if (ix.locus() == Locus::Point)
        {
            auto position = ix.point();
            if (float_eq(position, position0))
            {
                // assert == ...
                found_position0 = true;
                continue;
            }

            direction_indicators.push_back(dot(position - position0, direction));

            auto d = (position - position0).norm_squared();
            if (math::float_lt(d, d_closest))
            {
                d_closest = d;
                position_closest = position;
            }
        }
        else if (ix.locus() == Locus::Segment)
        {
            for (int i = 0; i < 2; ++i)
            {
                auto position = ix.segment().endpoint(i);
                if (float_eq(position, position0))
                {
                    found_position0 = true;
                    continue;
                }

                direction_indicators.push_back(dot(position - position0, direction));

                auto d = (position - position0).norm_squared();
                if (math::float_lt(d, d_closest))
                {
                    d_closest = d;
                    position_closest = position;
                }
            }
        }
        else if (ix.locus() == Locus::Polygon) // coplanar polygons
        {
            assert(false); // not yet supported ...
        }
    }

    // Check that position0 was in the intersection set
    assert(found_position0);

    if (!position_closest)
    {
        return position0;
    }

    // Check that all intersection points are in the same direction from position0
    size_t n_lt0 = 0;
    size_t n_gt0 = 0;
    for (auto d : direction_indicators)
    {
        if (math::float_lt(d, value_type(0))) ++n_lt0;
        else if (math::float_gt(d, value_type(0))) ++n_gt0;
    }
    assert(n_lt0 == 0 || n_gt0 == 0);

    auto segment = segment_type(position0, *position_closest);

    // Check that the resulting segment is inside the polygons
    // Because the segment is constructed from the closest point, there can be no intervening interior/exterior transitions
    auto midpoint = segment.point(value_type(0.5));
    assert(polygon_a.contains(midpoint));
    assert(polygon_b.contains(midpoint));

    return segment;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<std::tuple<quetzal::id_type, quetzal::id_type, typename quetzal::brep::IntersectionSplitter<M>::segment_type>> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction)
{
    using geometry::Locus;
std::cout << std::format("face_plane_intersection first face {} {}, plane {}, trace_direction {}", face.id(), face.attributes().normal(), plane, trace_direction) << std::endl;

    auto connection_contains = [this](const point_type& position)
    {
        for (const auto& connection_position : m_connection_positions)
        {
            if (float_eq(connection_position, position)) // Reject any portion of an earlier connection
            {
                return true;
            }
        }
        return false;
    };

//test_halfege_face_iteration(face);

    std::vector<std::tuple<id_type, intersection_type>> intersections;
    for (const auto& halfedge : face.halfedges())
//    for (const auto& halfedge : face.halfedges_ext())
    {
        if (m_connection_halfedge_ids.contains(halfedge.id())) // Reject any portion of an earlier connection
//if (connection_contains(vertex_position(halfedge)) || connection_contains(vertex_position(halfedge.next()))) // Reject any portion of an earlier connection
        {
//std::cout << std::format("face_plane_intersection rejecting halfedge {}", halfedge.id()) << std::endl;
            assert(m_connections.size() > 0); // This should never happen on the first connection
            continue;
        }

        auto ix = halfedge_plane_intersection(halfedge, plane);
        if (ix.empty())
        {
            continue;
        }
        else if (ix.locus() == Locus::Segment) // Intersection is an edge of the face
        {
            assert(intersections.empty() || get<0>(intersections[0]) == halfedge.next_id());
            assert(float_eq(ix.segment(), to_segment(halfedge)));
            if (direction_compare(ix.segment().direction(), trace_direction) <= 0)
            {
                return std::nullopt;
            }

            return std::make_tuple(halfedge.id(), halfedge.next_id(), to_segment(halfedge));
        }
        else if (ix.locus() == Locus::Point)
        {
            if (m_connection_positions.contains(ix.point())) // Reject any portion of an earlier connection
//            if (connection_contains(ix.point())) // Reject any portion of an earlier connection
            {
//std::cout << std::format("face_plane_intersection rejecting position ix {}", ix.point()) << std::endl;
                assert(m_connections.size() > 0); // This should never happen on the first connection
                continue;
            }

            intersections.push_back({halfedge.id(), ix});
        }
    }

    if (intersections.size() == 2)
    {
        auto [id0, ix0] = intersections[0];
        auto [id1, ix1] = intersections[1];
        assert(ix0.locus() == Locus::Point && ix1.locus() == Locus::Point);
        auto segment = segment_type(ix0.point(), ix1.point());
        if (direction_compare(segment.direction(), trace_direction) < 0)
        {
            std::swap(id0, id1);
            segment.reverse();
        }
        return std::make_tuple(id0, id1, segment);
    }

    if (intersections.size() == 1)
    {
        // This is the edge case where the plane grazes a single vertex, not yet supported ...
        // This would either be a floating point intersection or no interseetion (or a floating point that is later discarded) ...
        // (there is a similar edge or polyline only connection which should be treated similarly)
        // For now, discard it here, no intersection ...
        // This assertion is failing, we have no test cases that should ever reach this point ...
        // Why are we getting here at all? ...
//        assert(false);
        return std::nullopt;
    }

    assert(intersections.empty());
    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<std::tuple<quetzal::id_type, quetzal::id_type, typename quetzal::brep::IntersectionSplitter<M>::segment_type>> quetzal::brep::IntersectionSplitter<M>::face_plane_intersection(const face_type& face, const plane_type& plane, const vector_type& trace_direction, const id_type halfedge_id0, const Floating& floating)
{
    using geometry::Locus;
std::cout << std::format("face_plane_intersection next face {} {}, plane {}, trace_direction {}", face.id(), face.attributes().normal(), plane, trace_direction) << std::endl;

    point_type position0 = vertex_position(m_mesh.halfedge(halfedge_id0));
    ray_type ray{position0, trace_direction};

    if (floating)
    {
        if (plane.contains(m_connection_position0)) // Closing off the floating connection
        {
            assert(vertex_position(m_mesh.halfedge(floating.id0)) == m_connection_position0);
            segment_type segment(position0, m_connection_position0);
            return std::make_tuple(halfedge_id0, floating.id0, segment);
        }
    }

    std::vector<std::tuple<id_type, intersection_type>> intersections;
    for (const auto& halfedge : face.halfedges())
//    for (const auto& halfedge : face.halfedges_ext())
    {
        auto ix = halfedge_plane_intersection(halfedge, plane);
//std::cout << std::format("halfedge_plane_intersection returned hid {}, segment {}, ix {}", halfedge.id(), to_segment(halfedge), ix) << std::endl;
        if (ix.empty())
        {
            continue;
        }
        else if (ix.locus() == Locus::Segment)
        {
            auto segment = to_segment(halfedge);
            assert(intersections.empty() || get<0>(intersections[0]) == halfedge.next_id());
            assert(float_eq(ix.segment(), segment));
            assert(segment.endpoint(0) == position0 || segment.endpoint(1) == position0);
            if (segment.endpoint(1) == position0) // Not in the right direction (do direction_compare ...)
            {
                continue; // return std::nullopt is only ok for convex faces ...
            }

            return std::make_tuple(halfedge.id(), halfedge.next_id(), segment);
        }
        else if (ix.locus() == Locus::Point)
        {
            // discard halfedge endpoint (half-open interval) ...
        
            if (!ray.contains(ix.point()) || float_eq(ix.point(), position0)) // Not in the right direction
            {
                continue;
            }

            if (float_eq(ix.point(), m_connection_position0)) // Closing off the connection
            {
                ix = m_connection_position0;
                intersections = {{halfedge.id(), ix}};
                break;
            }

            intersections.push_back({halfedge.id(), ix});
        }
    }

    if (intersections.size() == 1)
    {
        id_type id0 = halfedge_id0;
        auto [id1, ix1] = intersections[0];
        assert(ix1.locus() == Locus::Point);
        auto segment = segment_type(position0, ix1.point());
        assert(float_eq(segment.direction(), trace_direction));
        return std::make_tuple(id0, id1, segment);
    }

    assert(intersections.empty());
    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::IntersectionSplitter<M>::intersection_type quetzal::brep::IntersectionSplitter<M>::halfedge_plane_intersection(const halfedge_type& halfedge, const plane_type& plane)
{
    using geometry::Locus;

    const auto segment = to_segment(halfedge);
//std::cout << std::format("halfedge_plane_intersection hid {} segment {} plane {}", halfedge.id(), segment, plane) << std::endl;
    const auto ix = intersection(segment, plane);
    if (ix.locus() == Locus::Point)
    {
        if (float_eq(ix.point(), segment.endpoint(1))) // Halfedges are half-open intervals
        {
            return {};
        }

        if (float_eq(ix.point(), segment.endpoint(0))) // Denoise
        {
            return segment.endpoint(0);
        }

        assert(segment.contains(ix.point()));
        return ix;
    }
    else if (ix.locus() == Locus::Segment)
    {
        assert(ix.segment() == segment);
        return ix;
    }

    assert(ix.empty());
    return {};
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_internal(const id_type face_id, const segment_type& segment)
{
    auto [position0, position1] = segment.endpoints();

    auto normal = m_mesh.face(face_id).attributes().normal();

    auto mapping = position_texcoord_mapping(m_mesh.face(face_id));
    auto texcoord0 = denoise01(mapping(position0));
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0{position0, normal, texcoord0};
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, nh + 1, face_id, av0);
    m_mesh.create_halfedge_vertex(nh, nh, nh, face_id, av1);

std::cout << "    " << std::format("create_edge_internal f {} segment {} created {} {}", id_string(face_id), segment, nh, nh + 1) << std::endl;
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_partial(const id_type halfedge_id0, const point_type& position1)
{
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    auto normal = m_mesh.face(face_id).attributes().normal();

    auto mapping = position_texcoord_mapping(m_mesh.face(face_id));
    auto texcoord1 = denoise01(mapping(position1));

    typename M::vertex_attributes_type av0 = m_mesh.halfedge(halfedge_id0).vertex_attributes();
    typename M::vertex_attributes_type av1{position1, normal, texcoord1};

    size_t nh = m_mesh.halfedge_store_count();
    m_mesh.create_halfedge_vertex(nh + 1, nh + 1, m_mesh.halfedge(halfedge_id0).prev_id(), face_id, av0);
    m_mesh.create_halfedge_vertex(nh, halfedge_id0, nh, face_id, av1);

    m_mesh.halfedge(halfedge_id0).prev().set_next_id(nh);
    m_mesh.halfedge(halfedge_id0).set_prev_id(nh + 1);

std::cout << std::format("create_edge_partial h {} p {} created {} {}", id_string(halfedge_id0), position1, nh, nh + 1) << std::endl;
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::create_edge_connect(const id_type halfedge_id0, const id_type halfedge_id1, bool coupled)
{
std::cout << std::format("create_edge_connect {} {} {} {}", id_string(halfedge_id0), id_string(halfedge_id1), vertex_position(m_mesh.halfedge(halfedge_id0)), vertex_position(m_mesh.halfedge(halfedge_id1))) << std::endl;
    id_type face_id = m_mesh.halfedge(halfedge_id0).face_id();
    assert(m_mesh.halfedge(halfedge_id1).face_id() == face_id);

    size_t nh = m_mesh.halfedge_store_count();

    id_type partner_id0 = coupled ? nh + 1 : nullid;
    id_type partner_id1 = coupled ? nh : nullid;

    const auto av0 = m_mesh.halfedge(halfedge_id0).vertex_attributes();
    const auto av1 = m_mesh.halfedge(halfedge_id1).vertex_attributes();

    m_mesh.create_halfedge_vertex(partner_id0, halfedge_id1, m_mesh.halfedge(halfedge_id0).prev_id(), face_id, av0);
    m_mesh.create_halfedge_vertex(partner_id1, halfedge_id0, m_mesh.halfedge(halfedge_id1).prev_id(), face_id, av1);

    m_mesh.halfedge(halfedge_id0).prev().set_next_id(nh);
    m_mesh.halfedge(halfedge_id0).set_prev_id(nh + 1);

    m_mesh.halfedge(halfedge_id1).prev().set_next_id(nh + 1);
    m_mesh.halfedge(halfedge_id1).set_prev_id(nh);

std::cout << "    " << std::format("create_edge_connect h0 {} h1 {} created {} {}", id_string(halfedge_id0), id_string(halfedge_id1), nh, nh + 1) << std::endl;
    return nh;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::IntersectionSplitter<M>::insert_trace_intersection(const id_type halfedge_id_a_exterior, const id_type halfedge_id_a_interior, const id_type halfedge_id_b_exterior, const id_type halfedge_id_b_interior)
{
    m_connection_halfedge_ids.insert(halfedge_id_a_exterior);
    m_connection_halfedge_ids.insert(halfedge_id_a_interior);
    m_connection_halfedge_ids.insert(halfedge_id_b_exterior);
    m_connection_halfedge_ids.insert(halfedge_id_b_interior);
    
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_a_exterior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_a_interior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_b_exterior)));
    m_connection_positions.insert(vertex_position(m_mesh.halfedge(halfedge_id_b_interior)));

    m_face_id_pairs_checked.insert({m_mesh.halfedge(halfedge_id_a_exterior).face_id(), m_mesh.halfedge(halfedge_id_b_exterior).face_id()});
    m_face_id_pairs_checked.insert({m_mesh.halfedge(halfedge_id_a_exterior).face_id(), m_mesh.halfedge(halfedge_id_b_interior).face_id()});
    m_face_id_pairs_checked.insert({m_mesh.halfedge(halfedge_id_a_interior).face_id(), m_mesh.halfedge(halfedge_id_b_exterior).face_id()});
    m_face_id_pairs_checked.insert({m_mesh.halfedge(halfedge_id_a_interior).face_id(), m_mesh.halfedge(halfedge_id_b_interior).face_id()});
    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::exterior_halfedge(const id_type halfedge_id, const vector_type& exterior_direction) const
{
    return direction_compare(direction(m_mesh.halfedge(halfedge_id)), exterior_direction) > 0 ? halfedge_id : m_mesh.halfedge(halfedge_id).partner_id();
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::interior_partner(const id_type halfedge_id_exterior, const vector_type& exterior_direction) const
{
    id_type halfedge_id_interior = m_mesh.halfedge(halfedge_id_exterior).partner_id();
auto ve = vertex_position(m_mesh.halfedge(halfedge_id_exterior));
auto vi = vertex_position(m_mesh.halfedge(halfedge_id_interior));
auto ven = vertex_position(m_mesh.halfedge(halfedge_id_exterior).next());
auto vin = vertex_position(m_mesh.halfedge(halfedge_id_interior).next());
auto zzz = direction(m_mesh.halfedge(halfedge_id_exterior));
    assert(halfedge_id_exterior == m_mesh.halfedge(halfedge_id_interior).partner_id());
    assert(vertex_position(m_mesh.halfedge(halfedge_id_exterior).next()) == vertex_position(m_mesh.halfedge(halfedge_id_interior)));
    assert(vertex_position(m_mesh.halfedge(halfedge_id_interior).next()) == vertex_position(m_mesh.halfedge(halfedge_id_exterior)));
    assert(direction_compare(direction(m_mesh.halfedge(halfedge_id_exterior)), exterior_direction) > 0);
    assert(float_eq(direction(m_mesh.halfedge(halfedge_id_exterior)), exterior_direction));
    return halfedge_id_interior;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::IntersectionSplitter<M>::find_halfedge(id_type face_id, const point_type& position)
{
    for (const auto& halfedge : m_mesh.face(face_id).halfedges())
    {
        if (to_segment(halfedge).contains(position))
        {
            return halfedge.id();
        }
    }
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
bool quetzal::brep::IntersectionSplitter<M>::farther(const point_type& position_a, const point_type& position_b, const vector_type& direction)
{
    return (direction_compare(position_a - position_b, direction) > 0);
}

#endif // QUETZAL_BREP_INTERSECTION_SPLITTER_HPP
