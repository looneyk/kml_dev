#if !defined(QUETZAL_BREP_SURFACE_HPP)
#define QUETZAL_BREP_SURFACE_HPP
//------------------------------------------------------------------------------
// brep
// Surface.hpp
//------------------------------------------------------------------------------

#include "Seam.hpp"
#include "id.hpp"
#include "quetzal/common/Elements.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <vector>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Surface : public Flags
    {
    public:

        using traits_type = Traits;
        using mesh_type = M;
        using size_type = Traits::size_type;
        using vertex_type = mesh_type::vertex_type;
        using face_type = mesh_type::face_type;
        using face_ids_type = std::set<id_type>;
        using faces_type = Elements<mesh_type, face_type, face_ids_type::iterator>;
        using halfedge_type = mesh_type::halfedge_type;
        using seam_type = Seam<Traits, M>;
        using seams_type = std::vector<seam_type>;
        using submesh_type = mesh_type::submesh_type;
        using attributes_type = mesh_type::surface_attributes_type;

        Surface();
        Surface(mesh_type& mesh, id_type id, const std::string& name, id_type submesh_id, const attributes_type& attributes = {}, const Properties& properties = {});
        Surface(const Surface&) = default;
        Surface(Surface&&) noexcept = default;
        ~Surface() = default;

        Surface& operator=(const Surface&) = default;
        Surface& operator=(Surface&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type id() const;

        const std::string& name() const;
        void set_name(const std::string& name);

        std::string extended_name() const;

        id_type submesh_id() const;
        void set_submesh_id(id_type submesh_id);

        const submesh_type& submesh() const;
        submesh_type& submesh();

        size_type face_count() const;
        const faces_type& faces() const;
        faces_type& faces();

        const face_type& face(id_type id) const;
        face_type& face(id_type id);

        const face_ids_type& face_ids() const;
        face_ids_type& face_ids();

        bool contains_face(id_type id) const;
        void insert_face(id_type id); // add face to list
        void link_face(id_type id); // add face to list and set face surface id
        void unlink_face(id_type id); // remove face from list and clear face surface id

        // Each seam starts on a surface partner change, if any
        size_type seam_count() const;
        const seams_type& seams() const;
        seams_type& seams();

        void set_regenerate_seams(bool b = true);

        std::vector<id_type> find_seam_sections(id_type surface_partner_id);
        std::vector<id_type> find_seam_sections(const std::string& surface_partner_name);

        const attributes_type& attributes() const;
        attributes_type& attributes();
        void set_attributes(const attributes_type& attributes);

        const Properties& properties() const;
        Properties& properties();

        const geometry::AxisAlignedBoundingBox<typename M::vector_traits>& bounds() const;
        void reset_bounds() const;

        bool empty() const;

        void clear(); // Clears contents, but does not change id or name
        void reset_all() const override; // Flags

        // Internal use, only by Mesh
        void set_mesh(mesh_type& mesh);
        void set_id(id_type id);
        void check_mesh(const mesh_type* const pmesh) const;

    private:

        void check_regenerate_seams();
        void generate_seams();
        void generate_seam(id_type halfedge_id);
        std::array<id_type, 2> generate_seam(id_type next_id, id_type prev_id, id_type halfedge_id, id_type seam_id);

        mesh_type* m_pmesh;
        id_type m_id;
        std::string m_name;
        id_type m_submesh_id;
        face_ids_type m_face_ids;
        faces_type m_faces;

        seams_type m_seams;

        bool m_regenerate_seams;
        bool m_generating_seams;

        attributes_type m_attributes;
        Properties m_properties;
        
        mutable std::optional<geometry::AxisAlignedBoundingBox<typename M::vector_traits>> m_bounds; // Lazy evaluation


        static faces_type::size_function_type m_faces_size;
        static faces_type::terminal_function_type m_faces_first;
        static faces_type::terminal_function_type m_faces_last;
        static faces_type::terminal_function_type m_faces_end;
        static faces_type::iterate_function_type m_faces_forward;
        static faces_type::iterate_function_type m_faces_reverse;
        static faces_type::element_function_type m_faces_element;
        static faces_type::const_element_function_type m_faces_const_element;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const Surface<Traits, M>& surface);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Surface<Traits, M>::Surface() :
    Flags(),
    m_pmesh(nullptr),
    m_id(nullid),
    m_name(),
    m_submesh_id(nullid),
    m_face_ids(),
    m_faces(m_pmesh, nullid, m_faces_size, m_faces_first, m_faces_last, m_faces_end, m_faces_forward, m_faces_reverse, m_faces_element, m_faces_const_element),
    m_seams(),
    m_regenerate_seams(false),
    m_generating_seams(false),
    m_attributes(),
    m_properties(),
    m_bounds()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Surface<Traits, M>::Surface(mesh_type& mesh, id_type id, const std::string& name, id_type submesh_id, const attributes_type& attributes, const Properties& properties) :
    Flags(),
    m_pmesh(&mesh),
    m_id(id),
    m_name(name),
    m_submesh_id(submesh_id),
    m_face_ids(),
    m_faces(m_pmesh, id, m_faces_size, m_faces_first, m_faces_last, m_faces_end, m_faces_forward, m_faces_reverse, m_faces_element, m_faces_const_element),
    m_seams(),
    m_regenerate_seams(false),
    m_generating_seams(false),
    m_attributes(attributes),
    m_properties(properties),
    m_bounds()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::mesh_type& quetzal::brep::Surface<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::mesh_type& quetzal::brep::Surface<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Surface<Traits, M>::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const std::string& quetzal::brep::Surface<Traits, M>::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_name(const std::string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::string quetzal::brep::Surface<Traits, M>::extended_name() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->extended_surface_name(m_submesh_id, m_name);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Surface<Traits, M>::submesh_id() const
{
    return m_submesh_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_submesh_id(id_type submesh_id)
{
    m_submesh_id = submesh_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::submesh_type& quetzal::brep::Surface<Traits, M>::submesh() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_submesh_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::submesh_type& quetzal::brep::Surface<Traits, M>::submesh()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->submesh(m_submesh_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::size_type quetzal::brep::Surface<Traits, M>::face_count() const
{
    return m_face_ids.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::faces_type& quetzal::brep::Surface<Traits, M>::faces() const
{
    return m_faces;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type& quetzal::brep::Surface<Traits, M>::faces()
{
    return m_faces;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::face_type& quetzal::brep::Surface<Traits, M>::face(id_type id) const
{
    assert(m_face_ids.contains(id));
    return m_pmesh->face(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::face_type& quetzal::brep::Surface<Traits, M>::face(id_type id)
{
    return m_pmesh->face(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::face_ids_type& quetzal::brep::Surface<Traits, M>::face_ids() const
{
    return m_face_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::face_ids_type& quetzal::brep::Surface<Traits, M>::face_ids()
{
    return m_face_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Surface<Traits, M>::contains_face(id_type id) const
{
    return m_face_ids.contains(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::insert_face(id_type id)
{
    assert(id != nullid);
    assert(!m_face_ids.contains(id));
    m_face_ids.insert(id);
    set_regenerate_seams();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::link_face(id_type id)
{
    assert(m_pmesh != nullptr);
    m_pmesh->face(id).set_surface_id(m_id);
    insert_face(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::unlink_face(id_type id)
{
    assert(m_pmesh != nullptr);
    assert(id != nullid);
    assert(m_face_ids.contains(id));

    m_face_ids.erase(id);
    m_pmesh->face(id).set_surface_id(nullid);

    set_regenerate_seams();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::size_type quetzal::brep::Surface<Traits, M>::seam_count() const
{
    check_regenerate_seams();
    return m_seams.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Surface<Traits, M>::seams_type& quetzal::brep::Surface<Traits, M>::seams() const
{
    check_regenerate_seams();
    return m_seams;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::seams_type& quetzal::brep::Surface<Traits, M>::seams()
{
    check_regenerate_seams();
    return m_seams;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_regenerate_seams(bool b)
{
    m_regenerate_seams = b;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Surface<Traits, M>::find_seam_sections(id_type surface_partner_id)
{
    check_regenerate_seams();
    std::vector<id_type> ids;

    for (const auto& seam : m_seams)
    {
        auto seam_ids = seam.find_seams(surface_partner_id);
        ids.append_range(seam_ids);
    }

    return ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Surface<Traits, M>::find_seam_sections(const std::string& surface_partner_name)
{
    check_regenerate_seams();
    std::vector<id_type> ids;

    for (const auto& seam : m_seams)
    {
        auto seam_ids = seam.find_seam_sections(surface_partner_name);
        ids.append_range(seam_ids);
    }

    return ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::brep::Surface<Traits, M>::attributes_type& quetzal::brep::Surface<Traits, M>::attributes() const
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::attributes_type& quetzal::brep::Surface<Traits, M>::attributes()
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_attributes(const attributes_type& attributes)
{
    m_attributes = attributes;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::Properties& quetzal::brep::Surface<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Surface<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::geometry::AxisAlignedBoundingBox<typename M::vector_traits>& quetzal::brep::Surface<Traits, M>::bounds() const
{
    if (!m_bounds)
    {
        geometry::AxisAlignedBoundingBox<typename M::vector_traits>  bounds = {};
        std::ranges::for_each(faces(), [&bounds](auto& face) { bounds.insert(face.bounds()); });
        m_bounds = bounds;
    }

    return *m_bounds;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::reset_bounds() const
{
    m_bounds = std::nullopt;
    std::ranges::for_each(faces(), [](auto& face) { face.reset_bounds(); });
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Surface<Traits, M>::empty() const
{
    return m_face_ids.empty();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::clear()
{
    m_submesh_id = nullid;
    m_face_ids.clear();
    m_seams.clear();
    m_regenerate_seams = false;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::reset_all() const
{
    for (auto& face : m_faces)
    {
        face.reset_all();
    }

    for (const auto& seam : m_seams)
    {
        seam.reset();
    }

    Flags::reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_faces.set_source(mesh);

    for (auto& seam : m_seams)
    {
        seam.set_mesh(mesh);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::set_id(id_type id)
{
    m_id = id;
    m_faces.set_id(id);

    for (auto& seam : m_seams)
    {
        seam.set_surface_id(id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    assert(m_faces.check_source(pmesh));

    for (auto& seam : m_seams)
    {
        seam.check_mesh(pmesh);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::check_regenerate_seams()
{
    if (m_regenerate_seams && !m_generating_seams)
    {
        m_generating_seams = true;
        generate_seams();
        m_regenerate_seams = false;
        m_generating_seams = false;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::generate_seams()
{
    assert(m_pmesh != nullptr);

    reset();

    for (const auto& face : faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.marked())
            {
                continue;
            }

            if (halfedge.surface_border())
            {
                id_type halfedge_id = halfedge.id();
                id_type halfedge_id_prev = prev_surface_halfedge_id(*m_pmesh, halfedge_id);
                while (halfedge_id_prev == halfedge_id)
                {
                    halfedge_id == halfedge_id_prev;
                }

                generate_seam(halfedge_id);
            }
            else
            {
                halfedge.set_marked();
                halfedge.partner().set_marked();
            }
        }
    }
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Surface<Traits, M>::generate_seam(id_type halfedge_id)
{
    assert(m_pmesh != nullptr);
    assert(m_pmesh->halfedge(halfedge_id).surface_id() != m_pmesh->halfedge(halfedge_id).partner_surface_id());

    auto& seam = m_seams.emplace_back(*m_pmesh, m_id, halfedge_id);

    id_type halfedge_id0 = halfedge_id;
    id_type halfedge_id = halfedge_id0;
    do
    {
        id_type halfedge_id_next = next_surface_halfedge_id(*m_pmesh, halfedge_id);
        if (m_pmesh->halfedge(halfedge_id_next).partner_surface_id() != m_pmesh->halfedge(halfedge_id).partner_surface_id())
        {
            return;
        }

        m_pmesh.halfedge(halfedge_id).set_marked();
        halfedge_id = halfedge_id_next;
    }
    while (halfedge_id != halfedge_id0);

    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::array<quetzal::id_type, 2> quetzal::brep::Surface<Traits, M>::generate_seam(id_type next_id, id_type prev_id, id_type halfedge_id, id_type seam_id)
{
    assert(m_pmesh != nullptr);
    assert(m_pmesh->halfedge(halfedge_id).surface_border());

    auto& seam = seams()[seam_id];
    auto& seam = seam.seams().emplace_back(*m_pmesh, next_id, prev_id, halfedge_id, seam_id);

    id_type halfedge_id0 = halfedge_id;

    do
    {
        m_pmesh->halfedge(halfedge_id).set_marked();
        halfedge_id = next_surface_halfedge_id(*m_pmesh, halfedge_id);
    } while (halfedge_id != halfedge_id0 && m_pmesh->halfedge(halfedge_id).surface_id() == m_id);

    return {seam.id(), halfedge_id};
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::size_function_type quetzal::brep::Surface<Traits, M>::m_faces_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    return mesh.surface(id).face_ids().size();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::terminal_function_type quetzal::brep::Surface<Traits, M>::m_faces_first = [](const mesh_type& mesh, id_type id) -> face_ids_type::iterator
{
    return mesh.surface(id).face_ids().begin();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::terminal_function_type quetzal::brep::Surface<Traits, M>::m_faces_last = [](const mesh_type& mesh, id_type id) -> face_ids_type::iterator
{
    assert(!mesh.surface(id).face_ids().empty());
    return --mesh.surface(id).face_ids().end();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::terminal_function_type quetzal::brep::Surface<Traits, M>::m_faces_end = [](const mesh_type& mesh, id_type id) -> face_ids_type::iterator
{
    return mesh.surface(id).face_ids().end();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::iterate_function_type quetzal::brep::Surface<Traits, M>::m_faces_forward = [](const mesh_type& mesh, id_type id, face_ids_type::iterator i) -> face_ids_type::iterator
{
    assert(i != m_faces_end(mesh, id));
    return std::next(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::iterate_function_type quetzal::brep::Surface<Traits, M>::m_faces_reverse = [](const mesh_type& mesh, id_type id, face_ids_type::iterator i) -> face_ids_type::iterator
{
    assert(i != m_faces_first(mesh, id));
    return std::prev(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::element_function_type quetzal::brep::Surface<Traits, M>::m_faces_element = [](mesh_type& mesh, id_type id, face_ids_type::iterator i) -> quetzal::brep::Surface<Traits, M>::face_type&
{
    assert(i != m_faces_end(mesh, id));
    return mesh.face(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Surface<Traits, M>::faces_type::const_element_function_type quetzal::brep::Surface<Traits, M>::m_faces_const_element = [](const mesh_type& mesh, id_type id, face_ids_type::iterator i) -> const quetzal::brep::Surface<Traits, M>::face_type&
{
    assert(i != m_faces_end(mesh, id));
    return mesh.face(*i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Surface<Traits, M>& surface)
{
    os << static_cast<Flags>(surface);
    os << '\t' << surface.id();

    if (!surface.deleted())
    {
        os << '\t' << surface.name();
        os << "\to ";
        if (surface.submesh_id() != nullid)
        {
            os << surface.submesh_id() << ' ' << surface.submesh().name();
        }
        else
        {
            os << "nullid";
        }

        os << '\t' << surface.face_count() << " faces (";
        for (id_type id : surface.face_ids())
        {
            os << ' ' << id;
        }
        os << ')';

        os << '\t' << surface.attributes();
        os << '\t' << surface.properties();
    }

    os << '\n';
    return os;
}

#endif // QUETZAL_BREP_SURFACE_HPP
