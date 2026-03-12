#if !defined(QUETZAL_BREP_SURFACE_OPERATIONS_HPP)
#define QUETZAL_BREP_SURFACE_OPERATIONS_HPP
//------------------------------------------------------------------------------
// brep
// surface_operations.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/intersection.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <array>
#include <algorithm>
#include <span>
#include <vector>

namespace quetzal::brep
{

    // Given that surfaces are rarely coplanar, are these useful at all? ...
    // Yes, but they might be applied only after determining that they are coplanar ...

    // Returns the sub/super surface ids resulting from the operation
/*
    // A || B
    template<typename Traits>
    std::vector<id_type> op_union(const Surface<Traits>& a, const Surface<Traits>& b);

    // A && B
    template<typename Traits>
    std::vector<id_type> op_intersection(const Surface<Traits>& a, const Surface<Traits>& b);
*/
    // A && !B
    // Surface a may be modified, surface b remains the same
    template<typename Traits>
    std::vector<id_type> op_difference(Surface<Traits>& a, const Surface<Traits>& b);

    // A && !B || !A && B
    // Both surface a and b may be modified
    template<typename Traits>
    std::vector<id_type> op_symmetric_difference(Surface<Traits>& a, Surface<Traits>& b);

    // internal

    using surface_border_selector_type = std::function<bool(id_type, id_type)>;

    using edge_intersections_type = std::map<id_type, id_type>; // relative to surface edge, halfedge outer and inner directed respectively
    using edge_intersection_type = edge_intersections_type::value_type;
    
    template<typename Traits>
    using bounding_box_index_type = std::map<id_type, geometry::AxisAlignedBoundingBox<Traits>>;

    template<typename Traits>
    using indexes_type = std::array<bounding_box_index_type<Traits>, 3>;

    // Internally uses checked Flag, should be cleared before calling
    // and marked and comparison ...
    template<typename Traits>
    edge_intersections_type perimeter_intersections(Surface<Traits>& a, const Surface<Traits>& b, surface_border_selector_type surface_border_selector, std::ostream& os = std::cout);

    // Calculate bounding boxes for surface and all of its component faces and halfedges
    // Internally uses checked Flag, should be cleared before calling
    template<typename Traits>
    void surface_detail_bounds(const Surface<Traits>& surface, indexes_type<typename Traits::vector_traits>& indexes);

    template<typename Traits>
//    void intersection_point(Mesh<Traits>& mesh, id_type id_a, id_type id_b, const typename Traits::point_type& position, edge_intersections_type& intersections);
    std::array<id_type, 2> intersection_point(Mesh<Traits>& mesh, id_type halfedge_id_a, id_type halfedge_id_b, const typename Traits::point_type& position);

    template<typename Traits>
    int halfedge_face_orientation(const Halfedge<Traits>& halfedge, const Halfedge<Traits>& halfedge_face);

    template<typename Traits>
    void print_intersection(const Mesh<Traits>& mesh, const edge_intersection_type& intersection);

    template<typename Traits>
    void print_intersections(const Mesh<Traits>& mesh, const edge_intersections_type& intersections);

    // could be tuple ...
    template<typename Traits>
    struct Split
    {
        id_type id_inner;
        id_type id_outer;
        std::vector<typename Traits::point_type> positions;
    };

    template<typename Traits>
    using Splits = std::vector<Split<Traits>>;

    template<typename Traits>
    void print_splits(const Splits<Traits>& splits);

} // namespace quetzal::brep
/*
//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::op_union(const Surface<Traits>& a, const Surface<Traits>& b)
{
    return {}; // ...
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::op_intersection(const Surface<Traits>& a, const Surface<Traits>& b)
{
    return {}; // ...
}
*/
//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::op_difference(Surface<Traits>& a, const Surface<Traits>& b)
{
// this is not a meaningful assertion given that surfaces generally don't have meaningful normals ...
//    assert(parallel(a.attributes().normal(), b.attributes().normal()) && to_plane(a).contains(b.faces().front().halfedge().vertex_attributes().position()));

    a.reset_all();
    b.reset_all();

    auto& mesh = a.mesh();
brep::dump_faces(mesh);

//    auto surface_border_selector = [&mesh](id_type id_a, id_type id_b) -> bool { return mesh.halfedge(id_a).surface_border() || mesh.halfedge(id_b).surface_border(); };
    auto surface_border_selector = [&mesh](id_type, id_type id_b) -> bool { return mesh.halfedge(id_b).surface_border(); };
    auto intersections = perimeter_intersections(a, b, surface_border_selector);
brep::dump_faces(mesh);
print_intersections(mesh, intersections);
    if (intersections.empty())
    {
        return {a.id()};
    }

    // intersections[outer] = inner

    Splits<Traits> splits;

    // split/trace_intersections ...
    for (const auto& intersection : intersections)
    {
        id_type id_outer = intersection.first;
        if (mesh.halfedge(id_outer).surface_id() != a.id()) // filter to only those intersections that will alter surface a (can perimeter_intersections now handle this? ...)
        {
            continue;
        }

        id_type id_trace = intersection.second;
        std::vector<typename Traits::point_type> positions{mesh.halfedge(id_trace).vertex_attributes().position()};
        do
        {
            id_trace = mesh.halfedge(id_trace).next_id();
            positions.push_back(mesh.halfedge(id_trace).vertex_attributes().position());
        }
        while (!mesh.halfedge(id_trace).compared());

        assert(mesh.halfedge(id_trace).comparison() >= 0);

        id_type id_inner = intersections[id_trace];
        splits.emplace_back(id_inner, id_outer, positions);
    }

print_splits(splits);

    std::set<id_type> ids_face_discard;

    for (const auto& split : splits)
    {
        // split face and decouple new halfedges
        split_face(mesh, split.id_outer, split.id_inner, split.positions, false);
std::cout << "split_face outer: " << split.id_outer << ", inner: " << split.id_inner << std::endl;
brep::dump_faces(mesh);
        if (ids_face_discard.contains(mesh.halfedge(split.id_outer).face_id()))
        {
std::cout << "ids_face_discard.erase id: " << mesh.halfedge(split.id_outer).face_id() << std::endl;
            ids_face_discard.erase(mesh.halfedge(split.id_outer).face_id());
        }
std::cout << "ids_face_discard.insert id: " << mesh.halfedge(split.id_inner).face_id() << std::endl;
        ids_face_discard.insert(mesh.halfedge(split.id_inner).face_id());
    }
for (const auto& id : ids_face_discard)
{
    std::cout << "ids_face_discard id: " << id << std::endl;
}

    // discard a faces that overlap b ...
//std::cout << "discard" << std::endl;
//mesh.delete_surface(b.id()); // discard b ...
    for (const auto& id_face : ids_face_discard)
    {
//std::cout << id_face << std::endl;
        if (!mesh.face(id_face).deleted())
        {
            delete_border_fragment(mesh, mesh.face(id_face).halfedge_id(), [](const Halfedge<Traits>& halfedge) -> bool { return halfedge.surface_border(); });
        }
    }
brep::dump_faces(mesh);

    return {a.id()};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::id_type> quetzal::brep::op_symmetric_difference(Surface<Traits>& a, Surface<Traits>& b)
{
    assert(parallel(a.attributes().normal(), b.attributes().normal()) && to_plane(a).contains(b.faces().front().halfedge().vertex_attributes().position()));

    a.reset_all();
    b.reset_all();

    auto intersections = perimeter_intersections(a, b);
    if (intersections.empty())
    {
        return {a.id(), b.id()};
    }

    // ...

    return {}; // ...
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::edge_intersections_type quetzal::brep::perimeter_intersections(Surface<Traits>& a, const Surface<Traits>& b, surface_border_selector_type surface_border_selector, std::ostream& os)
{
    // Could use RTree, but might not be worth it for this few edges ...
    indexes_type<typename Traits::vector_traits> indexes;
    surface_detail_bounds(a, indexes);
    surface_detail_bounds(b, indexes);

    auto& [surface_bounds, face_bounds, halfedge_bounds] = indexes;
    if (disjoint(surface_bounds[a.id()], surface_bounds[b.id()]))
    {
        return {};
    }

    auto& mesh = a.mesh();

    std::set<std::pair<id_type, id_type>> pairs_checked;
    edge_intersections_type intersections;

    for (auto& face_a : a.faces())
    {
        auto bounds_face_a = face_bounds[face_a.id()];
        auto bounds_face_a_surface_b = op_intersection(bounds_face_a, surface_bounds[b.id()]);
        if (bounds_face_a_surface_b.empty())
        {
            continue;
        }

        std::vector<id_type> ids_ha; // dynamic face a halfedge ids
        for (auto& halfedge_a : face_a.halfedges())
        {
            ids_ha.push_back(halfedge_a.id());
        }

        for (auto& face_b : b.faces())
        {
            auto bounds_face_b = face_bounds[face_b.id()];
            auto bounds_face_b_surface_a = op_intersection(bounds_face_b, surface_bounds[a.id()]);
            if (bounds_face_b_surface_a.empty())
            {
                continue;
            }

            if (op_intersection(bounds_face_a, bounds_face_b).empty())
            {
                continue;
            }

            std::vector<id_type> ids_hb; // dynamic face b halfedge ids
            for (auto& halfedge_b : face_b.halfedges())
            {
                ids_hb.push_back(halfedge_b.id());
            }

os << "a\t" << ids_ha.size() << std::endl;
            for (size_t ia = 0; ia < ids_ha.size(); ++ia)
            {
                id_type id_a = ids_ha[ia];
os << "aa\t" << ia << '/' << ids_ha.size() << ": " << id_a << std::endl;
                if (op_intersection(halfedge_bounds[id_a], bounds_face_b).empty())
                {
                    continue;
                }

                auto segment_a = to_segment(mesh.halfedge(id_a));

os << "b\t" << ids_hb.size() << std::endl;
                for (size_t ib = 0; ib < ids_hb.size(); ++ib)
                {
                    id_type id_b = ids_hb[ib];
os << "    " << ia << '/' << ids_ha.size() << ": " << id_a << "\t(" << segment_a << ")\t" << ib << '/' << ids_hb.size() << ": "  << id_b << "\t(" << to_segment(mesh.halfedge(id_b)) << ')';

                    if (pairs_checked.contains({id_a, id_b}))
                    {
os << "\t- (a)" << std::endl;
                        continue;
                    }

                    // Consider only intersections on edges of surfaces, not strictly internal intersections
// pass this function in for difference to filter just single surface_border ...
// is difference the only operation that will do anything different from this? asymmetric ...
// union obly needs a single surface_order (depending on reattach algorithm)  ...
//                    if (!mesh.halfedge(id_a).surface_border() && !mesh.halfedge(id_b).surface_border())
                    if (!surface_border_selector(id_a, id_b))
                    {
os << "\t- (b)" << std::endl;
                        continue;
                    }

                    if (op_intersection(halfedge_bounds[id_a], halfedge_bounds[id_b]).empty())
                    {
os << "\t- (c)" << std::endl;
                        continue;
                    }

                    // *may be a surface seam, partner nullid ...

                    auto segment_b = to_segment(mesh.halfedge(id_b));
                    auto inter = intersection(segment_a, segment_b);
                    if (inter.locus() == geometry::Locus::Empty)
                    {
os << "\t- (d)" << std::endl;
                        continue;
                    }
                    else if (inter.locus() == geometry::Locus::Point)
                    {
                        const auto& position = inter.point();
os << "\t*\t" << position << std::endl;
                        // Do not consider intersections at the halfedge end points (beginning of next halfedges)
                        if (float_eq(position, segment_a.endpoint(1)) || float_eq(position, segment_b.endpoint(1)))
                        {
os << "X (p1)" << std::endl;
                            continue;
                        }

                        auto [id_inter_a, id_inter_b] = intersection_point(mesh, id_a, id_b, position);
                        pairs_checked.insert({id_a, id_b});
os << " -> " << id_inter_a << '\t' << id_inter_b << std::endl;

                        auto& halfedge_inter_a = mesh.halfedge(id_inter_a);
                        auto& halfedge_inter_b = mesh.halfedge(id_inter_b);

                        if (mesh.halfedge(id_a).surface_border())
                        {
                            halfedge_inter_a.set_comparison(halfedge_face_orientation(halfedge_inter_a, halfedge_inter_b));
                            halfedge_inter_b.set_comparison(halfedge_face_orientation(halfedge_inter_b, halfedge_inter_a));

                            if (halfedge_inter_a.comparison() < 0)
                            {
                                assert(halfedge_inter_b.comparison() > 0);
                                intersections[id_inter_b] = id_inter_a;
                            }
                            else
                            {
                                assert(halfedge_inter_a.comparison() > 0);
                                intersections[id_inter_a] = id_inter_b;
                            }

                            if (id_inter_a != id_a)
                            {
                                ids_ha.push_back(id_inter_a);
                                halfedge_bounds[id_a] = bounds(mesh.halfedge(id_a));
                                halfedge_bounds[id_inter_a] = bounds(halfedge_inter_a);
                                pairs_checked.insert({id_inter_a, id_b});
                            }

                            if (id_inter_b != id_b)
                            {
                                ids_hb.push_back(id_inter_b);
                                halfedge_bounds[id_b] = bounds(mesh.halfedge(id_b));
                                halfedge_bounds[id_inter_b] = bounds(halfedge_inter_b);
                                pairs_checked.insert({id_a, id_inter_b});
                                if (id_inter_a != id_a) pairs_checked.insert({id_inter_a, id_inter_b});
                            }
                        }
                        else
                        {
                            id_type id_a_p = mesh.halfedge(id_inter_a).partner_id();
                            id_type id_inter_a_p = mesh.halfedge(id_a).partner_id();
os << " -> " << id_inter_a_p << '\t' << id_inter_b << std::endl;

                            auto& halfedge_a_p = mesh.halfedge(id_a_p);
                            auto& halfedge_inter_a_p = mesh.halfedge(id_inter_a_p);

                            halfedge_inter_a.set_comparison(halfedge_face_orientation(halfedge_inter_a, halfedge_inter_b));
                            halfedge_inter_a_p.set_comparison(-halfedge_inter_a.comparison());
                            halfedge_inter_b.set_comparison(0);

                            if (halfedge_inter_a.comparison() < 0)
                            {
                                intersections[id_inter_a_p] = id_inter_b;
                                intersections[id_inter_b] = id_inter_a;
                            }
                            else
                            {
                                intersections[id_inter_a] = id_inter_b;
                                intersections[id_inter_b] = id_inter_a_p;
                            }

                            if (id_inter_a != id_a)
                            {
                                ids_ha.push_back(id_inter_a);
                                ids_ha.push_back(id_inter_a_p);
                                halfedge_bounds[id_a] = bounds(mesh.halfedge(id_a));
                                halfedge_bounds[id_inter_a] = bounds(halfedge_inter_a);
                                halfedge_bounds[id_a_p] = bounds(halfedge_a_p);
                                halfedge_bounds[id_inter_a_p] = bounds(halfedge_inter_a_p);
                                pairs_checked.insert({id_inter_a, id_b});
                                pairs_checked.insert({id_inter_a_p, id_b});
                            }

                            if (id_inter_b != id_b)
                            {
                                ids_hb.push_back(id_inter_b);
                                halfedge_bounds[id_b] = bounds(mesh.halfedge(id_b));
                                halfedge_bounds[id_inter_b] = bounds(halfedge_inter_b);
                                pairs_checked.insert({id_a, id_inter_b});
                                pairs_checked.insert({id_a_p, id_b});
                                if (id_inter_a != id_a)
                                {
                                    pairs_checked.insert({id_inter_a, id_inter_b});
                                    pairs_checked.insert({id_inter_a_p, id_inter_b});
                                }
                            }
                        }
                    }
                    else // Segment
                    {
os << "X (s)" << std::endl;
                        const auto& segment = inter.segment();
                        intersection_point(mesh, id_a, id_b, segment.endpoint(0));
                        intersection_point(mesh, id_a, id_b, segment.endpoint(1));
                        // handle the case where there are two splits, one segment interior to the other ...
                        // maybe doing it in a specific order will work? ...
                        // *draw this out ...

                        // add new halfedge to idfxs ...
                    }
                }
            }
        }
    }

    return intersections;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::surface_detail_bounds(const Surface<Traits>& surface, indexes_type<typename Traits::vector_traits>& indexes)
{
    auto& [surface_bounds, face_bounds, halfedge_bounds] = indexes;

    for (auto& face : surface.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            if (halfedge.checked())
            {
                continue;
            }

            auto& box = halfedge_bounds[halfedge.id()];
            box.insert(halfedge.vertex_attributes().position());
            box.insert(halfedge.next().vertex_attributes().position());
            halfedge.set_checked();

            face_bounds[halfedge.face_id()].insert(box);

            if (!halfedge.surface_border())
            {
                halfedge_bounds[halfedge.partner_id()] = box;
                halfedge.partner().set_checked();
                face_bounds[halfedge.partner().face_id()].insert(box);
            }
        }

        surface_bounds[surface.id()].insert(face_bounds[face.id()]);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::array<quetzal::id_type, 2> quetzal::brep::intersection_point(Mesh<Traits>& mesh, id_type halfedge_id_a, id_type halfedge_id_b, const typename Traits::point_type& position)
{
std::cout << "intersection_point\t" << halfedge_id_a << '\t' << halfedge_id_b << '\t' << position << std::endl;
std::cout << "    segment a\t" << to_segment(mesh.halfedge(halfedge_id_a)) << std::endl;
std::cout << "    segment b\t" << to_segment(mesh.halfedge(halfedge_id_b)) << std::endl;

    // handle both border and surface_border edges here ...
    // split_edge handles both? ...

    if (position != mesh.halfedge(halfedge_id_a).vertex_attributes().position())
    {
        if (mesh.halfedge(halfedge_id_a).border())
        {
            split_halfedge(mesh, halfedge_id_a, position);
        }
        else
        {
            split_edge(mesh, halfedge_id_a, position);
        }

        halfedge_id_a = mesh.halfedge(halfedge_id_a).next_id();
    }

    if (position != mesh.halfedge(halfedge_id_b).vertex_attributes().position())
    {
        if (mesh.halfedge(halfedge_id_b).border())
        {
            split_halfedge(mesh, halfedge_id_b, position);
        }
        else
        {
            split_edge(mesh, halfedge_id_b, position);
        }

        halfedge_id_b = mesh.halfedge(halfedge_id_b).next_id();
    }
/*
    // is the && correct? ...
    if (!mesh.halfedge(halfedge_id_a).surface_border() && !mesh.halfedge(halfedge_id_b).surface_border())
    {
        intersections[mesh.halfedge(halfedge_id_a).partner().next_id()] = mesh.halfedge(halfedge_id_b).partner().next_id();
    }
*/
    return {halfedge_id_a, halfedge_id_b};
}

//------------------------------------------------------------------------------
template<typename Traits>
int quetzal::brep::halfedge_face_orientation(const Halfedge<Traits>& halfedge, const Halfedge<Traits>& halfedge_face)
{
    // Works only for halfedges originating at intersection points
    const auto& face = halfedge_face.face();
    auto v = cross(to_vector(halfedge_face), face.attributes().normal());
    auto d = dot(v, to_vector(halfedge));
    return math::float_compare0(d);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::print_intersection(const Mesh<Traits>& mesh, const edge_intersection_type& intersection)
{
    const auto& ha = mesh.halfedge(intersection.first);
    const auto& hb = mesh.halfedge(intersection.second);
    std::cout << std::setw(3) << ha.id() << ' ' << static_cast<const Flags&>(ha) << '\t' << std::setw(3) << hb.id() << ' ' << static_cast<const Flags&>(hb) << std::endl;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::print_intersections(const Mesh<Traits>& mesh, const edge_intersections_type& intersections)
{
    std::cout << "intersections" << std::endl;
    for (const auto& intersection : intersections)
    {
        print_intersection(mesh, intersection);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::print_splits(const Splits<Traits>& splits)
{
    for (const auto& split : splits)
    {
        std::cout << "split " << split.id_inner << ' ' << split.id_outer;
        for (const auto& position : split.positions)
        {
            std::cout << " [" << position << ']';
        }
        std::cout << std::endl;
    }

    return;
}

#endif // QUETZAL_BREP_SURFACE_OPERATIONS_HPP
