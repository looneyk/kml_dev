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
    void transform(U& u, const typename U::traits_type::matrix_type& matrix_position, const typename U::traits_type::matrix_type& matrix_direction);

    // Works with Mesh, Submesh, Surface
    template<typename U>
    void transform_positions(U& u, const typename U::traits_type::matrix_type& matrix);

    // Works with Mesh, Submesh
    // Function f calculates new vertex attributes
    // No optimization, no surface or seam cognizance, not terribly useful as is, just used in one set of test cases which themselves can be deprecated
    // deprecated ...
    template<typename U>
    void transform_vertices(U& u, std::function<typename U::traits_type::vertex_attributes_type(typename U::traits_type::vertex_attributes_type)> f);

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
    void transform_border_fragment_position(M& mesh, id_type halfedge_id, const typename M::matrix_type& matrix, bool reset = true);

    // Calculates normals for a submesh representing a sphere once all geometry has been created
    // face normals are not calculated here, they can be calculated on construcction ...
    // center? ...
    template<typename Traits>
    void calculate_spherical_surface_vertex_normals(brep::Submesh<Traits>& submesh);

    // Change to U& u ...
    template<typename M>
    void calculate_spherical_normals(M& mesh, const typename M::point_type& center = {});

    // Calculate texcoord from the spherical coordinates of the vertex normal vector
    // why not using the vertex position? ...
    template<typename M>
    typename M::vertex_attributes_type::texcoord_type calculate_spherical_texcoord(const typename M::vector_type& normal);

    // Calculate texcoords to wrap a sphere where the vertex normals have already been calculated
    // Change to U& u ...
    template<typename M>
    void calculate_spherical_texcoords(M& mesh);

    // Fix edge texcoords that span the 0/2Pi boundary by shifting the lower one by +1 ...
    // needs refinement/generalization ...
    template<typename F>
    void fix_spherical_texcoords(F& face);

    // this does arbitrary orientation, take a tangent vector to align textures? then fit triangles to unit square? ...
    // Change to U& u ...
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
void quetzal::model::transform(U& u, const typename U::traits_type::matrix_type& matrix_position, const typename U::traits_type::matrix_type& matrix_direction)
{
    if constexpr (std::same_as<U, brep::Mesh<typename U::traits_type>>) // separate functions, better identification of Mesh, less overhead if doing for entire Mesh, should be deprecated, might be useful for model manipulation for printing (but transform otherwise for rendering) ...
    {
        for (auto& vertex : u.vertices())
        {
            vertex.attributes().transform(matrix_position, matrix_direction);
        }

        for (auto& face : u.faces())
        {
            face.attributes().transform(matrix_position, matrix_direction);
        }
    }
    else
    {
// this recalculates normals that have already been calculated, but results should be the same, another consequence of menger_sponge internal surface? ...
// this is the original functionality, but had problems with surface specificity (below), that's the real fix ...
        typename U::traits_type::point_type position_orig;
        typename U::traits_type::point_type position;

        for_each_vertex_position(u,
            [&](const auto& vertex) -> bool
            {
                position_orig = vertex.attributes().position();
                position = position_orig * matrix_position;
                return true;
            },
            [&](auto& vertex) -> void
            {
                assert(float_eq(vertex.attributes().position(), position_orig));
                vertex.attributes().set_position(position);
                vertex.attributes().set_normal(normalize(vertex.attributes().normal() * matrix_direction));
                return;
            },
            [&](auto& face) -> void
            {
                face.attributes().transform(matrix_position, matrix_direction);
                return;
            }
        );
/*
        typename U::traits_type::point_type position_orig;
        typename U::traits_type::point_type position;
        typename U::traits_type::vector_type normal_orig;
        typename U::traits_type::vector_type normal;
        id_type surface_id = nullid;

        for_each_vertex(u,
            [&](const auto& vertex) -> bool
            {
                if (vertex.marked())
                {
                    return true;
                }

                if (!vertex.visited())
                {
                    position_orig = vertex.attributes().position();
                    position = position_orig * matrix_position;
                }

                normal_orig = vertex.attributes().normal();
                normal = normalize(normal_orig * matrix_direction);
                surface_id = vertex.halfedge().surface_id();
                return true;
            },
            [&](auto& vertex) -> void
            {
                if (vertex.marked())
                {
                    return;
                }

                if (!vertex.visited())
                {
                    assert(float_eq(vertex.attributes().position(), position_orig));
                    vertex.attributes().set_position(position);
                    vertex.set_visited();
                }

                if (vertex.halfedge().surface_id() == surface_id)
                {
// good assertion, but menger_sponge internal is not a planar surface, need to fix that ...
//                    assert(float_eq(vertex.attributes().normal(), normal_orig));
                    vertex.attributes().set_normal(normal);
                    vertex.set_marked();
                }
                return;
            },
            [&](auto& face) -> void
            {
                face.attributes().transform(matrix_position, matrix_direction);
                return;
            }
        );
*/
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform_positions(U& u, const typename U::traits_type::matrix_type& matrix)
{
    if constexpr (std::same_as<U, brep::Mesh<typename U::traits_type>>) // separate functions, or just get rid of this case, doesn't handle coincident vertex positions (just stop doing things like this at the mesh level) ...
    {
        for (auto& vertex : u.vertices())
        {
            vertex.attributes().transform_position(matrix);
        }
    }
    else
    {
        typename U::traits_type::point_type position_orig;
        typename U::traits_type::point_type position;

        for_each_vertex_position(u,
            [&](const auto& vertex) -> bool
            {
                position_orig = vertex.attributes().position();
                position = position_orig * matrix;
                return true;
            },
            [&](auto& vertex)
            {
                assert(float_eq(vertex.attributes().position(), position_orig));
                vertex.attributes().set_position(position);
                return;
            }
        );
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::transform_vertices(U& u, std::function<typename U::traits_type::vertex_attributes_type(typename U::traits_type::vertex_attributes_type)> f)
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
    typename U::traits_type::point_type position_orig;
    typename U::traits_type::point_type position;

    for_each_vertex_position(u,
        [&](const auto& vertex) -> bool
        {
            position_orig = vertex.attributes().position();
            position = f(position_orig);
            return true;
        },
        [&](auto& vertex)
        {
            assert(float_eq(vertex.attributes().position(), position_orig));
            vertex.attributes().set_position(position);
            return;
        }
    );

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::recalculate_normals(U& u)
{
    typename U::traits_type::vector_type normal;

    for_each_surface_vertex(u,
        [&](const auto& vertex) -> bool
        {
            normal = surface_normal(vertex);
            return true;
        },
        [&](auto& vertex) -> void
        {
            vertex.attributes().set_normal(normal);
            return;
        },
        [](auto& face) -> void
        {
            face.set_attributes({geometry::normal(face)});
            return;
        }
    );

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::transform_border_fragment_position(M& mesh, id_type halfedge_id, const typename M::matrix_type& matrix, bool reset)
{
    if (reset)
    {
        mesh.reset_all();
    }

    auto& face = mesh.halfedge(halfedge_id).face();
    if (face.visited())
    {
        return;
    }

    face.set_visited();

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
template<typename Traits>
void quetzal::model::calculate_spherical_surface_vertex_normals(brep::Submesh<Traits>& submesh)
{
    typename Traits::vector_type normal;

    for_each_surface_vertex(submesh,
        [&](const auto& vertex) -> bool
        {
            normal = normalize(vertex.attributes().position().vector());
            return true;
        },
        [&](auto& vertex) -> void
        {
            vertex.attributes().set_normal(normal);
            return;
        }
    );

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_spherical_normals(M& mesh, const typename M::point_type& center)
{
    typename M::traits_type::vector_type normal;

    for_each_surface_vertex(mesh,
        [&](const auto& vertex) -> bool
        {
            normal = normalize(vertex.attributes().position() - center);
            return true;
        },
        [&](auto& vertex) -> void
        {
            vertex.attributes().set_normal(normal);
            return;
        },
        [&](auto& face) -> void
        {
            face.attributes().set_normal(normalize(centroid(face) - center));
            return;
        }
    );

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
    typename M::vertex_attributes_type::texcoord_type texcoord;

    for_each_surface_vertex(mesh,
        [&](const auto& vertex) -> bool
        {
            texcoord = calculate_spherical_texcoord<M>(vertex.attributes().normal());
            return true;
        },
        [&](auto& vertex) -> void
        {
            vertex.attributes().set_texcoord(texcoord);
            return;
        },
        [](auto& face) -> void
        {
            face.set_attributes({geometry::normal(face)});
            return;
        }
    );

    // Handle the crossover case here (geodesic sphere) ...
    // Check for texcoord space edge crossing, needs refinement/generalization ...
    // This must be done after all texcoords are calculated (can't use for_each_surface_vertex with face) ...
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

    bool low = false;
    bool high = false;
    for (auto& halfedge : face.halfedges())
    {
        const auto& texcoord = halfedge.vertex_attributes().texcoord();
        if (texcoord.x() < T(0.25))
        {
            low = true;
        }
        if (texcoord.x() > T(0.75))
        {
            high = true;
        }
    }

    if (low && high)
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
