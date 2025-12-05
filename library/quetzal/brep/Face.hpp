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
#include <ostream>
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

        using traits_type = Traits;
        using mesh_type = M;
        using halfedge_type = mesh_type::halfedge_type;
        using surface_type = mesh_type::surface_type;
        using submesh_type = mesh_type::submesh_type;
        using hole_type = Hole<Traits, M>;
        using holes_type = std::vector<hole_type>;
        using halfedges_type = Elements<mesh_type, halfedge_type, id_type>;
        using attributes_type = mesh_type::face_attributes_type;
        using size_type = mesh_type::size_type;

        Face();
        Face(mesh_type& mesh, id_type id, id_type idSurface, id_type idSubmesh, id_type idHalfedge, const attributes_type& attributes = {}, const Properties& properties = {});
        Face(const Face&) = default;
        Face(Face&&) noexcept = default;
        ~Face() = default;

        Face& operator=(const Face&) = default;
        Face& operator=(Face&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type id() const;

        id_type partner_id() const;
        void set_partner_id(id_type idHalfedge);

        const Face& partner() const;
        Face& partner();

		// The halfedge exposed here is an arbitrary halfedge in the face and must not be expected be consistent between calls

        id_type halfedge_id() const;
        void set_halfedge_id(id_type idHalfedge);

        const halfedge_type& halfedge() const;
        halfedge_type& halfedge();

        // Virtual container interface, iterators for halfedges connected to this face
        size_type halfedge_count() const;
        const halfedges_type& halfedges() const;
        halfedges_type& halfedges();

        id_type surface_id() const;
        void set_surface_id(id_type idSurface);

        const surface_type& surface() const;
        surface_type& surface();

        id_type submesh_id() const;
        void set_submesh_id(id_type idSubmesh);

        const submesh_type& submesh() const;
        submesh_type& submesh();

        size_type hole_count() const;
        const holes_type& holes() const;
        holes_type& holes();

        const hole_type& hole(id_type idHalfedge) const;
        hole_type& hole(id_type idHalfedge);

        hole_type& create_hole(id_type idHalfedge);
        hole_type& insert_hole(const hole_type& hole);
        void remove_hole(id_type idHalfedge);

        const attributes_type& attributes() const;
        attributes_type& attributes();
        void set_attributes(const attributes_type& attributes);

        const Properties& properties() const;
        Properties& properties();

        const geometry::AxisAlignedBoundingBox<typename M::vector_traits>& bounds() const;
        void reset_bounds() const;

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
        id_type m_idPartner;
        id_type m_idHalfedge;
        id_type m_idSurface;
        id_type m_idSubmesh;

        halfedges_type m_halfedges;
        holes_type m_holes;

        attributes_type m_attributes;
        Properties m_properties;

        mutable std::optional<geometry::AxisAlignedBoundingBox<typename M::vector_traits>> m_bounds; // Lazy evaluation

        static halfedges_type::size_function_type m_halfedges_size;
        static halfedges_type::terminal_function_type m_halfedges_first;
        static halfedges_type::terminal_function_type m_halfedges_last;
        static halfedges_type::terminal_function_type m_halfedges_end;
        static halfedges_type::iterate_function_type m_halfedges_forward;
        static halfedges_type::iterate_function_type m_halfedges_reverse;
        static halfedges_type::element_function_type m_halfedges_element;
        static halfedges_type::const_element_function_type m_halfedges_const_element;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const Face<Traits, M>& face);

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
            return t.halfedges().size();
        }

        static point_type position(const brep::Face<Traits>& t, size_t n)
        {
            size_t i = 0;
            for (const auto& halfedge : t.halfedges())
            {
                if (i++ == n)
                {
                    return halfedge.vertex_attributes().position();
                }
            }

            throw std::out_of_range("PositionAccessor::range_check");
        }

        static bool constant_time_acccess()
        {
            return false;
        }
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Face<Traits, M>::Face() :
    Flags(),
    m_pmesh(nullptr),
    m_id(nullid),
    m_idPartner(nullid),
    m_idHalfedge(nullid),
    m_idSurface(nullid),
    m_idSubmesh(nullid),
    m_halfedges(m_pmesh, nullid, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_holes(),
    m_attributes(),
    m_properties(),
    m_bounds()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Face<Traits, M>::Face(mesh_type& mesh, id_type id, id_type idSurface, id_type idSubmesh, id_type idHalfedge, const attributes_type& attributes, const Properties& properties) :
    Flags(),
    m_pmesh(&mesh),
    m_id(id),
    m_idPartner(nullid),
    m_idHalfedge(idHalfedge),
    m_idSurface(idSurface),
    m_idSubmesh(idSubmesh),
    m_halfedges(m_pmesh, id, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_holes(),
    m_attributes(attributes),
    m_properties(properties),
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
    return m_idPartner;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_partner_id(id_type idPartner)
{
    m_idPartner = idPartner;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Face<Traits, M>& quetzal::brep::Face<Traits, M>::partner() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idPartner);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>& quetzal::brep::Face<Traits, M>::partner()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idPartner);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::halfedge_id() const
{
    return m_idHalfedge;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_halfedge_id(id_type idHalfedge)
{
    m_idHalfedge = idHalfedge;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::halfedge_type& quetzal::brep::Face<Traits, M>::halfedge() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedge_type& quetzal::brep::Face<Traits, M>::halfedge()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idHalfedge);
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
quetzal::id_type quetzal::brep::Face<Traits, M>::surface_id() const
{
    return m_idSurface;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_surface_id(id_type idSurface)
{
    m_idSurface = idSurface;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::surface_type& quetzal::brep::Face<Traits, M>::surface() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_idSurface);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::surface_type& quetzal::brep::Face<Traits, M>::surface()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_idSurface);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Face<Traits, M>::submesh_id() const
{
    return m_idSubmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_submesh_id(id_type idSubmesh)
{
    m_idSubmesh = idSubmesh;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Face<Traits, M>::submesh_type& quetzal::brep::Face<Traits, M>::submesh() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_idSubmesh);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::submesh_type& quetzal::brep::Face<Traits, M>::submesh()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_idSubmesh);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::size_type quetzal::brep::Face<Traits, M>::hole_count() const
{
    return m_holes.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::hole(id_type idHalfedge) const
{
    for (const auto& hole : m_holes)
    {
        if (hole.halfedge_id() == idHalfedge)
        {
            return hole;
        }
    }

    assert(false);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::hole(id_type idHalfedge)
{
    for (auto& hole : m_holes)
    {
        if (hole.halfedge_id() == idHalfedge)
        {
            return hole;
        }
    }

    assert(false);
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
typename quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::create_hole(id_type idHalfedge)
{
    assert(m_pmesh != nullptr);
    return m_holes.emplace_back(*m_pmesh, idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::hole_type& quetzal::brep::Face<Traits, M>::insert_hole(const hole_type& hole)
{
    assert(m_pmesh != nullptr);
    return m_holes.emplace_back(hole);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::remove_hole(id_type idHalfedge)
{
    for (auto i = m_holes.begin(); i != m_holes.end(); ++i)
    {
        if (i->halfedge_id() == idHalfedge)
        {
            m_holes.erase(i);
            return;
        }
    }

    assert(false);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::brep::Face<Traits, M>::attributes_type& quetzal::brep::Face<Traits, M>::attributes() const
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
typename const quetzal::Properties& quetzal::brep::Face<Traits, M>::properties() const
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

    return m_bounds.value();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::reset_bounds() const
{
    m_bounds = std::nullopt;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Face<Traits, M>::border() const
{
    return m_idPartner == nullid;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_border()
{
    m_idPartner = nullid;
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

    return m_id < m_pmesh->face_store_count() && m_pmesh->valid_halfedge_id(m_idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_halfedges.set_source(mesh);

    for (auto& hole : m_holes)
    {
        hole.set_mesh(mesh);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::set_id(id_type id)
{
    m_id = id;
    m_halfedges.set_id(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Face<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    assert(m_halfedges.source() == pmesh);

    for (auto& hole : m_holes)
    {
        hole.check_mesh(pmesh);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::size_function_type quetzal::brep::Face<Traits, M>::m_halfedges_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    const auto& halfedges = mesh.face(id).halfedges();
    return std::distance(halfedges.begin(), halfedges.end());
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_first = [](const mesh_type& mesh, id_type id) -> id_type
{
    return mesh.face_store()[id].halfedge_id();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_last = [](const mesh_type& mesh, id_type id) -> id_type
{
    return mesh.face_store()[id].halfedge().prev_id();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Face<Traits, M>::m_halfedges_end = [](const mesh_type& mesh, id_type id) -> id_type
{
    mesh;
    id;
    return nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_forward = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    i = mesh.halfedge_store()[i].next_id();
    return i != m_halfedges_first(mesh, id) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Face<Traits, M>::m_halfedges_reverse = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    i = mesh.halfedge_store()[i].prev_id();
    return i != m_halfedges_last(mesh, id) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_element = [](mesh_type& mesh, id_type id, id_type i) -> quetzal::brep::Face<Traits, M>::halfedge_type&
{
    id;
    auto& halfedge = mesh.halfedge_store()[i];
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Face<Traits, M>::halfedges_type::const_element_function_type quetzal::brep::Face<Traits, M>::m_halfedges_const_element = [](const mesh_type& mesh, id_type id, id_type i) -> const quetzal::brep::Face<Traits, M>::halfedge_type&
{
    id;
    const auto& halfedge = mesh.halfedge_store()[i];
    return halfedge;
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

#endif // QUETZAL_BREP_FACE_HPP
