#if !defined(QUETZAL_BREP_BORDERFRAGMENT_HPP)
#define QUETZAL_BREP_BORDERFRAGMENT_HPP
//------------------------------------------------------------------------------
// brep
// BorderFragment.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "inversion.hpp"
#include "validation.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include <iostream>
#include <set>
#include <string>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    template<typename Traits>
    using transform_texcoord_type = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_type;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class BorderFragment : public Flags
    {
    public:

        using traits_type = Traits;
        using mesh_type = M;
        using size_type = Traits::size_type;
        using face_ids_type = std::set<id_type>;
        using submesh_type = mesh_type::submesh_type;

        BorderFragment(mesh_type& mesh, id_type halfedge_id = nullid);
        BorderFragment(const BorderFragment&) = default;
        BorderFragment(BorderFragment&&) noexcept = default;
        ~BorderFragment() = default;

        BorderFragment& operator=(const BorderFragment&) = default;
        BorderFragment& operator=(BorderFragment&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type halfedge_id() const;
        void set_halfedge_id(id_type halfedge_id);

        id_type submesh_id() const;
        void set_submesh_id(id_type submesh_id);

        size_type face_count() const;

        const face_ids_type& face_ids() const;
        face_ids_type& face_ids();

        // probably unnecessary ...
        bool contains_face(id_type id) const;
        void insert_face(id_type id);

        void invert(transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);
        void delete_(std::function<bool(const typename M::halfedge_type&)> border_condition = [](const typename M::halfedge_type& halfedge) -> bool { return halfedge.border(); });

        bool empty() const;

        void clear();

    private:

        void init(id_type halfedge_id);

        mesh_type* m_pmesh;
        id_type m_halfedge_id;
        face_ids_type m_face_ids;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const BorderFragment<Traits, M>& border_fragment);

    template<typename Traits, typename M>
    void dump(const BorderFragment<Traits, M>& border_fragment);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::BorderFragment<Traits, M>::BorderFragment(mesh_type& mesh, id_type halfedge_id) :
    Flags(),
    m_pmesh(&mesh),
    m_halfedge_id(halfedge_id),
    m_face_ids()
{
    if (m_pmesh->halfedge(m_halfedge_id).face().marked())
    {
        set_marked();
    }
    else
    {
        init(m_halfedge_id);
    }
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::BorderFragment<Traits, M>::mesh_type& quetzal::brep::BorderFragment<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::BorderFragment<Traits, M>::mesh_type& quetzal::brep::BorderFragment<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::BorderFragment<Traits, M>::halfedge_id() const
{
    return m_halfedge_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::set_halfedge_id(id_type halfedge_id)
{
    m_halfedge_id = halfedge_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::BorderFragment<Traits, M>::submesh_id() const
{
    assert(m_pmesh != nullptr);
    return m_halfedge_id == nullid ? nullid : m_pmesh->halfedge(m_halfedge_id).submesh_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::set_submesh_id(id_type submesh_id)
{
    assert(m_pmesh != nullptr);
    assert(!m_face_ids.empty());

    for (id_type face_id : m_face_ids)
    {
        auto& face = m_pmesh->face(face_id);
        auto surface_name = face.surface().name();
        m_pmesh->unlink_face(face_id);
        m_pmesh->add_surface_face(submesh_id, surface_name, face_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::BorderFragment<Traits, M>::size_type quetzal::brep::BorderFragment<Traits, M>::face_count() const
{
    return m_face_ids.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::BorderFragment<Traits, M>::face_ids_type& quetzal::brep::BorderFragment<Traits, M>::face_ids() const
{
    return m_face_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::BorderFragment<Traits, M>::face_ids_type& quetzal::brep::BorderFragment<Traits, M>::face_ids()
{
    return m_face_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::BorderFragment<Traits, M>::contains_face(id_type id) const
{
    return m_face_ids.contains(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::insert_face(id_type id)
{
    assert(id != nullid);
    assert(!m_face_ids.contains(id));

    m_face_ids.insert(id);
    if (m_halfedge_id == nullid)
    {
        m_halfedge_id = mesh.face(id).halfedge_id();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::invert(transform_texcoord_type<Traits> transform_texcoord)
{
    assert(m_pmesh != nullptr);

    for (id_type face_id : m_face_ids)
    {
        auto& face = m_pmesh->face(face_id);
        assert(!face.marked());
        brep::invert(face, transform_texcoord);
        face.set_marked();
    }

    set_marked();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::delete_(std::function<bool(const typename M::halfedge_type&)> border_condition)
{
    assert(m_pmesh != nullptr);

    // Check that this fragment has not already been deleted
    if (m_face_ids.empty() || m_pmesh->face(m_face_ids.front()).deleted())
    {
        return;
    }

    for (id_type face_id : m_face_ids)
    {
        auto& face = mesh.face(face_id);
        mesh.delete_face(face_id);
    }

    clear();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::init(id_type halfedge_id)
{
    assert(m_pmesh != nullptr);

    id_type face_id = m_pmesh->halfedge(halfedge_id).face_id();
    m_face_ids.insert(face_id);

    const auto& face = m_pmesh->face(face_id);
    face.set_visited();

    for (const auto& halfedge : face.halfedges())
    {
        id_type partner_id = halfedge.partner_id();
        if (partner_id == nullid || m_pmesh->halfedge(partner_id).face().visited())
        {
            continue;
        }

        init(partner_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::BorderFragment<Traits, M>::empty() const
{
    return m_face_ids.empty();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::BorderFragment<Traits, M>::clear()
{
    m_halfedge_id = nullid;
    m_face_ids.clear();
    set_marked(false);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const BorderFragment<Traits, M>& border_fragment)
{
    os << static_cast<Flags>(border_fragment);
    os << "\thalfedge " << id_string(border_fragment.halfedge_id());

    os << '\t' << border_fragment.face_count() << " faces [";
    for (id_type id : border_fragment.face_ids())
    {
        os << ' ' << id;
    }
    os << "]\n";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::dump(const BorderFragment<Traits, M>& border_fragment)
{
    for (id_type face_id : border_fragment.face_ids())
    {
        dump_face(border_fragment.mesh(), face_id);
    }

    return;
}

#endif // QUETZAL_BREP_BORDERFRAGMENT_HPP
