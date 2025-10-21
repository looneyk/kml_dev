#if !defined(QUETZAL_BREP_MESHBUILDER_HPP)
#define QUETZAL_BREP_MESHBUILDER_HPP
//------------------------------------------------------------------------------
// brep
// MeshBuilder.hpp
//------------------------------------------------------------------------------

#include "geometry.hpp"
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace quetzal::brep
{

    template<typename M>
    class MeshBuilder
    {
    public:

        using mesh_type = M;
        using point_type = M::point_type;
        using vector_type = point_type::vector_type;

        MeshBuilder();
        explicit MeshBuilder(mesh_type& mesh);
        MeshBuilder(const MeshBuilder&) = default;
        MeshBuilder(MeshBuilder&&) noexcept = default;
        ~MeshBuilder() = default;

        MeshBuilder& operator=(const MeshBuilder&) = default;
        MeshBuilder& operator=(MeshBuilder&&) = default;

        mesh_type& mesh();
        void set_mesh(mesh_type& mesh);

        // Add geometry
        id_type submesh(const std::string& name);
        id_type surface(id_type submesh_id, const std::string& name, vector_type normal);
        id_type face(id_type surface_id, vector_type normal, const std::vector<point_type>& points = {}); // soon may not need this any more ...
        id_type face(id_type surface_id, vector_type normal, std::span<const point_type> points = {});
        id_type surface_face(id_type submesh_id, const std::string& name, vector_type normal, const std::vector<point_type>& points = {}); // soon may not need this any more ...
        id_type surface_face(id_type submesh_id, const std::string& name, vector_type normal, std::span<const point_type> points = {});
        // should be span, not necessarily points, but av, ...

        // Adds a face and surface for each remaining border opening
        void close_mesh();

    private:

        void find_borders();
        id_type find_partner(id_type idHalfedge);

        mesh_type* m_pmesh;
        std::set<id_type> m_borders;
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::MeshBuilder<M>::MeshBuilder() :
    m_pmesh(nullptr),
    m_borders()
{
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::MeshBuilder<M>::MeshBuilder(mesh_type& mesh) :
    m_pmesh(&mesh),
    m_borders()
{
    assert(m_pmesh != nullptr);
    find_borders();
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::brep::MeshBuilder<M>::mesh_type& quetzal::brep::MeshBuilder<M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::MeshBuilder<M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    find_borders();
    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::submesh(const std::string& name)
{
    assert(m_pmesh != nullptr);
    return m_pmesh->create_submesh(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::surface(id_type submesh_id, const std::string& name, vector_type normal)
{
    assert(m_pmesh != nullptr);

    typename M::surface_attributes_type as{normal};
    return m_pmesh->create_surface(submesh_id, name, as);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::face(id_type surface_id, vector_type normal, const std::vector<point_type>& points)
{
    return face(surface_id, normal, std::span<const point_type>(points));
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::face(id_type surface_id, vector_type normal, std::span<const point_type> points)
{
    assert(m_pmesh != nullptr);

    size_t nh = m_pmesh->halfedge_count();
    typename M::face_attributes_type af{normal};
    id_type face_id = m_pmesh->create_face(surface_id, nh, af);

    size_t n = points.size();
    for (size_t i = 0; i < n; ++i)
    {
        const auto& point = points[i];
        typename M::vertex_attributes_type av{point, normal, {}};
        id_type idNext = i < n - 1 ? nh + i + 1 : nh;
        id_type idPrev = i > 0 ? nh + i - 1 : nh + n - 1;
        m_pmesh->create_halfedge_vertex(nullid, idNext, idPrev, face_id, av);
    }

    for (size_t i = 0; i < n; ++i)
    {
        id_type id = nh + i;
        id_type idPartner = find_partner(id);
        if (idPartner != nullid)
        {
            m_pmesh->halfedge(id).set_partner_id(idPartner);
            m_pmesh->halfedge(idPartner).set_partner_id(id);
            m_borders.erase(idPartner);
        }
        else
        {
            m_borders.insert(id);
        }
    }

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::surface_face(id_type submesh_id, const std::string& name, vector_type normal, const std::vector<point_type>& points)
{
    return surface_face(submesh_id, name, normal, std::span<const point_type>(points));
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::surface_face(id_type submesh_id, const std::string& name, vector_type normal, std::span<const point_type> points)
{
    id_type surface_id = surface(submesh_id, name, normal);
    id_type face_id = face(surface_id, normal, points);

    if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
    {
        calculate_face_texcoords(mesh(), face_id);
    }

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::MeshBuilder<M>::close_mesh()
{
    assert(m_pmesh != nullptr);

    while (!m_borders.empty())
    {
        id_type idHalfedge = *m_borders.begin();
        id_type submesh_id = m_pmesh->halfedge(idHalfedge).face().submesh_id();

        std::string name = 's' + std::to_string(m_pmesh->surface_store_count());

        id_type surface_id = m_pmesh->create_surface(submesh_id, name);
        id_type face_id = create_border_face(*m_pmesh, idHalfedge, vector_type::z_direction(), surface_id); // dummy normal, will be calculated next

        auto& face = m_pmesh->face(face_id);
        set_facet_normal(face);
        for (auto& halfedge : face.halfedges())
        {
            m_borders.erase(halfedge.partner_id());
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::MeshBuilder<M>::find_borders()
{
    assert(m_pmesh != nullptr);

    m_borders.clear();
    for (const auto& halfedge : m_pmesh->halfedges())
    {
        if (halfedge.border())
        {
            m_borders.insert(halfedge.id());
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::MeshBuilder<M>::find_partner(id_type idHalfedge)
{
    assert(m_pmesh != nullptr);

    const auto& halfedge = m_pmesh->halfedge(idHalfedge);
    for (auto idBorder : m_borders)
    {
        const auto& border = m_pmesh->halfedge(idBorder);

        // float_eq? only if necessary ...
//        if (halfedge.vertex_attributes().position() == border.next().vertex_attributes().position() &&
//            halfedge.next().vertex_attributes().position() == border.vertex_attributes().position())
        if (float_eq(halfedge.vertex_attributes().position(), border.next().vertex_attributes().position()) &&
            float_eq(halfedge.next().vertex_attributes().position(), border.vertex_attributes().position()))
        {
            return idBorder;
        }
    }

    return nullid;
}

#endif // QUETZAL_BREP_MESHBUILDER_HPP
