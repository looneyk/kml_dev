#if !defined(QUETZAL_BREP_FACE_HPP)
#define QUETZAL_BREP_FACE_HPP
//------------------------------------------------------------------------------
// brep
// Face.hpp
//------------------------------------------------------------------------------

#include "Hole.hpp"
#include "id.hpp"
#include "quetzal/common/Elements.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/PositionAccessor.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include <exception>
#include <optional>
#include <ostream>
#include <ranges>
#include <tuple>
#include <vector>
#include <cassert>

#pragma warning(disable : 4715)

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Face : public Flags
    {
    public:

        using halfedge_ext_data = std::tuple<std::optional<size_t>, id_type>;

        using traits_type = Traits;
        using mesh_type = M;
        using halfedge_type = mesh_type::halfedge_type;
        using hole_type = mesh_type::hole_type;
        using surface_type = mesh_type::surface_type;
        using submesh_type = mesh_type::submesh_type;
        using halfedges_type = Elements<mesh_type, halfedge_type, std::optional<size_t>>;
        using halfedges_ext_type = Elements<mesh_type, halfedge_type, halfedge_ext_data>;
        using holes_type = Elements<mesh_type, hole_type, std::optional<size_t>>;
        using attributes_type = mesh_type::face_attributes_type;
        using size_type = mesh_type::size_type;

        Face();
        Face(mesh_type& mesh, id_type id, id_type surface_id, id_type submesh_id, id_type halfedge_id, const attributes_type& attributes = {}, const Properties& properties = {});
        Face(const Face&) = default;
        Face(Face&&) noexcept = default;
        ~Face() = default;

        Face& operator=(const Face&) = default;
        Face& operator=(Face&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type id() const;

        id_type partner_id() const;
        void set_partner_id(id_type halfedge_id);

        const Face& partner() const;
        Face& partner();

        // The halfedge exposed here is an arbitrary halfedge in the face and should not be expected to be consistent between calls

        id_type halfedge_id() const;
        void set_halfedge_id(id_type halfedge_id);

        const halfedge_type& halfedge() const;
        halfedge_type& halfedge();

        // Virtual container interface, iterators for halfedges connected to this face, does not include hole halfedges
        size_type halfedge_count() const;
        const halfedges_type& halfedges() const;
        halfedges_type& halfedges();

        std::vector<id_type> halfedge_ids() const;

        // Virtual container interface, iterators for halfedges connected to this face, both perimeter and hole halfedges
        size_type halfedge_ext_count() const;
        const halfedges_ext_type& halfedges_ext() const;
        halfedges_ext_type& halfedges_ext();

        size_type hole_count() const;
        const holes_type& holes() const;
        holes_type& holes();

        const std::vector<id_type>& hole_ids() const;
        std::vector<id_type>& hole_ids();

        id_type surface_id() const;
        void set_surface_id(id_type surface_id);

        const surface_type& surface() const;
        surface_type& surface();

        id_type submesh_id() const;
        void set_submesh_id(id_type submesh_id);

        const submesh_type& submesh() const;
        submesh_type& submesh();

        // These invalidate halfedge_ext and hole iterators
        hole_type& create_hole(id_type hole_halfedge_id);
        void remove_hole(id_type id);
        hole_type& insert_hole(id_type id);

        // rework to take id's ...
        void assign_halfedges(); // Give each halfedge its face id
        void assign_halfedges_ext(); // Give each halfedge its face id and hole halfedge its face id and hole id
        void assign_halfedges(hole_type& hole); // Give each hole halfedge its face id and hole id

        const attributes_type& attributes() const;
        attributes_type& attributes();
        void set_attributes(const attributes_type& attributes);

        const Properties& properties() const;
        Properties& properties();

        const geometry::AxisAlignedBoundingBox<typename M::vector_traits>& bounds() const;
        void invalidate() const; // Reset hidden internal state

        bool border() const;
        void set_border();

        void reset_all() const override; // Flags

        bool check() const;

        // Internal use
        void set_mesh(mesh_type& mesh);
        void set_id(id_type id);
        void check_mesh(const mesh_type* const pmesh) const;

    private:

        mesh_type* m_pmesh;
        id_type m_id;
        id_type m_partner_id;
        id_type m_halfedge_id;
        id_type m_surface_id;
        id_type m_submesh_id;
        std::vector<id_type> m_hole_ids;

        halfedges_type m_halfedges;
        halfedges_ext_type m_halfedges_ext;
        holes_type m_holes;

        attributes_type m_attributes;
        Properties m_properties;

        mutable std::optional<std::vector<id_type>> m_halfedge_ids; // Lazy evaluation
        mutable std::optional<geometry::AxisAlignedBoundingBox<typename M::vector_traits>> m_bounds; // Lazy evaluation

        static halfedges_type::size_function_type m_halfedges_size;
        static halfedges_type::terminal_function_type m_halfedges_first;
        static halfedges_type::terminal_function_type m_halfedges_last;
        static halfedges_type::terminal_function_type m_halfedges_end;
        static halfedges_type::iterate_function_type m_halfedges_forward;
        static halfedges_type::iterate_function_type m_halfedges_reverse;
        static halfedges_type::element_function_type m_halfedges_element;
        static halfedges_type::const_element_function_type m_halfedges_const_element;

        static halfedges_ext_type::size_function_type m_halfedges_ext_size;
        static halfedges_ext_type::terminal_function_type m_halfedges_ext_first;
        static halfedges_ext_type::terminal_function_type m_halfedges_ext_last;
        static halfedges_ext_type::terminal_function_type m_halfedges_ext_end;
        static halfedges_ext_type::iterate_function_type m_halfedges_ext_forward;
        static halfedges_ext_type::iterate_function_type m_halfedges_ext_reverse;
        static halfedges_ext_type::element_function_type m_halfedges_ext_element;
        static halfedges_ext_type::const_element_function_type m_halfedges_ext_const_element;

        static holes_type::size_function_type m_holes_size;
        static holes_type::terminal_function_type m_holes_first;
        static holes_type::terminal_function_type m_holes_last;
        static holes_type::terminal_function_type m_holes_end;
        static holes_type::iterate_function_type m_holes_forward;
        static holes_type::iterate_function_type m_holes_reverse;
        static holes_type::element_function_type m_holes_element;
        static holes_type::const_element_function_type m_holes_const_element;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const Face<Traits, M>& face);

    template<typename F>
    void test_halfedge_face_iteration(const F& face);

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct geometry::PositionAccessor<Face<Traits>>
    {
        using point_type = brep::Face<Traits>::attributes_type::point_type;
        using vector_type = point_type::vector_type;
        using value_type = point_type::value_type;
        static constexpr size_t dimension = point_type::dimension;

        static size_t position_count(const brep::Face<Traits>& t)
        {
            return t.halfedge_count();
        }

        static point_type position(const brep::Face<Traits>& t, size_t n)
        {
            if (n >= t.halfedge_count())
            {
                throw std::out_of_range("PositionAccessor::range_check");
            }

            return t.mesh().halfedge(t.halfedge_ids()[n]).vertex_attributes().position();
        }

        static bool constant_time_acccess()
        {
            return true;
        }
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Face<Traits, M>::Face() :
    Flags(),
    m_pmesh(nullptr),
    m_id(nullid),
    m_partner_id(nullid),
    m_halfedge_id(nullid),
    m_surface_id(nullid),
    m_submesh_id(nullid),
    m_hole_ids(),
    m_halfedges(m_pmesh, nullid, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_halfedges_ext(m_pmesh, nullid, m_halfedges_ext_size, m_halfedges_ext_first, m_halfedges_ext_last, m_halfedges_ext_end, m_halfedges_ext_forward, m_halfedges_ext_reverse, m_halfedges_ext_element, m_halfedges_ext_const_element),
    m_holes(m_pmesh, nullid, m_holes_size, m_holes_first, m_holes_last, m_holes_end, m_holes_forward, m_holes_reverse, m_holes_element, m_holes_const_element),
    m_attributes(),
    m_properties(),
    m_halfedge_ids(),
    m_bounds()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Face<Traits, M>::Face(mesh_type& mesh, id_type id, id_type surface_id, id_type submesh_id, id_type halfedge_id, const attributes_type& attributes, const Properties& properties) :
    Flags(),
    m_pmesh(&mesh),
    m_id(id),
    m_partner_id(nullid),
    m_halfedge_id(halfedge_id),
    m_surface_id(surface_id),
    m_submesh_id(submesh_id),
    m_hole_ids(),
    m_halfedges(m_pmesh, id, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_halfedges_ext(m_pmesh, id, m_halfedges_ext_size, m_halfedges_ext_first, m_halfedges_ext_last, m_halfedges_ext_end, m_halfedges_ext_forward, m_halfedges_ext_reverse, m_halfedges_ext_element, m_halfedges_ext_const_element),
    m_holes(m_pmesh, id, m_holes_size, m_holes_first, m_holes_last, m_holes_end, m_holes_forward, m_holes_reverse, m_holes_element, m_holes_const_element),
    m_attributes(attributes),
    m_properties(properties),
    m_halfedge_ids(),
    m_bounds()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::mesh_type& quetzal::brep::Face<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::mesh_type& quetzal::brep::Face<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::partner_id() const
{
    return m_partner_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_partner_id(id_type partner_id)
{
    m_partner_id = partner_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Face<Traits, M>& quetzal::brep::Face<Traits, M>::partner() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_partner_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>& quetzal::brep::Face<Traits, M>::partner()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_partner_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::halfedge_id() const
{
    return m_halfedge_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_halfedge_id(id_type halfedge_id)
{
    m_halfedge_id = halfedge_id;
    invalidate();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::halfedge_type& quetzal::brep::Face<Traits, M>::halfedge() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedge_type& quetzal::brep::Face<Traits, M>::halfedge()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::size_type quetzal::brep::Face<Traits, M>::halfedge_count() const
{
    return m_halfedges.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::halfedges_type& quetzal::brep::Face<Traits, M>::halfedges() const
{
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type& quetzal::brep::Face<Traits, M>::halfedges()
{
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Face<Traits, M>::halfedge_ids() const
{
    if (!m_halfedge_ids)
    {
//        m_halfedge_ids = std::ranges::transform_view(m_halfedges, [](const auto& halfedge) { return halfedge.id(); }) | std::ranges::to<std::vector>();
        m_halfedge_ids.emplace();
        id_type halfedge_id = m_halfedge_id;
        do
        {
            m_halfedge_ids->push_back(halfedge_id);
            halfedge_id = m_pmesh->halfedge(halfedge_id).next_id();
        } while (halfedge_id != m_halfedge_id);
    }

    return *m_halfedge_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::size_type quetzal::brep::Face<Traits, M>::halfedge_ext_count() const
{
    return m_halfedges_ext.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::halfedges_ext_type& quetzal::brep::Face<Traits, M>::halfedges_ext() const
{
    return m_halfedges_ext;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type& quetzal::brep::Face<Traits, M>::halfedges_ext()
{
    return m_halfedges_ext;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::size_type quetzal::brep::Face<Traits, M>::hole_count() const
{
    return m_hole_ids.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::holes_type& quetzal::brep::Face<Traits, M>::holes() const
{
    return m_holes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type& quetzal::brep::Face<Traits, M>::holes()
{
    return m_holes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const std::vector<quetzal::id_type>& quetzal::brep::Face<Traits, M>::hole_ids() const
{
    return m_hole_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type>& quetzal::brep::Face<Traits, M>::hole_ids()
{
    return m_hole_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::surface_id() const
{
    return m_surface_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_surface_id(id_type surface_id)
{
    m_surface_id = surface_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::surface_type& quetzal::brep::Face<Traits, M>::surface() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_surface_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::surface_type& quetzal::brep::Face<Traits, M>::surface()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_surface_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::submesh_id() const
{
    return m_submesh_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_submesh_id(id_type submesh_id)
{
    m_submesh_id = submesh_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::submesh_type& quetzal::brep::Face<Traits, M>::submesh() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_submesh_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::submesh_type& quetzal::brep::Face<Traits, M>::submesh()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_submesh_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::create_hole(id_type hole_halfedge_id)
{
    assert(m_pmesh != nullptr);
    id_type id = m_pmesh->create_hole(m_id, hole_halfedge_id);
    m_hole_ids.push_back(id);

    auto& hole = m_pmesh->hole(id);
    hole.assign_halfedges();
    return hole;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::remove_hole(id_type id)
{
    assert(m_pmesh != nullptr);

    auto i = std::ranges::find(m_hole_ids, id);
    assert(i != m_hole_ids.end());

    m_pmesh->hole(id).release_halfedges(); // Halfedges are no longer associated with this hole
    m_pmesh->hole(id).set_face_id(nullid);
    m_hole_ids.erase(i);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::insert_hole(id_type id)
{
    assert(m_pmesh != nullptr);
    m_hole_ids.push_back(id);

    auto& hole = m_pmesh->hole(id);
    hole.set_face_id(m_id);
    hole.assign_halfedges();
    return hole;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::assign_halfedges()
{
    assert(m_pmesh != nullptr);

    for (const auto& halfedge : halfedges())
    {
        m_pmesh->halfedge(halfedge.id()).set_face_id(id());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::assign_halfedges_ext()
{
    assign_halfedges();

    for (auto& hole : holes())
    {
        assign_halfedges(hole);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::assign_halfedges(hole_type& hole)
{
    assert(m_pmesh != nullptr);

    for (const auto& halfedge : hole.halfedges())
    {
        m_pmesh->halfedge(halfedge.id()).set_face_id(id());
    }

    hole.assign_halfedges();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::attributes_type& quetzal::brep::Face<Traits, M>::attributes() const
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::attributes_type& quetzal::brep::Face<Traits, M>::attributes()
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_attributes(const attributes_type& attributes)
{
    m_attributes = attributes;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::Properties& quetzal::brep::Face<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Face<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::geometry::AxisAlignedBoundingBox<typename M::vector_traits>& quetzal::brep::Face<Traits, M>::bounds() const
{
    if (!m_bounds)
    {
        m_bounds = geometry::bounds(*this);
    }

    return *m_bounds;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::invalidate() const
{
    m_halfedge_ids.reset();
    m_bounds.reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Face<Traits, M>::border() const
{
    return m_partner_id == nullid;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_border()
{
    m_partner_id = nullid;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::reset_all() const
{
    for (auto& halfedge : m_halfedges)
    {
        halfedge.reset();
        halfedge.vertex().reset();
    }

    Flags::reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Face<Traits, M>::check() const
{
    assert(m_pmesh != nullptr);

    if (deleted())
    {
        return true;
    }

    return m_id < m_pmesh->face_store_count() && m_pmesh->valid_halfedge_id(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_halfedges.set_source(mesh);
    m_halfedges_ext.set_source(mesh);
    m_holes.set_source(mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_id(id_type id)
{
    m_id = id;
    m_halfedges.set_id(id);
    m_halfedges_ext.set_id(id);
    m_holes.set_id(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    assert(m_halfedges.source() == pmesh);
    assert(m_halfedges_ext.source() == pmesh);
    assert(m_holes.source() == pmesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::size_function_type quetzal::brep::Face<Traits, M>::m_halfedges_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    return mesh.face(id).halfedge_ids().size();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_first = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    if (m_halfedges_size(mesh, id) == 0)
    {
        return std::nullopt;
    }

    return 0;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_last = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    auto size = m_halfedges_size(mesh, id);
    if (size == 0)
    {
        return std::nullopt;
    }

    return size - 1;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_end = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    mesh;
    id;
    return std::nullopt;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_forward = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> std::optional<size_t>
{
    auto size = m_halfedges_size(mesh, id);
    if (!i || size == 0 || *i == size - 1)
    {
        return std::nullopt;
    }

    return ++(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_reverse = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> std::optional<size_t>
{
    auto size = m_halfedges_size(mesh, id);
    if (!i || size == 0 || *i == 0)
    {
        return std::nullopt;
    }

    return --(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_element = [](mesh_type& mesh, id_type id, std::optional<size_t> i) -> halfedge_type&
{
    id;

    assert(i);

    auto& halfedge = mesh.halfedge_store()[mesh.face(id).halfedge_ids()[*i]];
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::const_element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_const_element = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> const halfedge_type&
{
    id;

    assert(i);

    const auto& halfedge = mesh.halfedge_store()[mesh.face(id).halfedge_ids()[*i]];
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::size_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    const auto& halfedges = mesh.face(id).halfedges();
    size_t n = std::distance(halfedges.begin(), halfedges.end());

    for (id_type hole_id : mesh.face_store()[id].hole_ids())
    {
        n += mesh.hole_store()[hole_id].halfedges().size();
    }

    return n;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_first = [](const mesh_type& mesh, id_type id) -> halfedges_ext_type::i_type
{
    return {std::nullopt, mesh.face_store()[id].halfedge_id()};
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_last = [](const mesh_type& mesh, id_type id) -> halfedges_ext_type::i_type
{
    const auto& hole_ids = mesh.face_store()[id].hole_ids();
    if (hole_ids.empty())
    {
        return {std::nullopt, mesh.face_store()[id].halfedge_id() != nullid ? mesh.face_store()[id].halfedge().prev_id() : nullid};
    }

    const auto& hole_last = mesh.hole_store()[hole_ids.back()];
    return {hole_ids.size() - 1, hole_last.halfedge_id() != nullid ? hole_last.halfedge().prev_id() : nullid};
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_end = [](const mesh_type& mesh, id_type id) -> halfedges_ext_type::i_type
{
    mesh;
    id;
    return {std::nullopt, nullid};
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_forward = [](const mesh_type& mesh, id_type id, halfedges_ext_type::i_type i) -> halfedges_ext_type::i_type
{
    auto [hole_slot_opt, halfedge_id] = i;

    const auto& hole_ids = mesh.face_store()[id].hole_ids();
    if (hole_ids.empty())
    {
        id_type halfedge_id_next = mesh.halfedge_store()[halfedge_id].next_id();
        return {std::nullopt, halfedge_id_next != mesh.face_store()[id].halfedge_id() ? halfedge_id_next : nullid};
    }

    // Handle perimeter values
    if (!hole_slot_opt.has_value())
    {
        if (halfedge_id == mesh.face_store()[id].halfedge().prev_id())
        {
            return {0, mesh.hole_store()[hole_ids[0]].halfedge_id()};
        }
        else
        {
            return {std::nullopt, mesh.halfedge_store()[halfedge_id].next_id()};
        }
    }

    // Handle last value of last hole
    if (halfedge_id == mesh.hole_store()[hole_ids.back()].halfedge().prev_id())
    {
        return {std::nullopt, nullid};
    }

    size_t hole_slot = *hole_slot_opt;

    // handle last values of holes other than the last
    if (halfedge_id == mesh.hole_store()[hole_ids[hole_slot]].halfedge().prev_id())
    {
        ++hole_slot;
        return {hole_slot, mesh.hole_store()[hole_ids[hole_slot]].halfedge_id()};
    }

    // Now, it's simply the next halfedge in the current hole
    return {hole_slot, mesh.halfedge_store()[halfedge_id].next_id()};
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_reverse = [](const mesh_type& mesh, id_type id, halfedges_ext_type::i_type i) -> halfedges_ext_type::i_type
{
    auto [hole_slot_opt, halfedge_id] = i;

    const auto& hole_ids = mesh.face_store()[id].hole_ids();
    if (hole_ids.empty())
    {
        id_type halfedge_id_prev = mesh.halfedge_store()[halfedge_id].prev_id();
        return {std::nullopt, halfedge_id_prev != mesh.face_store()[id].halfedge().prev_id() ? halfedge_id_prev : nullid};
    }

    // Handle perimeter values
    if (!hole_slot_opt.has_value())
    {
        if (halfedge_id == mesh.face_store()[id].halfedge_id())
        {
            return {std::nullopt, nullid};
        }
        else
        {
            return {std::nullopt, mesh.halfedge_store()[halfedge_id].prev_id()};
        }
    }

    // Handle first value of first hole
    if (halfedge_id == mesh.hole_store()[hole_ids.front()].halfedge_id())
    {
        return {std::nullopt, mesh.face(id).halfedge().prev_id()};
    }

    size_t hole_slot = *hole_slot_opt;

    // handle first values of holes other than the first
    if (halfedge_id == mesh.hole_store()[hole_ids[hole_slot]].halfedge_id())
    {
        --hole_slot;
        return {hole_slot, mesh.hole_store()[hole_ids[hole_slot]].halfedge().prev_id()};
    }

    // Now, it's simply the prev halfedge in the current hole
    return {hole_slot, mesh.halfedge_store()[halfedge_id].prev_id()};
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_element = [](mesh_type& mesh, id_type id, halfedges_ext_type::i_type i) -> halfedge_type&
{
    id;
    auto& halfedge = mesh.halfedge_store()[std::get<1>(i)];
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_ext_type::const_element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_ext_const_element = [](const mesh_type& mesh, id_type id, halfedges_ext_type::i_type i) -> const halfedge_type&
{
    id;
    const auto& halfedge = mesh.halfedge_store()[std::get<1>(i)];
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::size_function_type quetzal::brep::Face<Traits, M>::m_holes_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    // The store rather than face(), as the other iterators do, since a deleted face is still asked for
    // its holes and face() rejects one. A face with no id of its own has none
    return id != nullid ? mesh.face_store()[id].hole_ids().size() : 0;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_holes_first = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    if (m_holes_size(mesh, id) == 0)
    {
        return std::nullopt;
    }

    return 0;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_holes_last = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    auto size = m_holes_size(mesh, id);
    if (size == 0)
    {
        return std::nullopt;
    }

    return size - 1;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_holes_end = [](const mesh_type& mesh, id_type id) -> std::optional<size_t>
{
    mesh;
    id;
    return std::nullopt;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_holes_forward = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> std::optional<size_t>
{
    auto size = m_holes_size(mesh, id);
    if (!i || size == 0 || *i == size - 1)
    {
        return std::nullopt;
    }

    return ++(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_holes_reverse = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> std::optional<size_t>
{
    auto size = m_holes_size(mesh, id);
    if (!i || size == 0 || *i == 0)
    {
        return std::nullopt;
    }

    return --(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::element_function_type quetzal::brep::Face<Traits, M>::m_holes_element = [](mesh_type& mesh, id_type id, std::optional<size_t> i) -> hole_type&
{
    id;

    assert(i);

    auto& hole = mesh.hole_store()[mesh.face_store()[id].hole_ids()[*i]];
    return hole;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::holes_type::const_element_function_type quetzal::brep::Face<Traits, M>::m_holes_const_element = [](const mesh_type& mesh, id_type id, std::optional<size_t> i) -> const hole_type&
{
    id;

    assert(i);

    const auto& hole = mesh.hole_store()[mesh.face_store()[id].hole_ids()[*i]];
    return hole;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Face<Traits, M>& face)
{
    os << static_cast<Flags>(face);
    os << '\t' << face.id();

    if (!face.deleted())
    {
        os << '\t' << face.halfedge_count() << " edges";
        os << '\t' << face.halfedge_id();

        os << '\t';
//        os << '\t' << id_string(face.partner_id(), "border"); // nullid_text ...
        if (face.partner_id() != nullid)
        {
            os << face.partner_id();
        }
        else
        {
            os << "border";
        }

        os << "\ts " << id_string(face.surface_id());
        if (face.surface_id() != nullid)
        {
            os << face.surface_id() << ' ' << face.surface().name();
        }

        os << "\to " << id_string(face.submesh_id());
        if (face.submesh_id() != nullid)
        {
            os << ' ' << face.submesh().name();
        }

        os << '\t' << face.attributes();
//        os << '\t' << face.properties();
    }

    os << '\n';
    return os;
}

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

#endif // QUETZAL_BREP_FACE_HPP
