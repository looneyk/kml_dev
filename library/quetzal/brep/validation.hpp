#if !defined(QUETZAL_BREP_VALIDATION_HPP)
#define QUETZAL_BREP_VALIDATION_HPP
//------------------------------------------------------------------------------
// brep
// validation.hpp
//------------------------------------------------------------------------------

#include "border.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "id.hpp"
#include "normal.hpp"
#include "texcoord.hpp"
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/Submesh.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/coordinate_system.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <iomanip>
#include <iostream>
#include <format>
#include <map>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_set>

namespace quetzal::brep
{

    constexpr int v_ulp = math::tolerance_default;
//    constexpr int v_ulp = 1;
    template<typename M>
    constexpr typename M::value_type v_value_max = M::val(1000);

    std::ostream& eol(std::ostream& os)
    {
        os << std::endl;
//        os << '\n'; // seems to improve speed by about half ...
        return os;
    }

    // One check that did not pass, recorded so that a caller can act on which elements deviated and in
    // what way rather than only on how many did
    struct Deviation
    {
        std::string m_check;
        id_type m_id = nullid;
        bool m_bad = false;
    };

    struct ValidationState
    {
        ValidationState(bool checked = false) :
            m_checked(checked),
            m_warn_count(0),
            m_bad_count(0)
        {
        }

        ValidationState& operator+=(const ValidationState& other)
        {
            m_warn_count += other.m_warn_count;
            m_bad_count += other.m_bad_count;
            m_deviations.append_range(other.m_deviations);
            return *this;
        }

        // Deviation count by check, which is what makes a count of 18 tell you whether it is one thing
        // eighteen times or eighteen different things
        std::map<std::string, size_t> check_counts() const
        {
            std::map<std::string, size_t> counts;
            for (const auto& deviation : m_deviations)
            {
                ++counts[deviation.m_check];
            }

            return counts;
        }

        // The elements that deviated on one check
        std::vector<id_type> check_ids(const std::string& check) const
        {
            std::vector<id_type> ids;
            for (const auto& deviation : m_deviations)
            {
                if (deviation.m_check == check)
                {
                    ids.push_back(deviation.m_id);
                }
            }

            return ids;
        }

        bool ok() const
        {
            return m_warn_count == 0 && m_bad_count == 0;
        }

        bool warn() const
        {
            return m_warn_count > 0 && m_bad_count == 0;
        }

        bool bad() const
        {
            return m_bad_count > 0;
        }

        // Return 1, 0, -1 for OK, Warn, Bad respectively
        int value() const
        {
            return bad() ? -1 : warn() ? 0 : 1;
        }

        // check names the deviation so that it can be counted and located; unnamed until a call site
        // has been given one
        void set_warn(const std::string& check = "unnamed", id_type id = nullid)
        {
            ++m_warn_count;
            m_deviations.emplace_back(check, id, false);
            return;
        }

        void set_bad(const std::string& check = "unnamed", id_type id = nullid)
        {
            ++m_bad_count;
            m_deviations.emplace_back(check, id, true);
            return;
        }

        bool m_checked = false; // do we need this info? the caller should know what it asked for ...
        size_t m_warn_count = 0;
        size_t m_bad_count = 0;
        std::vector<Deviation> m_deviations;
    };

    template<typename M>
    ValidationState validate(const M& mesh);

    template<typename M>
    ValidationState check_halfedge(const M& mesh, const typename M::halfedge_type& halfedge, bool bBorderOK = true);

    template<typename M>
    ValidationState check_halfedges(const M& mesh);

    template<typename M>
    ValidationState check_vertex(const M& mesh, const typename M::vertex_type& vertex);

    template<typename M>
    ValidationState check_vertices(const M& mesh);

    template<typename M>
    ValidationState check_face(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_faces(const M& mesh);

    template<typename M>
    ValidationState check_border_face(const M& mesh, id_type idHalfedge);

    // Checks that the merged perimeter/hole iteration visits every halfedge of the face exactly once,
    // the perimeter loop followed by each hole loop, and that it agrees with halfedge_ext_count
    template<typename M>
    ValidationState check_face_halfedges_ext(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_faces_halfedges_ext(const M& mesh);

    template<typename M>
    ValidationState check_surfaces(const M& mesh);

    template<typename M>
    ValidationState check_submeshes(const M& mesh);

    template<typename M>
    ValidationState check_triangular_face(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_triangulation(const M& mesh);

    template<typename M>
    ValidationState check_manifold_edges(const M& mesh);

    template<typename M>
    ValidationState check_adjacent_face_normals(const M& mesh);

    template<typename M>
    ValidationState check_adjacent_face_normals(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_split_edge(const M& mesh, const typename M::halfedge_type& halfedgeOrig, const typename M::halfedge_type& partnerOrig, const typename M::halfedge_type& halfedge);

    template<typename M>
    ValidationState check_split_face(const M& mesh, id_type idHalfedgeA, id_type idHalfedgeB, size_t nEdgesOrig);

    template<typename M>
    ValidationState check_attributes(const M& mesh);

    template<typename T>
    std::string halfedge_string(const T& halfedge);

    template<typename T>
    std::string vertex_string(const T& vertex);

    template<typename T>
    std::string face_string(const T& face);

    template<typename T>
    std::string surface_string(const T& surface);

    template<typename T>
    std::string submesh_string(const T& submesh);

    template<typename M>
    void write_halfedge(const typename M::halfedge_type& halfedge, const std::string& label = "");

    template<typename M>
    void write_vertex(const typename M::vertex_type& vertex, const std::string& label = "");

    template<typename M>
    void write_face(const typename M::face_type& face, const std::string& label = "");

    template<typename M>
    void write_face_vertices(const M& mesh, id_type idHalfedge);

    template<typename M>
    void write_hole_vertices(const M& mesh, id_type idHalfedge);

    template<typename M>
    void write_surface(const typename M::surface_type& surface, const std::string& label = "");

    template<typename M>
    void write_submesh(const typename M::submesh_type& submesh, const std::string& label = "");

    template<typename M>
    void write_summary(const M& mesh);

    template<typename M>
    void dump(const M& mesh);

    template<typename M>
    void dump_compact(const M& mesh);

    template<typename M>
    void dump_structure(const M& mesh);

    template<typename M>
    void dump_submesh(const M& mesh, id_type submesh_id);

    template<typename M>
    void dump_submeshes(const M& mesh);

    template<typename M>
    void dump_face(const M& mesh, id_type idFace);

    template<typename M>
    void dump_faces(const M& mesh);

    template<typename M>
    void mesh_info(const M& mesh);

    // Functions to validate mesh geometry
    // model/attributes_validation.hpp? ...

    // Calculates face normals and compares with each face vertex normal
    template<typename M>
    ValidationState check_face_normals(const M& mesh);

    // Calculates face normal and compares with each face vertex normal
    template<typename M>
    ValidationState check_face_normal(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_normal_consistency(const M& mesh);

    template<typename M>
    ValidationState check_normal_consistency(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_surface_normal_consistency(const M& mesh);

    template<typename M>
    ValidationState check_surface_normal_consistency(const M& mesh, const typename M::surface_type& surface);

    template<typename M>
    ValidationState check_surface_normal_consistency(const M& mesh, const typename M::surface_type& surface, const typename M::vertex_type& vertex);

    template<typename M>
    ValidationState check_surface_vertex_consistency_exact(const M& mesh);

    template<typename M>
    ValidationState check_face_angles(const M& mesh);

    template<typename M>
    ValidationState check_face_angles(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_convex(const M& mesh);

    template<typename M>
    ValidationState check_convex(const M& mesh, const typename M::face_type& face);

    template<typename M>
    ValidationState check_outward_normals(const M& mesh);

    template<typename Traits>
    ValidationState check_spherical(const Mesh<Traits>& mesh, typename Traits::value_type radius);

    template<typename Traits>
    ValidationState check_spherical(const Submesh<Traits>& submesh, typename Traits::value_type radius);

    template<typename M>
    std::map<typename M::value_type, size_t> count_edge_lengths(const M& mesh); // Returns count of each distinct length

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::validate(const M& mesh)
{
    auto state = check_halfedges(mesh);
    state += check_vertices(mesh);
    state += check_faces(mesh);
    state += check_surfaces(mesh);
    state += check_submeshes(mesh);
    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_halfedge(const M& mesh, const typename M::halfedge_type& halfedge, bool bBorderOK)
{
    ValidationState state;
    if (halfedge.deleted())
    {
        return state;
    }

    std::ostringstream oss;

    if (mesh.halfedge(halfedge.id()).id() != halfedge.id())
    {
        oss << "  Bad halfedge id " << "mesh.halfedge(halfedge.id()).id() != halfedge.id() " << mesh.halfedge(halfedge.id()).id() << ' ' << halfedge.id() << eol;
        state.set_bad();
    }

    if (halfedge.partner_id() == nullid)
    {
        oss << "  Border" << eol;
        if (!bBorderOK)
        {
            state.set_bad();
        }
    }
    else if (!good(halfedge.partner_id(), mesh.halfedge_store()))
    {
        oss << "  Bad partner " << id_string(halfedge.partner_id(), mesh.halfedge_store()) << eol;
        state.set_bad();
    }

    if (!good(halfedge.next_id(), mesh.halfedge_store()))
    {
        oss << "  Bad next " << id_string(halfedge.next_id(), mesh.halfedge_store()) << eol;
        state.set_bad();
    }

    if (!good(halfedge.prev_id(), mesh.halfedge_store()))
    {
        oss << "  Bad prev " << id_string(halfedge.prev_id(), mesh.halfedge_store()) << eol;
        state.set_bad();
    }

    if (!good(halfedge.vertex_id(), mesh.vertex_store()))
    {
        oss << "  Bad vertex " << id_string(halfedge.vertex_id(), mesh.vertex_store()) << eol;
        state.set_bad();
    }

    if (!good(halfedge.face_id(), mesh.face_store()))
    {
        oss << "  Bad face " << id_string(halfedge.face_id(), mesh.face_store()) << eol;
        state.set_bad();
    }
    
    if (float_eq(halfedge.vertex_attributes().position(), halfedge.next().vertex_attributes().position(), v_ulp))
    {
        const auto& pos1 = halfedge.vertex_attributes().position();
        const auto& pos2 = halfedge.next().vertex_attributes().position();
        oss << "Bad: float_eq(halfedge.vertex_attributes().position(), halfedge.next().vertex_attributes().position()) " << halfedge.id() << ' ' << halfedge.next_id() << " [" << pos1 << "] [" << pos2 << ']' << eol;
        state.set_bad();
    }

//    if (!halfedge.deleted() && !halfedge.border() && good(halfedge.partner_id(), mesh.halfedge_store()))
    if (good(halfedge.partner_id(), mesh.halfedge_store()))
    {
        const auto& partner = halfedge.partner();
        if (!(halfedge.partner_id() == partner.id()))
        {
            oss << "Bad: !(halfedge.partner_id() == partner.id()" << halfedge.partner_id() << ' ' << partner.id() << eol;
            state.set_bad();
        }
        if (!(float_eq(halfedge.vertex_attributes().position(), partner.next().vertex_attributes().position(), v_ulp)))
        {
            const auto& pos1 = halfedge.vertex_attributes().position();
            const auto& pos2 = partner.next().vertex_attributes().position();
            oss << "Bad: !(float_eq(halfedge.vertex_attributes().position(), partner.next().vertex_attributes().position()) " << halfedge.id() << ' ' << partner.next_id() << " [" << pos1 << "] [" << pos2 << ']' << eol;
            state.set_bad();
        }
        if (!float_eq(halfedge.next().vertex_attributes().position(), partner.vertex_attributes().position(), v_ulp))
        {
            const auto& pos1 = halfedge.next().vertex_attributes().position();
            const auto& pos2 = partner.vertex_attributes().position();
            oss << "Bad: !(float_eq(halfedge.next().vertex_attributes().position(), partner.vertex_attributes().position()) " << halfedge.next_id() << ' ' << partner.id() << " [" << pos1 << "] [" << pos2 << ']' << eol;
            state.set_bad();
        }
        if (halfedge.prev().partner_id() != nullid
            && !(float_eq(halfedge.vertex_attributes().position(), halfedge.prev().partner().vertex_attributes().position(), v_ulp)))
        {
            const auto& pos1 = halfedge.vertex_attributes().position();
            const auto& pos2 = halfedge.prev().partner().vertex_attributes().position();
            oss << "Bad: !(float_eq(halfedge.vertex_attributes().position(), halfedge.prev().partner().vertex_attributes().position()) " << halfedge.id() << ' ' << halfedge.prev().partner_id() << " [" << pos1 << "] [" << pos2 << ']' << eol;
            state.set_bad();
        }
        if (partner.prev().partner_id() != nullid
            && !(float_eq(partner.vertex_attributes().position(), partner.prev().partner().vertex_attributes().position(), v_ulp)))
        {
            const auto& pos1 = partner.vertex_attributes().position();
            const auto& pos2 = partner.prev().partner().vertex_attributes().position();
            oss << "Bad: !(float_eq(partner.vertex_attributes().position(), partner.prev().partner().vertex_attributes().position()) " << partner.id() << ' ' << partner.prev().partner_id() << " [" << pos1 << "] [" << pos2 << ']' << eol;
            state.set_bad();
        }
    }

    if (!state.ok())
    {
        std::cout << "Halfedge " << halfedge.id() << eol << oss.str();
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_halfedges(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking halfedges: " << mesh.halfedge_count() << '/' << mesh.halfedge_store_count() << " halfedges" << eol;
    for (const auto& halfedge : mesh.halfedges())
    {
        state += check_halfedge<M>(mesh, halfedge);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_vertex(const M& mesh, const typename M::vertex_type& vertex)
{
    ValidationState state;
    if (vertex.deleted())
    {
        return state;
    }

    // Arbitrary maximum check for realistic values, not a good test in general
    typename M::value_type coordMax = v_value_max<M>;
    const auto position = vertex.attributes().position();
    if (std::abs(position.x()) > coordMax || std::abs(position.y()) > coordMax || std::abs(position.z()) > coordMax)
    {
        std::cout << "Bad: coordinate > " << coordMax << ": " << vertex.id() << position << eol;
        state.set_bad();
    }

    std::string errors;
    bool ok = vertex.attributes().validate(errors);
    if (!ok)
    {
        // restore attributes texcoord validation to 0..1 and filter out texcoord range errors here ...
        std::cout << "Bad: vertex " << vertex.id() << " attributes: " << errors << eol;
        state.set_bad();
    }

    if (mesh.vertex(vertex.id()).id() != vertex.id())
    {
        std::cout << "  Bad vertex id " << "mesh.vertex(vertex.id()).id() != vertex.id() " << mesh.vertex(vertex.id()).id() << ' ' << vertex.id() << eol;
        state.set_bad();
    }

    // A border vertex is checked like any other apart from the fan walk below, which cannot close
    // through a null partner
    const bool border = vertex.halfedge().border();
    if (border)
    {
        std::cout << "  Border halfedge, vertex " << vertex.id() << " face " << vertex.halfedge().face_id() << eol;
        state.set_warn("border halfedge", vertex.id());
    }

    if (!good(vertex.halfedge_id(), mesh.halfedge_store()))
    {
        std::cout << "  Bad halfedge " << id_string(vertex.halfedge_id(), mesh.halfedge_store()) << eol;
        state.set_bad();
    }

    // This is just an artifact of the 1-1 pairing of halfedges and vertices
    if (mesh.halfedge_store_count() != mesh.vertex_store_count())
    {
        std::cout << "  Bad mesh.halfedge_store_count() != mesh.vertex_store_count() " << mesh.halfedge_store_count() << ' ' << mesh.vertex_store_count() << eol;
        state.set_bad();
    }

    // This is just an artifact of the 1-1 pairing of halfedges and vertices
    if (mesh.halfedge_count() != mesh.vertex_count())
    {
        std::cout << "  Bad mesh.halfedge_count() != mesh.vertex_count() " << mesh.halfedge_count() << ' ' << mesh.vertex_count() << eol;
        state.set_bad();
    }

    // This is just an artifact of the 1-1 pairing of halfedges and vertices, but this will fail if inversion is performed
    if (vertex.halfedge_id() != vertex.id())
    {
        std::cout << "  Warn: vertex halfedge id match " << id_string(vertex.halfedge_id(), mesh.halfedge_store()) << ' ' << id_string(vertex.id(), mesh.halfedge_store()) << eol;
        state.set_warn("vertex halfedge id match", vertex.id());
    }

    if (border)
    {
        return state; // Everything above has been checked; only the fan walk below cannot be
    }

    // Shared vertices could make this test moot
    for (const auto& halfedge : vertex.halfedges())
    {
        if (halfedge.vertex_attributes().position() != vertex.attributes().position())
        {
            std::cout << "  Warn: coincident vertex position: reference vertex " << vertex.id() << " (" << vertex.attributes().position() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().position() << ')' << eol;
            state.set_warn("coincident vertex position", vertex.id());
        }

        if (halfedge.surface_id() != vertex.halfedge().surface_id())
        {
            continue;
        }

        if (halfedge.vertex_attributes().normal() != vertex.attributes().normal())
        {
            std::cout << "  Warn: coincident surface vertex normal: reference vertex " << vertex.id() << " (" << vertex.attributes().normal() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().normal() << ')' << eol;
            state.set_warn("coincident surface vertex normal", vertex.id());
        }

        if (!texcoord_eq(halfedge.vertex_attributes().texcoord(), vertex.attributes().texcoord()))
        {
            std::cout << "  Warn: coincident surface vertex texcoord: reference vertex " << vertex.id() << " (" << vertex.attributes().texcoord() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().texcoord() << ')' << eol;
            state.set_warn("coincident surface vertex texcoord", vertex.id());
        }
    }

/*
    // will be same but could be wrong with multiple border sections, need a better test for this ...
    if (vertex.halfedges().size() != vertex.halfedge_count())
    {
        std::cout << "Bad: vertex.halfedges().size() != vertex.halfedge_count() " << std::setw(4) << vertex.halfedges().size() << ' ' << vertex.halfedge_count() << eol;
        state.set_bad();
    }
*/
    size_t n = 0;
    std::vector<id_type> ids_ordered;
    std::unordered_set<id_type> ids;
    for (const auto& halfedge : vertex.halfedges())
    {
        if (ids.contains(halfedge.id()))
        {
            std::cout << "Bad: halfedge iterator: duplicate id at n " << n << ", v " << vertex.id() << ": h " << halfedge.id() << eol;
            std::ranges::for_each(ids_ordered, [](auto id) { std::cout << id << ' '; });
            state.set_bad();
            break;
        }

        ids.insert(halfedge.id());

        if (halfedge.partner_id() == nullid)
        {
            std::cout << "Warn: halfedge iterator: halfedge.partner_id() == nullid" << " v " << vertex.id() << " h " << halfedge.id() << eol;
            state.set_warn("halfedge iterator, null partner", vertex.id());
            break;
        }

        const auto& attributes = halfedge.vertex_attributes();
        if (!float_eq(attributes.position(), position))
        {
            std::cout << "Bad: !float_eq(attributes.position(), position) : " << vertex.id() << " [" << attributes.position() << "] [" << position << ']' << eol;
            state.set_bad();
        }
/* work on this ...
        else if (attributes.position() != position)
        {
            std::cout << "Warn: attributes.position() != position : " << vertex.id() << " [" << attributes.position() << "] [" << position << ']' << eol;
            state.set_warn();
        }
*/
        if (!float_eq(halfedge.vertex_attributes().position(), halfedge.partner().next().vertex_attributes().position(), v_ulp))
        {
            std::cout << "Bad: !float_eq(halfedge.vertex_attributes().position(), halfedge.partner().next().vertex_attributes().position())" << ' ' << halfedge.id() << ' ' << halfedge.partner().next_id() << " [" << halfedge.vertex_attributes().position() << "] [" << halfedge.partner().next().vertex_attributes().position() << ']' << eol;
            state.set_bad();
        }

        if (halfedge.prev().partner_id() == nullid)
        {
            std::cout << "Warn: halfedge iterator: halfedge.prev().partner_id() == nullid" << " v " << vertex.id() << " h " << halfedge.id() << eol;
            state.set_warn("halfedge iterator, null prev partner", vertex.id());
            break;
        }

        if (!float_eq(halfedge.vertex_attributes().position(), halfedge.prev().partner().vertex_attributes().position(), v_ulp))
        {
            std::cout << "Bad: !float_eq(halfedge.vertex_attributes().position(), halfedge.prev().partner().vertex_attributes().position())" << ' ' << halfedge.id() << ' ' << halfedge.prev().partner_id() << " [" << halfedge.vertex_attributes().position() << "] [" << halfedge.prev().partner().vertex_attributes().position() << ']' << eol;
            state.set_bad();
        }

        ++n;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_vertices(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking vertices: " << mesh.vertex_count() << '/' << mesh.vertex_store_count() << " vertices" << eol;
    for (const auto& vertex : mesh.vertices())
    {
        state += check_vertex(mesh, vertex);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face(const M& mesh, const typename M::face_type& face)
{
    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    std::map<id_type, size_t> counts;
    size_t halfedge_count = 0;

    auto normal = face.attributes().normal();
    if (math::float_ne(normal.norm(), M::val(1)))
    {
        std::cout << "Bad: |normal| != 1:" << '\t' << face.id() << '\t' << normal << eol;
        state.set_bad();
    }

/*
    if (!planar(face))
    {
        std::cout << "Warn: !planar(face):" << '\t' << face.id() << eol;
        state.set_warn();
    }
    else
    {
        auto normalPolygon = to_polygon(face).normal();
        if (!float_eq(normal, normalPolygon)) // tolerance ...
        {
            std::cout << "Warn: normal != polygon normal:" << '\t' << face.id() << "\t[" << normal << "]\t[" << normalPolygon << ']' << eol;
            state.set_warn();
        }
    }
*/

    if (!good(face.halfedge_id(), mesh.halfedge_store()))
    {
        std::cout << "  Bad halfedge " << id_string(face.halfedge_id(), mesh.halfedge_store()) << eol;
        state.set_bad();
    }

    if (mesh.face(face.id()).id() != face.id())
    {
        std::cout << "  Bad face id " << "mesh.face(face.id()).id() != face.id() " << mesh.face(face.id()).id() << ' ' << face.id() << eol;
        state.set_bad();
    }

    size_t edge_count = face.halfedge_count();
    if (edge_count == 2)
    {
        std::cout << "Warn: face.halfedge_count() == 2 :" << '\t' << face.id() << '\t' << edge_count << eol;
        state.set_warn("degenerate face, halfedge_count == 2", face.id());
    }
    else if (edge_count < 2)
    {
        std::cout << "Bad: face.halfedge_count() < 2 :" << '\t' << face.id() << '\t' << edge_count << eol;
        state.set_bad();
    }

    for (const auto& halfedge : face.halfedges())
    {
        if (halfedge.face_id() == face.id())
        {
            ++halfedge_count;
        }

        if (halfedge.face_id() != face.id())
        {
            std::cout << "Bad: halfedge.face_id() != face.id()" << '\t' << halfedge.face_id() << '\t' << face.id() << eol;
            state.set_bad();
        }

        // A loop belongs to one face throughout, so a disagreement across next is where a reassignment
        // stopped short rather than the loop genuinely leaving the face
        if (halfedge.face_id() != halfedge.next().face_id())
        {
            std::cout << "Bad: halfedge " << halfedge.id() << " face " << halfedge.face_id() << " != next " << halfedge.next_id() << " face " << halfedge.next().face_id() << eol;
            state.set_bad("halfedge face differs from next", face.id());
        }

        if (direction_compare(halfedge.vertex_attributes().normal(), face.attributes().normal()) <= 0)
        {
            std::cout << "Bad: direction_compare(halfedge.vertex_attributes().normal(), face.attributes().normal()) <= 0" << " f " << face.id() << " h " << halfedge.id() << " [" << halfedge.vertex_attributes().normal() << "] [" << face.attributes().normal() << ']' << eol;
            state.set_bad();
        }

        if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
        {
            if (edge_count >= 3)
            {
                auto to_point = [](const typename M::vertex_attributes_type::texcoord_type& t) -> typename M::point_type { return {t.x(), t.y(), M::val(0)}; };

                // This is fine for triangulated models, but does not handle reflex vertices correctly ...
                auto t0 = to_point(halfedge.vertex_attributes().texcoord());
                auto t1 = to_point(halfedge.next().vertex_attributes().texcoord());
                auto t2 = to_point(halfedge.prev().vertex_attributes().texcoord());

                // Colinear corner texcoords leave no winding to test, so the corner says nothing either
                // way. The spur left by a partial edge is the case that arises here: its tip halfedge has
                // next and prev both at the anchor position carrying the same texcoord, collapsing the
                // three points to two. Colinearity is judged relative to the magnitudes involved, which
                // a comparison against the cross product alone cannot do
                if (!colinear(t0, t1, t2))
                {
                    // Normalized because direction_compare requires a non-degenerate vector, which the
                    // raw cross product of two texcoord differences is not once they get small
                    auto texcoord_normal = normalize(cross(t1 - t0, t2 - t0));

                    // cross(next - this, prev - this) turns the other way at a reflex vertex, so the sign a
                    // correctly wound face gives depends on which kind of corner this is
                    const bool texcoord_reflex = reflex(halfedge);

                    if (auto comparison = direction_compare(texcoord_normal, M::vector_type::z_direction()); texcoord_reflex ? comparison <= 0 : comparison >= 0)
                    {
                        std::cout << "Warn: texcoord winding : " << face.id() << ' ' << halfedge.id() << ' ' << texcoord_normal << (texcoord_reflex ? " (reflex)" : " (convex)") << eol;
                        state.set_warn("texcoord winding", face.id());
                    }
                }
/*
                typename M::value_type projection = dot(texture_direction, M::vector_type::z_direction());
                if (convex(halfedge) && math::float_le0(projection))
                {
                    std::cout << "Bad: convex dot(texture_direction, M::vector_type::z_direction()) <= 0" << " f " << halfedge.face_id() << " h " << halfedge.id() << " [" << texture_direction << "]: [" << halfedge.vertex_attributes().position() << "] [" << halfedge.vertex_attributes().texcoord() << ']' << eol;
                    std::cout << '(' << texcoord << ") (" << texcoordNext << ") (" << texcoordPrev << ") " << eol;
                    state.set_bad();
                }
                else if (reflex(halfedge) && math::float_ge0(projection))
                {
                    std::cout << "Bad: reflex dot(texture_direction, M::vector_type::z_direction()) >= 0" << " f " << halfedge.face_id() << " h " << halfedge.id() << " [" << texture_direction << "]: [" << halfedge.vertex_attributes().position() << "] [" << halfedge.vertex_attributes().texcoord() << ']' << eol;
                    state.set_bad();
                }
*/
            }
        }

        auto position = halfedge.vertex_attributes().position();
        auto position_next = halfedge.next().vertex_attributes().position();
        if (float_eq(position, position_next))
        {
            std::cout << "Bad: duplicated next vertex position: " << halfedge.id() << " [" << position << "] " << halfedge.next_id() << " [" << position_next << ']' << eol;
            state.set_bad();
        }

        ++counts[halfedge.vertex_id()];
    }

    if (halfedge_count != face.halfedge_count())
    {
        std::cout << "    Bad halfedge face ids, " << halfedge_count << " / " << face.halfedge_count() << " face id " << face.id() << " references found." << eol;
    }

    if (counts.size() < face.halfedge_count())
    {
        std::cout << "Bad: counts.size() < face.halfedge_count()" << eol;
        state.set_bad();
    }

    // Holes are not reached by the perimeter iteration above, so they are checked here. Each hole loop
    // has to close, name this face throughout, and be disjoint from the perimeter and from every other
    // hole of the face. A hole of two halfedges is a legitimate degenerate case, the pair being its own
    // partner chain, so it is checked as such rather than rejected for being short
    std::map<id_type, size_t> hole_halfedge_counts;

    std::map<id_type, size_t> perimeter_halfedge_counts;
    for (const auto& halfedge : face.halfedges())
    {
        ++perimeter_halfedge_counts[halfedge.id()];
    }

    for (const auto& hole : face.holes())
    {
        size_t n = 0;
        for (const auto& halfedge : hole.halfedges())
        {
            ++n;

            if (halfedge.face_id() != face.id())
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " face " << halfedge.face_id() << " != " << face.id() << eol;
                state.set_bad("hole halfedge face id", face.id());
            }

            if (perimeter_halfedge_counts.contains(halfedge.id()))
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " is on the perimeter of face " << face.id() << eol;
                state.set_bad("hole halfedge on perimeter", face.id());
            }

            if (++hole_halfedge_counts[halfedge.id()] > 1)
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " belongs to more than one hole of face " << face.id() << eol;
                state.set_bad("hole halfedge shared between holes", face.id());
            }

            if (halfedge.hole_id() != hole.id())
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " hole " << id_string(halfedge.hole_id()) << " != " << id_string(hole.id()) << " on face " << face.id() << eol;
                state.set_bad("hole halfedge hole id", face.id());
            }

            // As for the perimeter, a loop carries one face and one hole the whole way round
            if (halfedge.face_id() != halfedge.next().face_id())
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " face " << halfedge.face_id() << " != next " << halfedge.next_id() << " face " << halfedge.next().face_id() << eol;
                state.set_bad("hole halfedge face differs from next", face.id());
            }

            if (halfedge.hole_id() != halfedge.next().hole_id())
            {
                std::cout << "Bad: hole halfedge " << halfedge.id() << " hole " << id_string(halfedge.hole_id()) << " != next " << halfedge.next_id() << " hole " << id_string(halfedge.next().hole_id()) << eol;
                state.set_bad("hole halfedge hole differs from next", face.id());
            }
        }

        if (n != hole.halfedge_count())
        {
            std::cout << "Bad: hole walk " << n << " != hole.halfedge_count() " << hole.halfedge_count() << " face " << face.id() << eol;
            state.set_bad("hole halfedge count", face.id());
        }

        if (n == 2)
        {
            std::cout << "Warn: hole.halfedge_count() == 2 :" << '\t' << face.id() << '\t' << n << eol;
            state.set_warn("degenerate hole, halfedge_count == 2", face.id());

            // Degenerate but legitimate, so what makes it well formed is checked rather than assumed:
            // the pair is its own partner chain, each halfedge being the other's partner and next
            id_type halfedge_id = hole.halfedge_id();
            const auto& halfedge = mesh.halfedge(halfedge_id);
            if (halfedge.partner_id() != halfedge.next_id() || halfedge.next().next_id() != halfedge_id)
            {
                std::cout << "Bad: two halfedge hole " << halfedge_id << " is not its own partner pair, face " << face.id() << eol;
                state.set_bad("two halfedge hole not self paired", face.id());
            }
        }
        else if (n < 2)
        {
            std::cout << "Bad: hole.halfedge_count() < 2 :" << '\t' << face.id() << '\t' << n << eol;
            state.set_bad("degenerate hole, halfedge_count < 2", face.id());
        }
    }

    if (face.halfedge_count() + hole_halfedge_counts.size() != face.halfedge_ext_count())
    {
        std::cout << "Bad: face " << face.id() << " halfedge_ext_count " << face.halfedge_ext_count() << " != perimeter " << face.halfedge_count() << " + holes " << hole_halfedge_counts.size() << eol;
        state.set_bad("halfedge_ext_count mismatch", face.id());
    }

    if (face.surface_id() == nullid)
    {
        std::cout << "Warn: face " << face.id() << " surface null" << eol;
        state.set_warn("face surface null", face.id());
    }
    else if (face.surface_id() >= mesh.surface_store_count())
    {
        std::cout << "Bad: face " << face.id() << " surface out of range:  " << face.surface_id() << '/' << mesh.surface_store_count() << eol;
        state.set_bad();
    }
    else if (!face.surface().contains_face(face.id()))
    {
        std::cout << "Bad: face " << face.id() << " not included in surface " << face.surface_id() << ' ' << face.surface().name()<< eol;
        state.set_bad();
    }
    else if (face.submesh_id() != face.surface().submesh_id())
    {
        std::cout << "Bad: face " << face.id() << " submesh " << face.submesh_id() << " does not match face surface " << face.surface_id() << " submesh " << face.surface().submesh_id()<< eol;
        state.set_bad();
    }

    if (face.submesh_id() == nullid)
    {
        std::cout << "Warn: face " << face.id() << " submesh null" << eol;
        state.set_warn("face submesh null", face.id());
    }
    else if (face.submesh_id() >= mesh.submesh_store_count())
    {
        std::cout << "Bad: face " << face.id() << " submesh out of range:  " << face.submesh_id() << '/' << mesh.submesh_store_count() << eol;
        state.set_bad();
    }
    else if (!face.submesh().contains_face(face.id()))
    {
        std::cout << "Bad: face " << face.id() << " not included in submesh " << face.submesh_id() << ' ' << face.submesh().name() << eol;
        state.set_bad();
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_faces(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking faces: " << mesh.face_count() << '/' << mesh.face_store_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_face(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_border_face(const M& mesh, id_type idHalfedge)
{
    ValidationState state;

    id_type idPrev = prev_border_halfedge_id(mesh, idHalfedge);
    auto positionPrev = mesh.halfedge(idPrev).vertex_attributes().position();

    id_type id = idHalfedge;
    do
    {
        const auto& halfedge = mesh.halfedge(id);
        auto position = halfedge.vertex_attributes().position();
        if (float_eq(position, positionPrev))
        {
            std::cout << "Bad: duplicated vertex positions: " << idPrev << " [" << positionPrev << "] " << halfedge.id() << " [" << position << ']' << eol;
            state.set_bad();
        }

        idPrev = id;
        positionPrev = position;

        id = next_border_halfedge_id(mesh, id);
    } while (id != idHalfedge);

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_surfaces(const M& mesh)
{
    ValidationState state;

    if (mesh.surface_count() != mesh.surface_index_count())
    {
        std::cout << "Bad: mesh.surface_count() != mesh.surface_index_count() " << mesh.surface_count() << ' ' << mesh.surface_index_count() << eol;
        state.set_bad();
    }

    if (mesh.surface_count() == 0)
    {
        return state;
    }

    std::map<id_type, size_t> face_counts;

    std::cout << "Checking surfaces: " << mesh.surface_count() << '/' << mesh.surface_store_count() << " surfaces" << eol;
    for (const auto& surface : mesh.surfaces())
    {
        if (surface.empty())
        {
            std::cout << "Bad: surface.empty() " << surface.id() << eol;
            state.set_bad();
        }

        if (surface.name().empty())
        {
            std::cout << "Warn: surface.name().empty() " << surface.id() << eol;
            state.set_warn("surface name empty", surface.id());
        }

        if (surface.submesh_id() == nullid)
        {
            std::cout << "Warn: surface " << surface.id() << " submesh null" << eol;
            state.set_warn("surface submesh null", surface.id());
        }
        else if (surface.submesh_id() > mesh.submesh_store_count())
        {
            std::cout << "Bad: surface " << surface.id() << " submesh out of range:  " << surface.submesh_id() << '/' << mesh.submesh_store_count() << eol;
            state.set_bad();
        }
        else if (!surface.submesh().contains_surface(surface.id()))
        {
            std::cout << "Bad: surface " << surface.id() << " not included in submesh " << surface.submesh_id() << ' ' << surface.submesh().name() << eol;
            state.set_bad();
        }

        for (id_type idFace : surface.face_ids())
        {
            ++face_counts[idFace];

            if (mesh.face_store()[idFace].deleted())
            {
                std::cout << "Bad: mesh.face(idFace).deleted(): face: " << idFace << " in surface " << surface.id() << eol;
                state.set_bad();
            }

            if (mesh.face(idFace).surface_id() != surface.id())
            {
                std::cout << "Bad: mesh.face(idFace).surface_id() != surface.id(): face: " << idFace << ' ' << mesh.face(idFace).surface_id() << ' ' << surface.id() << eol;
                state.set_bad();
            }
        }
    }

    for (const auto& face_count : face_counts)
    {
        if (face_count.second > 1)
        {
            std::cout << "Bad: face " << face_count.first << " included in " << face_count.second << " surfaces" << eol;
            state.set_bad();
        }
    }

    std::map<id_type, size_t> surface_face_counts;

    for (const auto& face : mesh.faces())
    {
        if (face.surface_id() == nullid)
        {
            size_t n = 0;
            for (const auto& surface : mesh.surfaces())
            {
                for (id_type idFace : surface.face_ids())
                {
                    if (idFace == face.id())
                    {
                        ++n;
                    }
                }
            }

            if (n > 0)
            {
                std::cout << "Bad: face with null surface occurs " << n << " times in surfaces" << eol;
                state.set_bad();
            }

            continue;
        }

        ++surface_face_counts[face.surface_id()];

        size_t n = 0;
        for (id_type idFace : face.surface().face_ids())
        {
            if (idFace == face.id())
            {
                ++n;
            }
        }

        if (n != 1)
        {
            std::cout << "Bad: face " << face.id() << " occurs " << n << "/1 times in surface " << face.surface_id() << ' ' << face.surface().name() << eol;
            state.set_bad();
        }
    }

    if (surface_face_counts.size() != mesh.surface_count())
    {
        std::cout << "Bad: surface_face_counts.size() != mesh.surface_count() " << surface_face_counts.size() << ' ' << mesh.surface_count() << eol;
        state.set_bad();
    }

    for (const auto& surface_face_count : surface_face_counts)
    {
        if (mesh.surface(surface_face_count.first).face_count() != surface_face_count.second)
        {
            std::cout << "Bad: mesh.surface(surface_face_count.first).face_count() != surface_face_count.second for surface " << surface_face_count.first << ", " << mesh.surface(surface_face_count.first).face_count() << ' ' << surface_face_count.second << eol;
            state.set_bad();
        }
    }

    for (const auto& s : mesh.surfaces())
    {
        if (s.face_count() != surface_face_counts[s.id()])
        {
            std::cout << "Bad: s.face_count() != surface_face_counts[s.id()] for surface " << s.id() << ", " << s.face_count() << ' ' << surface_face_counts[s.id()] << eol;
            state.set_bad();
        }
    }

    if (face_counts.size() != mesh.face_count())
    {
        std::cout << "Bad: surface face counts: Only " << face_counts.size() << " out of " << mesh.face_count() << " faces are associated with surfaces" << eol;
        state.set_bad();
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_submeshes(const M& mesh)
{
    ValidationState state;

    if (mesh.submesh_count() != mesh.submesh_index_count())
    {
        std::cout << "Bad: mesh.submesh_count() != mesh.submesh_index_count() " << mesh.submesh_count() << ' ' << mesh.submesh_index_count() << eol;
        state.set_bad();
    }

    if (mesh.submesh_count() == 0)
    {
        return state;
    }

    std::map<id_type, size_t> face_counts;
    std::map<id_type, size_t> surface_counts;

    std::cout << "Checking submeshes: " << mesh.submesh_count() << '/' << mesh.submesh_store_count() << " submeshes" << eol;
    for (const auto& submesh : mesh.submeshes())
    {
        if (submesh.empty())
        {
            std::cout << "Bad: submesh.empty() " << submesh.id() << eol;
            state.set_bad();
        }

        if (submesh.name().empty())
        {
            std::cout << "Warn: submesh.name().empty() " << submesh.id() << eol;
            state.set_warn("submesh name empty", submesh.id());
        }

        for (id_type idFace : submesh.face_ids())
        {
            ++face_counts[idFace];

            if (mesh.face_store()[idFace].deleted())
            {
                std::cout << "Bad: mesh.face(idFace).deleted(): face: " << idFace << " in submesh " << submesh.id() << eol;
                state.set_bad();
            }

            if (mesh.face(idFace).submesh_id() != submesh.id())
            {
                std::cout << "Bad: mesh.face(idFace).submesh_id() != submesh.id() " << mesh.face(idFace).submesh_id() << ' ' << submesh.id() << eol;
                state.set_bad();
            }
        }

        for (id_type idSurface : submesh.surface_ids())
        {
            ++surface_counts[idSurface];

            if (mesh.surface_store()[idSurface].deleted())
            {
                std::cout << "Bad: mesh.surface(idSurface).deleted(): surface: " << idSurface << " in submesh " << submesh.id() << eol;
                state.set_bad();
            }

            if (mesh.surface(idSurface).submesh_id() != submesh.id())
            {
                std::cout << "Bad: mesh.surface(idSurface).submesh_id() != submesh.id() " << mesh.surface(idSurface).submesh_id() << ' ' << submesh.id() << eol;
                state.set_bad();
            }
        }
    }

    for (const auto& face_count : face_counts)
    {
        if (face_count.second > 1)
        {
            std::cout << "Bad: face " << face_count.first << " occurs " << face_count.second << "/1 times in submeshes" << eol;
            state.set_bad();
        }
    }

    for (const auto& surface_count : surface_counts)
    {
        if (surface_count.second > 1)
        {
            std::cout << "Bad: surface " << surface_count.first << " occurs " << surface_count.second << "/1 times in submeshes" << eol;
            state.set_bad();
        }
    }

    for (const auto& face : mesh.faces())
    {
        if (face.submesh_id() == nullid)
        {
            size_t n = 0;
            for (const auto& submesh : mesh.submeshes())
            {
                for (id_type idFace : submesh.face_ids())
                {
                    if (idFace == face.id())
                    {
                        ++n;
                    }
                }
            }

            if (n > 0)
            {
                std::cout << "Bad: face with null submesh occurs " << n << " times in submeshes" << eol;
                state.set_bad();
            }

            continue;
        }

        size_t n = 0;
        for (id_type idFace : face.submesh().face_ids())
        {
            if (idFace == face.id())
            {
                ++n;
            }
        }

        if (n != 1)
        {
            std::cout << "Bad: face " << face.id() << " occurs " << n << "/1 times in submesh " << face.submesh_id() << ' ' << face.submesh().name() << eol;
            state.set_bad();
        }
    }

    // same for surfaces ...

    if (face_counts.size() != mesh.face_count())
    {
        std::cout << "Bad: submesh face counts: " << face_counts.size() << " out of " << mesh.face_count() << " faces are associated with submeshes" << eol;
        state.set_bad();
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_triangular_face(const M& mesh, const typename M::face_type& face)
{
    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    if (face.halfedge_count() != 3)
    {
        std::cout << "Bad: face.halfedge_count() != 3" << eol;
        state.set_bad();
    }

    if (!convex(face.halfedge()))
    {
        std::cout << "Bad: !convex(face.halfedge())" << eol;
        state.set_bad();
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_triangulation(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking triangulation: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_triangular_face<M>(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_manifold_edges(const M& mesh)
{
    ValidationState state;

    std::map<id_type, size_t> face_counts;
    std::map<id_type, size_t> partner_face_counts;

    std::cout << "Checking manifold edges: " << mesh.halfedge_count() << " halfedges" << eol;
    for (const auto& face : mesh.faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            face_counts[halfedge.id()]++;
            partner_face_counts[halfedge.partner_id()]++;
        }
    }

    for (const auto& i : face_counts)
    {
        if (i.second > 1)
        {
            std::cout << "Bad: halfedge " << i.first << " connected to " << i.second << " faces" << eol;
            state.set_bad();
        }
    }

    for (const auto& i : partner_face_counts)
    {
        if (i.second > 1)
        {
            std::cout << "Bad: halfedge partner " << i.first << " connected to " << i.second << " faces" << eol;
            state.set_bad();
        }
    }

    state += check_adjacent_face_normals(mesh);

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_adjacent_face_normals(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking adjacent face normals: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_adjacent_face_normals(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_adjacent_face_normals(const M& mesh, const typename M::face_type& face)
{
    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    for (const auto& halfedge : face.halfedges())
    {
        const auto& faceA = halfedge.face();
        const auto& faceB = halfedge.partner().face();

        if (dot(faceA.attributes().normal(), faceB.attributes().normal()) < 0)
        {
            std::cout << "Bad?: adjacent face normals " << (faceA.surface_id() != nullid ? face.surface().name() : "-") << ' ' << faceA.id() << '/' << halfedge.id() << " [" << faceA.attributes().normal() << "] and " << (faceB.surface_id() != nullid ? face.surface().name() : "-") << ' ' << faceB.id() << '/' << halfedge.partner_id() << " [" << faceB.attributes().normal() << ']' << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_split_edge(const M& mesh, const typename M::halfedge_type& halfedgeOrig, const typename M::halfedge_type& partnerOrig, const typename M::halfedge_type& halfedge)
{
    const auto& partner = mesh.halfedge(halfedge.partner_id());
    const auto& next = mesh.halfedge(halfedge.next_id());
    const auto& next_partner = mesh.halfedge(halfedge.next_id()).partner();

    ValidationState state;

    std::cout << "Checking split_edge: " << halfedgeOrig.id() << ' ' << partnerOrig.id() << ' ' << halfedge.id() << eol;

    // Original
    if (halfedgeOrig.partner_id() != partnerOrig.id())
    {
        std::cout << "Bad: halfedgeOrig.partner_id() != partnerOrig.id()" << eol;
        state.set_bad();
    }
    if (partnerOrig.partner_id() != halfedgeOrig.id())
    {
        std::cout << "Bad: partnerOrig.partner_id() != halfedgeOrig.id()" << eol;
        state.set_bad();
    }

    // Halfedge
    if (halfedge.id() != halfedgeOrig.id())
    {
        std::cout << "Bad: halfedge.id() != halfedgeOrig.id()" << eol;
        state.set_bad();
    }
    if (halfedge.partner_id() != partner.id())
    {
        std::cout << "Bad: halfedge.partner_id() != partner.id()" << eol;
        state.set_bad();
    }
    if (halfedge.next_id() != next_partner.partner_id())
    {
        std::cout << "Bad: halfedge.next_id() != next_partner.partner_id()" << eol;
        state.set_bad();
    }
    if (halfedge.prev_id() != halfedgeOrig.prev_id())
    {
        std::cout << "Bad: halfedge.prev_id() != halfedgeOrig.prev_id()" << eol;
        state.set_bad();
    }
    if (halfedge.vertex_id() != halfedgeOrig.vertex_id())
    {
        std::cout << "Bad: halfedge.vertex_id() != halfedgeOrig.vertex_id()" << eol;
        state.set_bad();
    }
    if (halfedge.face_id() != halfedgeOrig.face_id())
    {
        std::cout << "Bad: halfedge.face_id() != halfedgeOrig.face_id()" << eol;
        state.set_bad();
    }

    // Partner
    if (partner.partner_id() != halfedgeOrig.id())
    {
        std::cout << "Bad: partner.partner_id() != halfedgeOrig.id()" << eol;
        state.set_bad();
    }
    if (partner.next_id() != partnerOrig.next_id())
    {
        std::cout << "Bad: partner.next_id() != partnerOrig.next_id()" << eol;
        state.set_bad();
    }
    if (partner.prev_id() != partnerOrig.id())
    {
        std::cout << "Bad: partner.prev_id() != partnerOrig.id()" << eol;
        state.set_bad();
    }
    if (partner.face_id() != partnerOrig.face_id())
    {
        std::cout << "Bad: partner.face_id() != partnerOrig.face_id()" << eol;
        state.set_bad();
    }

    // Next
    if (next.partner_id() != partnerOrig.id())
    {
        std::cout << "Bad: next.partner_id() != partnerOrig.id()" << eol;
        state.set_bad();
    }
    if (next.next_id() != halfedgeOrig.next_id())
    {
        std::cout << "Bad: next.next_id() != halfedgeOrig.next_id()" << eol;
        state.set_bad();
    }
    if (next.prev_id() != halfedgeOrig.id())
    {
        std::cout << "Bad: next.prev_id() != halfedgeOrig.id()" << eol;
        state.set_bad();
    }
    if (next.face_id() != halfedgeOrig.face_id())
    {
        std::cout << "Bad: next.face_id() != halfedgeOrig.face_id()" << eol;
        state.set_bad();
    }

    // Partner, Next
    if (!float_eq(next.vertex_attributes().position(), partner.vertex_attributes().position(), v_ulp))
    {
        std::cout << "Bad: !float_eq(next.vertex_attributes().position(), partner.vertex_attributes().position()" << eol;
        state.set_bad();
    }

    // Next Partner
    if (next_partner.id() != partnerOrig.id())
    {
        std::cout << "Bad: next_partner.id() != partnerOrig.id()" << eol;
        state.set_bad();
    }
    if (next_partner.partner_id() != next.id())
    {
        std::cout << "Bad: next_partner.partner_id() != next.id()" << eol;
        state.set_bad();
    }
    if (next_partner.next_id() != partner.id())
    {
        std::cout << "Bad: next_partner.next_id() != partner.id()" << eol;
        state.set_bad();
    }
    if (next_partner.prev_id() != partnerOrig.prev_id())
    {
        std::cout << "Bad: next_partner.prev_id() != partnerOrig.prev_id()" << eol;
        state.set_bad();
    }
    if (next_partner.vertex_id() != partnerOrig.vertex_id())
    {
        std::cout << "Bad: next_partner.vertex_id() != partnerOrig.vertex_id()" << eol;
        state.set_bad();
    }
    if (next_partner.face_id() != partnerOrig.face_id())
    {
        std::cout << "Bad: next_partner.face_id() != partnerOrig.face_id()" << eol;
        state.set_bad();
    }

    // External
    if (halfedge.prev().next_id() != halfedge.id())
    {
        std::cout << "Bad: halfedge.prev().next_id() != halfedge.id()" << eol;
        state.set_bad();
    }
    if (partner.next().prev_id() != partner.id())
    {
        std::cout << "Bad: partner.next().prev_id() != partner.id()" << eol;
        state.set_bad();
    }
    if (next.next().prev_id() != next.id())
    {
        std::cout << "Bad: next.next().prev_id() != next.id()" << eol;
        state.set_bad();
    }
    if (next_partner.prev().next_id() != next_partner.id())
    {
        std::cout << "Bad: next_partner.prev().next_id() != next_partner.id()" << eol;
        state.set_bad();
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }
    else
    {
        write_halfedge<M>(halfedgeOrig, "halfedgeOrig");
        write_halfedge<M>(partnerOrig, "partnerOrig");
        write_halfedge<M>(halfedge, "halfedge");
        write_halfedge<M>(partner, "partner");
        write_halfedge<M>(next, "next");
        write_halfedge<M>(next_partner, "next_partner");
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_split_face(const M& mesh, id_type idHalfedgeA, id_type idHalfedgeB, size_t nEdgesOrig)
{
    const auto& faceA = mesh.halfedge(idHalfedgeA).face();
    const auto& faceB = mesh.halfedge(idHalfedgeB).face();
    const auto& halfedgeA = mesh.halfedge(idHalfedgeA).prev();
    const auto& halfedgeB = mesh.halfedge(idHalfedgeB).prev();

    ValidationState state;

    std::cout << "Checking split_face: " << idHalfedgeA << ' ' << idHalfedgeB << ' ' << nEdgesOrig << eol;

    if (faceA.halfedge_count() + faceB.halfedge_count() != nEdgesOrig + 2)
    {
        std::cout << "Bad: faceA.halfedge_count() + faceB.halfedge_count() != nEdgesOrig + 2";
        std::cout << ": " << faceA.halfedge_count() << " + " << faceB.halfedge_count() << " != " << nEdgesOrig << " + 2" << eol;
        state.set_bad();
    }

    if (faceA.id() == faceB.id())
    {
        std::cout << "Bad: faceA.id() == faceB.id()" << eol;
        state.set_bad();
    }

    if (faceA.halfedge().face_id() != faceA.id())
    {
        std::cout << "Bad: faceA.halfedge().face_id != faceA.id()" << eol;
        state.set_bad();
    }

    if (faceB.halfedge().face_id() != faceB.id())
    {
        std::cout << "Bad: faceB.halfedge().face_id != faceB.id()" << eol;
        state.set_bad();
    }

    if (halfedgeA.next_id() != idHalfedgeA)
    {
        std::cout << "Bad: halfedgeA.next_id != idHalfedgeA" << eol;
        state.set_bad();
    }

    if (halfedgeB.next_id() != idHalfedgeB)
    {
        std::cout << "Bad: halfedgeB.next_id != idHalfedgeB" << eol;
        state.set_bad();
    }

    if (halfedgeA.partner_id() != halfedgeB.id())
    {
        std::cout << "Bad: halfedgeA.partner_id() != halfedgeB.id()" << eol;
        state.set_bad();
    }

    if (halfedgeB.partner_id() != halfedgeA.id())
    {
        std::cout << "Bad: halfedgeB.partner_id() != halfedgeA.id()" << eol;
        state.set_bad();
    }

    if (halfedgeA.vertex_attributes().position() != halfedgeB.next().vertex_attributes().position())
    {
        std::cout << "Bad: halfedgeA.vertex_attributes().position != halfedgeB.next().vertex_attributes().position()" << eol;
        state.set_bad();
    }

    if (halfedgeB.vertex_attributes().position() != halfedgeA.next().vertex_attributes().position())
    {
        std::cout << "Bad: halfedgeB.vertex_attributes().position != halfedgeA.next().vertex_attributes().position()" << eol;
        state.set_bad();
    }

    for (const auto& halfedge : faceA.halfedges())
    {
        if (halfedge.face_id() != faceA.id())
        {
            std::cout << "Bad: halfedge.face_id() != faceA.id()" << eol;
            state.set_bad();
        }
    }

    for (const auto& halfedge : faceB.halfedges())
    {
        if (halfedge.face_id() != faceB.id())
        {
            std::cout << "Bad: halfedge.face_id() != faceB.id()" << eol;
            state.set_bad();
        }
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }
    else
    {
        write_face<M>(faceA, "faceA halfedges");
        for (const auto& halfedge : faceA.halfedges())
        {
            write_halfedge<M>(halfedge, "    ");
        }
        write_face<M>(faceB, "faceB halfedges");
        for (const auto& halfedge : faceB.halfedges())
        {
            write_halfedge<M>(halfedge, "    ");
        }
        write_halfedge<M>(halfedgeA, "halfedgeA");
        write_halfedge<M>(halfedgeB, "halfedgeB");
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_attributes(const M& mesh)
{
    std::string error;
    ValidationState state;

    for (const auto& vertex : mesh.vertices())
    {
        if (!vertex.attributes().validate(error))
        {
            // restore attributes texcoord validation to 0..1 and filter out texcoord range errors here ...
            std::cout << "Bad: vertex.attributes().validate() : " << vertex.id() << ' ' << error << eol;
            state.set_bad();
        }
    }

    for (const auto& face : mesh.faces())
    {
        if (!face.attributes().validate(error))
        {
            std::cout << "Bad: face.attributes().validate() : " << face.id() << ' ' << error << eol;
            state.set_bad();
        }
    }

    return state;
}

//--------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::halfedge_string(const T& halfedge)
{
    std::ostringstream oss;
    oss << static_cast<const Flags&>(halfedge);
    oss << ' ' << std::right << std::setw(5) << halfedge.id();
    if (!halfedge.deleted())
    {
        oss << ' ' << std::setw(5) << partner_id_string(halfedge.partner_id());
        oss << ' ' << std::setw(5) << id_string(halfedge.next_id());
        oss << ' ' << std::setw(5) << id_string(halfedge.prev_id());
        oss << ' ' << std::setw(5) << id_string(halfedge.vertex_id());
        oss << ' ' << std::setw(5) << id_string(halfedge.face_id());
//        oss << " (" << halfedge.vertex_attributes().position() << ')' << " (" << math::cartesian_to_spherical(halfedge.vertex_attributes().position()) << ')';
        oss << " (" << halfedge.vertex_attributes().position() << ')';
    }
    return oss.str();
}

//--------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::vertex_string(const T& vertex)
{
    std::ostringstream oss;
    oss << static_cast<const Flags&>(vertex);
    oss << std::right << std::setw(5) << vertex.id();
    if (!vertex.deleted())
    {
        oss << ' ' << std::setw(5) << vertex.halfedge_id();
        if (vertex.halfedge().border())
        {
            oss << " border halfedge";
        }
        else
        {
//            oss << ' ' << std::setw(3) << vertex.halfedge_count() << " halfedges";
        }
        oss << ' ' << vertex.attributes();
    }
    return oss.str();
}

//--------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::face_string(const T& face)
{
    std::ostringstream oss;
    oss << static_cast<const Flags&>(face);
    oss << ' ' << std::right << std::setw(5) << face.id();
    if (!face.deleted())
    {
        oss << ' ' << std::setw(5) << face.halfedge_id();
        oss << ' ' << std::setw(3) << face.halfedge_count() << " edges";
        oss << ' ' << std::setw(3) << face.hole_count() << " holes";

        oss << ", s " << id_string(face.surface_id());
        if (face.surface_id() != nullid)
        {
            oss << ' ' << face.surface().name();
        }

        oss << ", o " << id_string(face.submesh_id());
        if (face.submesh_id() != nullid)
        {
            oss << ' ' << face.submesh().name();
        }
        oss << ' ' << face.attributes();
    }
    return oss.str();
}

//--------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::surface_string(const T& surface)
{
    std::ostringstream oss;
    oss << static_cast<const Flags&>(surface);
    oss << ' ' << std::right << std::setw(5) << surface.id();
    if (!surface.deleted())
    {
        oss << ' ' << surface.name();

        oss << ", o " << id_string(surface.submesh_id());
        if (surface.submesh_id() != nullid)
        {
            oss << ' ' << surface.submesh().name();
        }

        oss << ", " << surface.face_count() << " faces";
        for (id_type idFace : surface.face_ids())
        {
            oss << ' ' << idFace;
        }
    }
    return oss.str();
}

//--------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::submesh_string(const T& submesh)
{
    std::ostringstream oss;
    oss << static_cast<const Flags&>(submesh);
    oss << ' ' << std::right << std::setw(5) << submesh.id();
    if (!submesh.deleted())
    {
        oss << ' ' << submesh.name();
        oss << "\n\t" << submesh.face_count() << " faces";
        for (id_type idFace : submesh.face_ids())
        {
            oss << ' ' << idFace;
        }
        oss << "\n\t" << submesh.surface_count() << " surfaces";
        for (id_type idSurface : submesh.surface_ids())
        {
            oss << ' ' << idSurface;
        }
    }
    return oss.str();
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_halfedge(const typename M::halfedge_type& halfedge, const std::string& label)
{
    std::cout << std::setw(16) << std::left << label << ' ';
    std::cout << halfedge_string(halfedge);
    std::cout << eol;
    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_vertex(const typename M::vertex_type& vertex, const std::string& label)
{
    std::cout << std::setw(16) << std::left << label << ' ';
    std::cout << vertex_string(vertex);
    std::cout << eol;
    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_face(const typename M::face_type& face, const std::string& label)
{
    std::cout << std::setw(16) << std::left << label << ' ';
    std::cout << face_string(face);
    std::cout << eol;
    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_face_vertices(const M& mesh, id_type idHalfedge)
{
    const auto& face = mesh.halfedge(idHalfedge).face();
    check_face(mesh, face);

    const auto normal = face.attributes().normal();

    std::cout << "Face " << '\t' << face.id() << " of " << mesh.face_store_count() << '\t' << face.halfedge_count() << " edges, surface " << id_string(face.surface_id());
    if (face.surface_id() != nullid)
    {
        std::cout << ' ' << face.surface().name();
    }
    std::cout << " [" << normal << ']' << " (" << geometry::cartesian_to_spherical(normal) << ')';
    std::cout << '\t' << face.halfedge_count() << " halfedges";
    std::cout << eol;

    size_t n = 0;
    for (const auto& halfedge : face.halfedges())
    {
        check_halfedge(mesh, halfedge);
        id_type id = halfedge.id();
        const auto& position = halfedge.vertex_attributes().position();
        const auto& positionNext = halfedge.next().vertex_attributes().position();
        const auto& positionPrev = halfedge.prev().vertex_attributes().position();
        typename M::value_type angle = angle_unit(positionNext - position, positionPrev - position, normal);
        bool bConvex = convex_triad(position, positionNext, positionPrev, normal);
        bool bColinear = colinear(position, positionNext, positionPrev);
        bool bReflex = reflex_triad(position, positionNext, positionPrev, normal);
        bool bEarVertex = ear(halfedge);
        std::cout << '\t' << n++ << '\t' << id << '\t' << halfedge.partner_id() << '\t' << halfedge.next_id() << '\t' << halfedge.prev_id() << '\t' << halfedge.face_id() << '\t' << angle;
        std::cout << '\t' << vertex_orientation(mesh, id);
        std::cout << '\t' << (bConvex ? "convex" : "      ");
        std::cout << "  " << (bColinear ? "colinear" : "        ");
        std::cout << "  " << (bReflex ? "reflex" : "      ");
        std::cout << '\t' << (bEarVertex ? "ear" : " - ");
        std::cout << eol;
    }

    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_hole_vertices(const M& mesh, id_type idHalfedge)
{
    const auto& face = mesh.halfedge(idHalfedge).face();

    const auto normal = face.attributes().normal();

    std::cout << "    hole " << idHalfedge << ", face " << face.id();
    std::cout << eol;

// hole halfedge iterator? ...

    size_t n = 0;
    id_type id = idHalfedge;
    do
    {
        id_type idHalfedgeNext = mesh.halfedge(id).next_id();
        
        const auto& halfedge = mesh.halfedge(id);
        check_halfedge(mesh, halfedge);
//        typename M::value_type angle_ = angle(halfedge); // wrong ...
        bool bConvex = convex(halfedge);
        bool bColinear = colinear(halfedge);
        bool bReflex = reflex(halfedge);
        bool bEar = ear(halfedge);
//        std::cout << '\t' << n++ << '\t' << id << '\t' << halfedge.partner_id() << '\t' << halfedge.next_id() << '\t' << halfedge.prev_id() << '\t' << halfedge.face_id() << '\t' << angle_;
        std::cout << '\t' << n++ << '\t' << halfedge_string(halfedge);
//        std::cout << '\t' << orientation(halfedge);
        std::cout << '\t' << (bConvex ? "convex" : "      ");
        std::cout << "  " << (bColinear ? "colinear" : "        ");
        std::cout << "  " << (bReflex ? "reflex" : "      ");
        std::cout << '\t' << (bEar ? "ear" : " - ");
        std::cout << eol;

        id = idHalfedgeNext;
    } while (id != idHalfedge);

    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_surface(const typename M::surface_type& surface, const std::string& label)
{
    std::cout << std::setw(16) << std::left << label << ' ';
    std::cout << surface_string(surface);
    std::cout << eol;
    return;
}

//--------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_submesh(const typename M::submesh_type& submesh, const std::string& label)
{
    std::cout << std::setw(16) << std::left << label << ' ';
    std::cout << submesh_string(submesh);
    std::cout << eol;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::write_summary(const M& mesh)
{
    std::cout << "h " << mesh.halfedge_count() << '/' << mesh.halfedge_store_count() << "; ";
    std::cout << "v " << mesh.vertex_count() << '/' << mesh.vertex_store_count() << "; ";
    std::cout << "f " << mesh.face_count() << '/' << mesh.face_store_count() << "; ";
    std::cout << "s " << mesh.surface_count() << '/' << mesh.surface_store_count() << "; ";
    std::cout << "o " << mesh.submesh_count() << '/' << mesh.submesh_store_count();
    std::cout << eol;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump(const M& mesh)
{
    size_t n = 0;
    for (const auto& halfedge : mesh.halfedge_store())
    {
        write_halfedge<M>(halfedge, std::format("Halfedge[{}]", n++));
        if (halfedge.deleted())
        {
            continue;
        }

        check_halfedge(mesh, halfedge);
    }

    n = 0;
    for (const auto& vertex : mesh.vertex_store())
    {
        write_vertex<M>(vertex, std::format("Vertex[{}]", n++));
        if (vertex.deleted())
        {
            continue;
        }

        const auto& position = vertex.attributes().position();

        assert(vertex.halfedges().size() == vertex.halfedge_count());
        for (const auto& halfedge : vertex.halfedges())
        {
            std::cout << "    halfedge " << std::right << std::setw(5) << halfedge.id();
            std::cout << ", v " << std::setw(5) << halfedge.vertex_id();
            std::cout << ", vh " << std::setw(5) << halfedge.vertex().halfedge_id();
            std::cout << " (" << halfedge.vertex_attributes().position() << ") (" << halfedge.vertex_attributes().normal() << ')';
            std::cout << ' ' << (float_eq(halfedge.vertex_attributes().position(), position, v_ulp) ? "OK" : "Bad");
            std::cout << eol;

    		if (halfedge.border())
    		{
    			break;
    		}
        }

        check_vertex(mesh, vertex);
    }

    n = 0;
    for (const auto& face : mesh.face_store())
    {
        write_face<M>(face, std::format("Face[{}]", face.id()));
        if (face.deleted())
        {
            continue;
        }

// need this check in check_face, but n is not always available to pass in ...
// not true, split_face can change face ids ...
// if that's true, then split_face should be fixed ...
/*
        if (face.id() != n)
        {
            std::cout << "Bad: face.id() != n:" << "\tface " << face.id() << " found at position " << n << eol;
            state.set_bad();
        }
*/
        for (const auto& halfedge : face.halfedges())
        {
//            write_halfedge<M>(halfedge, "    halfedge"); // ...
            std::cout << "    halfedge " << std::right << std::setw(5) << halfedge.id();
            std::cout << ", v " << std::setw(5) << halfedge.vertex_id();
            std::cout << ", f " << std::setw(5) << halfedge.face_id();
            std::cout << ' ' << halfedge.vertex_attributes();
            std::cout << eol;
        }

        //size_t i = 0;
        for (const auto& hole : face.holes())
        {
            write_hole_vertices<M>(mesh, hole.halfedge_id());
            /*
            std::cout << "    hole " << i++ << ' ' << std::setw(3) << hole.halfedge_count() << " halfedges" << eol;
            for (const auto& halfedge : hole.halfedges())
            {
                write_halfedge<M>(halfedge, "    halfedge");
            }
            */
        }

        check_face(mesh, face);
        ++n;
    }

    n = 0;
    for (const auto& surface : mesh.surface_store())
    {
        write_surface<M>(surface, std::format("Surface[{}]", n++));
        if (surface.deleted())
        {
            continue;
        }
    }

    std::cout << "Surface index" << eol;
    for (const auto& i : mesh.surface_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    check_surfaces(mesh);

    n = 0;
    for (const auto& submesh : mesh.submesh_store())
    {
        write_submesh<M>(submesh, std::format("Submesh[{}]", n++));
        if (submesh.deleted())
        {
            continue;
        }
    }

    std::cout << "Submesh index" << eol;
    for (const auto& i : mesh.submesh_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    check_submeshes(mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_compact(const M& mesh)
{
    size_t n = 0;
    for (const auto& halfedge : mesh.halfedge_store())
    {
        write_halfedge<M>(halfedge, std::format("Halfedge[{}]", n++));
    }

    n = 0;
    for (const auto& vertex : mesh.vertex_store())
    {
        write_vertex<M>(vertex, std::format("Vertex[{}]", n++));
    }

    n = 0;
    for (const auto& face : mesh.face_store())
    {
        write_face<M>(face, std::format("Face[{}]", n++));
        if (face.deleted())
        {
            continue;
        }

        for (const auto& halfedge : face.halfedges())
        {
            std::cout << std::format("    halfedge {:>5}, v {:5}, f {:5}, {}", halfedge.id(), halfedge.vertex_id(), halfedge.face_id(), halfedge.vertex_attributes()) << eol;
        }
    }

    n = 0;
    for (const auto& surface : mesh.surface_store())
    {
        write_surface<M>(surface, std::format("Surface[{}]", n++));
    }

    std::cout << "Surface index" << eol;
    for (auto i : mesh.surface_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    n = 0;
    for (const auto& submesh : mesh.submesh_store())
    {
        write_submesh<M>(submesh, std::format("Submesh[{}]", n++));
    }

    std::cout << "Submesh index" << eol;
    for (auto i : mesh.submesh_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_structure(const M& mesh)
{
    size_t n = 0;
    for (const auto& surface : mesh.surface_store())
    {
        write_surface<M>(surface, std::format("Surface[{}]", n++));
        if (surface.deleted())
        {
            continue;
        }
    }

    std::cout << "Surface index" << eol;
    for (auto i : mesh.surface_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    check_surfaces(mesh);

    n = 0;
    for (const auto& submesh : mesh.submesh_store())
    {
        write_submesh<M>(submesh, std::format("Submesh[{}]", n++));
        if (submesh.deleted())
        {
            continue;
        }
    }

    std::cout << "Submesh index" << eol;
    for (auto i : mesh.submesh_index())
    {
        std::cout << '\t' << i.second << '\t' << i.first << eol;
    }

    check_submeshes(mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_submesh(const M& mesh, id_type id)
{
    const auto& submesh = mesh.submesh_store()[id];
    write_submesh<M>(submesh, std::format("Submesh[{}]", id));
    if (submesh.deleted())
    {
        return;
    }

/*
    for (const auto& face : submesh.faces())
    {
        write_face<M>(face, std::format("Face[{}]", face.id()));

        for (const auto& halfedge : face.halfedges())
        {
//                std::cout << std::format("    halfedge {:>5}, v {:5}, f {:5}, {}", halfedge.id(), halfedge.vertex_id(), halfedge.face_id(), halfedge.vertex_attributes()) << eol;
// attributes need format support ...
            std::cout << std::format("    halfedge{:>5}, v {:5}, f {:5}, ", halfedge.id(), halfedge.vertex_id(), halfedge.face_id()) << halfedge.vertex_attributes() << eol;
        }
    }

    for (const auto& surface : submesh.surfaces())
    {
        write_surface<M>(surface, std::format("Surface[{}]", surface.id()));
    }
*/    

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_submeshes(const M& mesh)
{
    for (const auto& submesh : mesh.submesh_store())
    {
        dump_submesh(mesh, submesh.id());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_face(const M& mesh, id_type id)
{
    const auto& face = mesh.face_store()[id];
    write_face<M>(face, std::format("Face[{}]", id));
    if (face.deleted())
    {
        return;
    }

    for (const auto& halfedge : face.halfedges())
    {
        write_halfedge<M>(halfedge, "    halfedge");
//        std::cout << "        " << halfedge.vertex_attributes();
    }

//    size_t i = 0;
    for (const auto& hole : face.holes())
    {
        /*
        std::cout << "    hole " << i++ << ' ' << std::setw(3) << hole.halfedge_count() << " halfedges" << eol;
        for (const auto& halfedge : hole.halfedges())
        {
            write_halfedge<M>(halfedge, "    halfedge");
        }
        */
        write_hole_vertices<M>(mesh, hole.halfedge_id());
    }

    check_face(mesh, face);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_faces(const M& mesh)
{
    for (const auto& face : mesh.face_store())
    {
        dump_face(mesh, face.id());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::mesh_info(const M& mesh)
{
    std::cout << "halfedges   " << mesh.halfedge_count() << '/' << mesh.halfedge_store_count() << eol;
    std::cout << "vertices    " << mesh.vertex_count() << '/' << mesh.vertex_store_count() << eol;
    std::cout << "faces       " << mesh.face_count() << '/' << mesh.face_store_count() << eol;
    std::cout << "surfaces    " << mesh.surface_count() << '/' << mesh.surface_store_count() << ", " << mesh.surface_index_count() << " i" << eol;
    std::cout << "submeshes   " << mesh.submesh_count() << '/' << mesh.submesh_store_count() << ", " << mesh.submesh_index_count() << " i" << eol;

    for (const auto& o : mesh.submeshes())
    {
        std::cout << std::setw(4) << std::right << o.id() << ' ' << std::setw(14) << std::left << o.name() << ' ' << o.surface_count() << " s, " << o.surface_index_count() << " i, " << o.face_count() << " f" << eol;
        for (const auto& s : o.surfaces())
        {
            std::cout << std::setw(8) << std::right << s.id() << ' ' << std::setw(14) << std::left << s.name() << ' ' << s.face_count() << " f" << eol;
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face_halfedges_ext(const M& mesh, const typename M::face_type& face)
{
    ValidationState state(true);

    // What the merged iteration should produce: the perimeter loop, then each hole loop in turn
    std::vector<id_type> halfedge_ids_expected;
    for (const auto& halfedge : face.halfedges())
    {
        halfedge_ids_expected.push_back(halfedge.id());
    }

    for (const auto& hole : face.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            halfedge_ids_expected.push_back(halfedge.id());
        }
    }

    std::vector<id_type> halfedge_ids;
    for (const auto& halfedge : face.halfedges_ext())
    {
        halfedge_ids.push_back(halfedge.id());
    }

    if (halfedge_ids != halfedge_ids_expected)
    {
        std::cout << "Face " << face.id() << " halfedges_ext " << halfedge_ids.size() << " halfedges, expected " << halfedge_ids_expected.size() << " (" << face.hole_count() << " holes)" << eol;
        state.set_bad();
    }

    if (face.halfedge_ext_count() != halfedge_ids_expected.size())
    {
        std::cout << "Face " << face.id() << " halfedge_ext_count " << face.halfedge_ext_count() << ", expected " << halfedge_ids_expected.size() << eol;
        state.set_bad();
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_faces_halfedges_ext(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking face halfedges_ext: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_face_halfedges_ext(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face_normals(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking face normals: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_face_normal(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face_normal(const M& mesh, const typename M::face_type& face)
{
    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    auto normal = face.attributes().normal();

    for (const auto& halfedge : face.halfedges())
    {
        if (!float_eq(halfedge.vertex_attributes().normal(), normal, v_ulp))
        {
            std::cout << "Bad: !float_eq(halfedge.vertex_attributes().normal(), normal) : " << face.id() << ' ' << halfedge.id() << "  " << halfedge.vertex_attributes().normal() << "  " << normal << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_normal_consistency(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking normal consistency: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_normal_consistency(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_normal_consistency(const M& mesh, const typename M::face_type& face)
{
    mesh;

    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    const auto& normal = face.attributes().normal();

    for (const auto& halfedge : face.halfedges())
    {
        const auto& attributes = halfedge.vertex_attributes();
        if (dot(attributes.normal(), normal) < 0)
        {
            std::cout << "Bad: dot(attributes.normal(), normal) < 0 : " << face.id() << ' ' << halfedge.id() << " [" << attributes.normal() << "] [" << normal << ']' << eol;
            state.set_bad();
        }

        auto to_point = [](const typename M::vertex_attributes_type::texcoord_type& t) -> typename M::point_type { return {t.x(), t.y(), M::val(0)}; };

        auto t0 = to_point(halfedge.vertex_attributes().texcoord());
        auto t1 = to_point(halfedge.next().vertex_attributes().texcoord());
        auto t2 = to_point(halfedge.prev().vertex_attributes().texcoord());
        auto texcoord_normal = cross(t1 - t0, t2 - t0);
        if (dot(texcoord_normal, normal) > 0)
        {
            std::cout << "Bad: dot(texcoord_normal, normal) > 0 : " << face.id() << ' ' << halfedge.id() << " [" << texcoord_normal << "] [" << normal << ']' << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_surface_normal_consistency(const M& mesh)
{
    ValidationState state;

    mesh.reset_all();

    std::cout << "Checking surface normal consistency: " << mesh.surface_count() << " surfaces" << eol;
    for (const auto& surface : mesh.surfaces())
    {
        state += check_surface_normal_consistency(mesh, surface);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_surface_normal_consistency(const M& mesh, const typename M::surface_type& surface)
{
    ValidationState state;
    if (surface.deleted())
    {
        return state;
    }

    std::cout << surface.name() << eol;
    for (id_type idFace : surface.face_ids())
    {
        const auto& face = mesh.face(idFace);
        for (const auto& halfedge : face.halfedges())
        {
            const auto& vertex = halfedge.vertex();
            state += check_surface_normal_consistency(mesh, surface, vertex);
        }
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_surface_normal_consistency(const M& mesh, const typename M::surface_type& surface, const typename M::vertex_type& vertex)
{
    ValidationState state;
    if (vertex.deleted())
    {
        return state;
    }

mesh; // ...
//    const auto& normal = vertex.halfedge().face_attributes().normal();
// check that surface/face, face/vertex normals agree too ...

    const auto& attributesNominal = vertex.halfedge().vertex_attributes(); // what? ...
    for (const auto& halfedge : vertex.halfedges())
    {
        if (halfedge.face().surface_id() == surface.id() && !halfedge.vertex().marked())
        {
            const auto& attributes = halfedge.vertex_attributes();

            if (!float_eq(attributes.normal(), attributesNominal.normal()))
            {
                std::cout << "Bad: !float_eq(attributes.normal(), attributesNominal.normal()) : " << vertex.id() << " [" << attributes.normal() << "] [" << attributesNominal.normal() << ']' << eol;
                state.set_bad();
            }
            else if (attributes.normal() != attributesNominal.normal())
            {
                std::cout << "Warn: attributes.normal() != attributesNominal.normal() : " << vertex.id() << " [" << attributes.normal() << "] [" << attributesNominal.normal() << ']' << eol;
                state.set_warn("vertex normal differs from nominal", vertex.id());
            }

            const_cast<typename M::halfedge_type&>(halfedge).vertex().set_marked();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_surface_vertex_consistency_exact(const M& mesh)
{
    ValidationState state;

    for (const auto& vertex : mesh.vertices())
    {
        if (vertex.deleted())
        {
            return state;
        }

        auto normal = surface_normal(vertex);

        for (const auto& halfedge : vertex.halfedges())
        {
            if (halfedge.vertex_id() == vertex.id())
            {
                continue;
            }

            if (halfedge.vertex_attributes().position() != vertex.attributes().position())
            {
                std::cout << "  Warn: coincident vertex position: reference vertex " << vertex.id() << " (" << vertex.attributes().position() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().position() << ')' << eol;
                state.set_warn("coincident vertex position", vertex.id());
            }

            if (halfedge.surface_id() != vertex.halfedge().surface_id())
            {
                continue;
            }

            if (!float_eq(halfedge.vertex_attributes().normal(), normal, 10000))
            {
                std::cout << "  Warn: surface vertex normal: vertex " << vertex.id() << " normal (" << vertex.attributes().normal() << ") != (float) normal (" << normal << ')' << eol;
                state.set_warn("surface vertex normal differs from computed", vertex.id());
            }

            if (halfedge.vertex_attributes().normal() != vertex.attributes().normal())
            {
                std::cout << "  Warn: coincident surface vertex normal: reference vertex " << vertex.id() << " (" << vertex.attributes().normal() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().normal() << ')' << eol;
                state.set_warn("coincident surface vertex normal", vertex.id());
            }

            if (halfedge.vertex_attributes().texcoord() != vertex.attributes().texcoord())
            {
                std::cout << "  Warn: coincident surface vertex texcoord: reference vertex " << vertex.id() << " (" << vertex.attributes().texcoord() << ") != vertex " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().texcoord() << ')' << eol;
                state.set_warn("coincident surface vertex texcoord", vertex.id());
            }
        }
    }

    std::cout << "check_surface_vertex_consistency_exact: " << std::boolalpha << state.ok() << eol;

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face_angles(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking face angles: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_face_angles(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_face_angles(const M& mesh, const typename M::face_type& face)
{
    mesh;

    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    using T = M::value_type;

    size_t n = face.halfedge_count();
    T sum = 0;

    for (const auto& halfedge : face.halfedges())
    {
        const auto& av = halfedge.vertex_attributes();

        T a = angle_between(halfedge.next().vertex_attributes().position() - av.position(), halfedge.prev().vertex_attributes().position() - av.position());
        bool bColinear = colinear(halfedge);
        bool bConvex = convex(halfedge);
        bool bReflex = reflex(halfedge);
        if (bReflex)
        {
            a = math::PiTwo<T> - a;
        }

        if (bReflex)
        {
            std::cout << "Warn: bReflex : " << face.id() << ' ' << halfedge.id() << eol;
        }

        if (bColinear && bConvex)
        {
            std::cout << "Bad: bColinear && bConvex : " << face.id() << ' ' << halfedge.id() << eol;
            state.set_bad();
        }

        if (bColinear && bReflex)
        {
            std::cout << "Bad: bColinear && bReflex : " << face.id() << ' ' << halfedge.id() << eol;
            state.set_bad();
        }

        if (bConvex && bReflex)
        {
            std::cout << "Bad: bConvex && bReflex : " << face.id() << ' ' << halfedge.id() << eol;
            state.set_bad();
        }

        if (!bConvex && !bColinear && !bReflex)
        {
            std::cout << "Bad: !bConvex && !bColinear && !bReflex : " << face.id() << ' ' << halfedge.id() << eol;
            state.set_bad();
        }

        sum += a;
    }

    if (math::float_ne(sum, math::Pi<T> * (n - 2)))
    {
        std::cout << "Bad: sum != math::Pi<T> * (n - 2) : " << face.id() << ' ' << sum << ' ' << (math::Pi<T> * (n - 2)) << eol;
        state.set_bad();
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_convex(const M& mesh)
{
    ValidationState state;

    std::cout << "Checking convex: " << mesh.face_count() << " faces" << eol;
    for (const auto& face : mesh.faces())
    {
        state += check_convex(mesh, face);
    }

    if (state.ok())
    {
        std::cout << "OK" << eol;
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_convex(const M& mesh, const typename M::face_type& face)
{
    mesh;

    ValidationState state;
    if (face.deleted())
    {
        return state;
    }

    // convex_face not used so that non-convex vertex can be identified
    for (const auto& halfedge : face.halfedges())
    {
        if (!convex(halfedge))
        {
            std::cout << "Bad: !convex(halfedge) : " << face.id() << ' ' << halfedge.id() << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::ValidationState quetzal::brep::check_outward_normals(const M& mesh)
{
    ValidationState state;

    for (const auto& vertex : mesh.vertices())
    {
        if (dot(vertex.attributes().position(), vertex.attributes().normal()) <= 0)
        {
            std::cout << "Bad: dot(vertex.attributes().position(), vertex.attributes().normal()) <= 0 : " << vertex.id() << "  " << vertex.attributes().position() << "  " << vertex.attributes().normal() << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::ValidationState quetzal::brep::check_spherical(const Mesh<Traits>& mesh, typename Traits::value_type radius)
{
    ValidationState state;
    auto r_squared = radius * radius;

    for (const auto& vertex : mesh.vertices())
    {
        if (math::float_ne(vertex.attributes().position().vector().norm_squared(), r_squared))
        {
            std::cout << "Bad: vertex.attributes().position().vector().norm_squared() != radius * radius : " << vertex.id() << "  " << vertex.attributes().position() << " " << vertex.attributes().position().vector().norm_squared() << "  " << radius << " " << r_squared << eol;
            state.set_bad();
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::ValidationState quetzal::brep::check_spherical(const Submesh<Traits>& submesh, typename Traits::value_type radius)
{
    ValidationState state;
    auto r_squared = radius * radius;

    for (const auto& face : submesh.faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            const auto& vertex = halfedge.vertex();
            if (math::float_ne(vertex.attributes().position().vector().norm_squared(), r_squared))
            {
                std::cout << "Bad: vertex.attributes().position().vector().norm_squared() != radius * radius : " << vertex.id() << "  " << vertex.attributes().position() << " " << vertex.attributes().position().vector().norm_squared() << "  " << radius << " " << r_squared << eol;
                state.set_bad();
            }
        }
    }

    return state;
}

//------------------------------------------------------------------------------
template<typename M>
std::map<typename M::value_type, size_t> quetzal::brep::count_edge_lengths(const M& mesh)
{
    using value_type = M::value_type;
    using point_type = M::point_type;

    std::map<value_type, size_t> lengths;

    for (const auto& face : mesh.faces())
    {
        typename M::halfedge_type& halfedge = face.halfedge();

        const point_type& pos0 = halfedge.vertex_attributes().position();
        const point_type& pos1 = halfedge.next().vertex_attributes().position();
        const point_type& pos2 = halfedge.prev().vertex_attributes().position();

        value_type length = norm(pos0 - pos1);
        lengths[length]++;
//        std::cout << (i + 0) << '\t' << (i + 1) << '\t' << length << eol;

        length = norm(pos1 - pos2);
        lengths[length]++;
//        std::cout << (i + 1) << '\t' << (i + 2) << '\t' << length << eol;

        length = norm(pos2 - pos0);
        lengths[length]++;
//        std::cout << (i + 2) << '\t' << (i + 0) << '\t' << length << eol;
    }

    return lengths;
}

#endif // QUETZAL_BREP_VALIDATION_HPP
