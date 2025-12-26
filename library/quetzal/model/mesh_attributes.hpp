#if !defined(QUETZAL_MODEL_MESH_ATTRIBUTES_HPP)
#define QUETZAL_MODEL_MESH_ATTRIBUTES_HPP
//------------------------------------------------------------------------------
// model
// mesh_attributes.hpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/floating_point.hpp"
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

#include <iostream>

namespace quetzal::model
{

    // Works with Mesh, Submesh, Surface
    template<typename U>
    void transform(U& u, const typename U::traits_type::matrix_type& matrix);

    // Works with Mesh, Submesh, Surface
    template<typename U>
    void transform(U& u, const typename U::traits_type::matrix_type& matrixPosition, const typename U::traits_type::matrix_type& matrixDirection);

    // For use with translation and/or uniform scaling only when there is no need to transform normals
    // Works with Mesh, Submesh, Surface
    template<typename U>
    void transform_positions(U& u, const typename U::traits_type::matrix_type& matrix);

    // Works with Mesh, Submesh, Surface
    template<typename U>
    void transform(U& u, std::function<void(typename U::vertex_type&)> fv, std::optional<std::function<void(typename U::face_type&)>> ff = std::nullopt);

    // Works only with Mesh
    // Function f calculates new vertex attributes
    // No optimization, no surface or seam cognizance, deprecated ...
    template<typename U>
    void transform_vertices(U& u, std::function<typename U::traits_type::vertex_attributes_type(const typename U::traits_type::vertex_attributes_type&)> f);

    // Works with Mesh, Submesh, Surface
    // Function f calculates new vertex positions
    // Once all those are set, then new face and surface vertex normals are calculated
    // Reduces calculations for coincident vertices in the same surface
    template<typename U>
    void transform(U& u, std::function<typename U::traits_type::point_type(const typename U::traits_type::point_type&)> f);

    // Works with Mesh, Submesh, Surface
    // Function f calculates new vertex positions
    // Reduces calculations for coincident vertices in the same surface
    template<typename U>
    void transform_positions(U& u, std::function<typename U::traits_type::point_type(const typename U::traits_type::point_type&)> f);

    // Works with Mesh, Submesh, Surface
    // Function f calculates new vertex normals
    // Reduces calculations for coincident vertices in the same surface
    template<typename U>
    void recalculate_normals(U& u);

    template<typename M>
    void transform_border_fragment_position(M& mesh, id_type idHalfedge, const typename M::matrix_type& matrix, bool bReset = true);

    template<typename M>
    void calculate_spherical_normals(M& mesh, const typename M::point_type& center = {});

    // Calculate texcoord from the spherical coordinates of the vertex normal vector
    template<typename M>
    typename M::vertex_attributes_type::texcoord_type calculate_spherical_texcoord(const typename M::vector_type& normal);

    // Calculate texcoords to wrap a sphere where the vertex normals have already been calculated
    template<typename M>
    void calculate_spherical_texcoords(M& mesh);

    // Fix edge texcoords that span the 0/2Pi boundary by shifting the lower one by +1 ...
    // needs refinement/generalization ...
    template<typename F>
    void fix_spherical_texcoords(F& face);

    // this does arbitrary orientation, take an up vector to align textures? then fit triangles to unit square? ...
    template<typename M>
    void apply_triangular_face_texcoords(M& mesh);

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform(U& u, const typename U::traits_type::matrix_type& matrix)
{
    transform(u, matrix, geometry::direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform(U& u, const typename U::traits_type::matrix_type& matrixPosition, const typename U::traits_type::matrix_type& matrixDirection)
{
    if constexpr (std::same_as<U, brep::Mesh<typename U::traits_type>>) // separate functions ...
    {
        for (auto& vertex : u.vertices())
        {
            vertex.attributes().transform(matrixPosition, matrixDirection);
        }

        for (auto& face : u.faces())
        {
            face.attributes().transform(matrixPosition, matrixDirection);
        }
    }
    else
    {
        u.reset_all();

        for (auto& face : u.faces())
        {
            for (auto& halfedgeFace : face.halfedges())
            {
                auto& vertex = halfedgeFace.vertex();
                if (vertex.marked())
                {
                    continue;
                }

                auto position = vertex.attributes().position() * matrixPosition;

                for (auto& halfedge : vertex.halfedges())
                {
                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex_attributes().set_normal(normalize(halfedge.vertex_attributes().normal() * matrixDirection));
                    halfedge.vertex().set_marked();
                }
            }

            face.attributes().transform(matrixPosition, matrixDirection);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform_positions(U& u, const typename U::traits_type::matrix_type& matrix)
{
    if constexpr (std::same_as<U, brep::Mesh<typename U::traits_type>>) // separate functions ...
    {
        for (auto& vertex : u.vertices())
        {
            vertex.attributes().transform_position(matrix);
        }
    }
    else
    {
        u.reset_all();

        for (auto& face : u.faces())
        {
            for (auto& halfedgeFace : face.halfedges())
            {
                auto& vertex = halfedgeFace.vertex();
                if (vertex.marked())
                {
                    continue;
                }

                auto positionOrig = vertex.attributes().position();
                auto position = vertex.attributes().position() * matrix;

                for (auto& halfedge : vertex.halfedges())
                {
                    assert(float_eq(halfedge.vertex_attributes().position(), positionOrig));
                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex().set_marked();
                }
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform(U& u, std::function<void(typename U::vertex_type&)> fv, std::optional<std::function<void(typename U::face_type&)>> ff)
{
    for (auto& face : u.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            fv(halfedge.vertex());
        }

        if (ff)
        {
            (*ff)(face);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform_vertices(U& u, std::function<typename U::traits_type::vertex_attributes_type(const typename U::traits_type::vertex_attributes_type&)> f)
{
    for (auto& face : u.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            vertex.set_attributes(f(vertex.attributes()));
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform(U& u, std::function<typename U::traits_type::point_type(const typename U::traits_type::point_type&)> f)
{
    transform_positions(u, f);
    recalculate_normals(u);
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform_positions(U& u, std::function<typename U::traits_type::point_type(const typename U::traits_type::point_type&)> f)
{
    u.reset_all();
    for (auto& face : u.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            if (vertex.marked())
            {
                continue;
            }

            auto position = f(vertex.attributes().position());

            for (auto& h : vertex.halfedges())
            {
                auto& v = h.vertex();
                v.attributes().set_position(position);
                v.set_marked();
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::recalculate_normals(U& u)
{
    u.reset_all();
    for (auto& face : u.faces())
    {
        face.set_attributes({geometry::normal(face)});

        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            if (vertex.marked())
            {
                continue;
            }

            auto normal = surface_normal(vertex);

            for (auto& h : vertex.halfedges())
            {
                if (h.surface_id() == halfedge.surface_id())
                {
                    auto& v = h.vertex();
                    v.attributes().set_normal(normal);
                    v.set_marked();
                }
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::transform_border_fragment_position(M& mesh, id_type idHalfedge, const typename M::matrix_type& matrix, bool bReset)
{
    if (bReset)
    {
        mesh.reset_all();
    }

    auto& face = mesh.halfedge(idHalfedge).face();
    if (face.marked())
    {
        return;
    }

    face.set_marked();

    for (auto& halfedge : face.halfedges())
    {
        if (!halfedge.border())
        {
            transform_border_fragment_position(mesh, halfedge.partner_id(), matrix, false);
        }

        halfedge.vertex_attributes().position() *= matrix;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_spherical_normals(M& mesh, const typename M::point_type& center)
{
    mesh.reset_all();

    for (auto& vertex : mesh.vertices())
    {
        if (vertex.marked())
        {
            continue;
        }

        auto normal = normalize(vertex.attributes().position() - center);
        for (auto halfedge : vertex.halfedges())
        {
            halfedge.vertex_attributes().set_normal(normal);
            halfedge.vertex().set_marked();
        }
    }

    for (auto& face : mesh.faces())
    {
        face.attributes().set_normal(normalize(centroid(face) - center));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
typename M::vertex_attributes_type::texcoord_type quetzal::model::calculate_spherical_texcoord(const typename M::vector_type& normal)
{
    using T = M::value_type;

    auto u = T(0.5);
    if (!math::float_eq0(normal.x()) || !math::float_eq0(normal.y()))
    {
        u = math::clamp01((atan2(-normal.y(), -normal.x()) / math::Pi<T> + T(1.0)) * T(0.5));
    }

    auto v = math::clamp01(asin(-normal.z()) / math::Pi<T> + T(0.5));

    return {u, v};
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_spherical_texcoords(M& mesh)
{
    using T = M::value_type;

    mesh.reset_all();

    for (auto& vertex : mesh.vertices())
    {
        if (vertex.marked())
        {
            continue;
        }

        auto texcoord = calculate_spherical_texcoord<M>(vertex.attributes().normal());
        for (auto halfedge : vertex.halfedges())
        {
            halfedge.vertex_attributes().set_texcoord(texcoord);
            halfedge.vertex().set_marked();
        }

        // Handle the crossover case here ...
    }

    // Check for texcoord space edge crossing, needs refinement/generalization ...
    for (auto& face : mesh.faces())
    {
        fix_spherical_texcoords(face);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename F>
void quetzal::model::fix_spherical_texcoords(F& face)
{
    using T = F::mesh_type::value_type;

    bool bLow = false;
    bool bHigh = false;
    for (auto& halfedge : face.halfedges())
    {
        const auto& texcoord = halfedge.vertex_attributes().texcoord();
        if (texcoord.x() < T(0.25))
        {
            bLow = true;
        }
        if (texcoord.x() > T(0.75))
        {
            bHigh = true;
        }
    }

    if (bLow && bHigh)
    {
        for (auto& halfedge : face.halfedges())
        {                
            auto& texcoord = halfedge.vertex_attributes().texcoord();
            if (texcoord.x() < T(0.25))
            {
                texcoord.x() += T(1.0);
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::apply_triangular_face_texcoords(M& mesh)
{
    for (auto& face : mesh.faces())
    {
        assert(face.halfedge_count() == 3);

        face.halfedge().vertex_attributes().set_texcoord({M::val(0), M::val(1)});
        face.halfedge().next().vertex_attributes().set_texcoord({M::val(1), M::val(1)});
        face.halfedge().prev().vertex_attributes().set_texcoord({M::val(0.5), M::val(0)});
    }

    return;
}

#endif // QUETZAL_MODEL_MESH_ATTRIBUTES_HPP
