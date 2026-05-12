#if !defined(QUETZAL_MODEL_HELIX_ANTICYLINDER_HPP)
#define QUETZAL_MODEL_HELIX_ANTICYLINDER_HPP
//------------------------------------------------------------------------------
// model
// helix_anticylinder.hpp
//------------------------------------------------------------------------------

#include "primitives_util.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/math_util.hpp"
#include <functional>
#include <string>
#include <cmath>
#include <cassert>

#include <iostream>

namespace quetzal::model
{

    // what about half pair?
    // this helix is not the same as the helix_cone helix
    // with the correct parameters, helix_cone could create nAzimuth helixes

    // dz = (zUpper - zLower) / (revs + 1)
    // dz is the vertical distance between bands, not the closest distance between its bottom and top
    // Create an anticylinder where a band rises by dz over the course of each rev
    // nAzimuth is number of triangle pairs per rev
    // no extent parameter, model-specific
    template<typename M>
    id_type create_helix_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, data_value_type<M> rLower, data_value_type<M> rUpper, data_value_type<M> zLower, data_value_type<M> zUpper, data_value_type<M> revsLower, data_value_type<M> revsUpper, bool bSmooth = false);

    template<typename M>
    id_type create_helix_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, std::function<data_value_type<M>(const data_value_type<M>&)> fr, std::function<data_value_type<M>(const data_value_type<M>&)> fdr, data_value_type<M> zLower, data_value_type<M> zUpper, data_value_type<M> revsLower, data_value_type<M> revsUpper, bool bSmooth = false);

} // namespace quetzal::model

//--------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_helix_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, data_value_type<M> rLower, data_value_type<M> rUpper, data_value_type<M> zLower, data_value_type<M> zUpper, data_value_type<M> revsLower, data_value_type<M> revsUpper, bool bSmooth)
{
    // does not provide fdr ...
    return create_helix_anticylinder(mesh, name, nAzimuth, [=](const data_value_type<M>& t) -> data_value_type<M> { return math::lerp(rLower, rUpper, t); }, zLower, zUpper, revsLower, revsUpper, bSmooth);
}

//--------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_helix_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, std::function<data_value_type<M>(const data_value_type<M>&)> fr, std::function<data_value_type<M>(const data_value_type<M>&)> fdr, data_value_type<M> zLower, data_value_type<M> zUpper, data_value_type<M> revsLower, data_value_type<M> revsUpper, bool bSmooth)
{
    using T = M::value_type;

// only for revs > 1 ...
// def for closure, but here too? ...

    T dz = zUpper - zLower;
    T revs = revsUpper - revsLower;
    size_t nPanels = static_cast<size_t>(round(abs(nAzimuth * revs)));
    size_t n = nPanels + nAzimuth;
//std::cout << "n " << n << std::endl;

    id_type idSurface0 = mesh.surface_store_count();

    size_t nh = mesh.halfedge_store_count();
    size_t ns = idSurface0;

    id_type idSubmesh = mesh.create_submesh(name);

    if (bSmooth)
    {
        mesh.create_surface(idSubmesh, "body");
    }
    else
    {
        size_t nSurfaces = 2 * nPanels;
        for (size_t i = 0; i < nSurfaces; ++i)
        {
            mesh.create_surface(idSubmesh, std::format("{}{}", "body", i));
        }
    }

    vertices_attributes_type<M> avs0(nAzimuth + 1); // store bottom most vertices before any connection is possible
    vertices_attributes_type<M> avs1(nAzimuth + 1); // store top most vertex attributes to use when closing top

    for (size_t i = 0; i <= n; ++i)
    {
        T t = T(i) / T(n);
        T r = fr(t);
// This should more gracefully handle the r = 0 case ...
assert(r > 0);
        T z = math::lerp(zLower, zUpper, t);
        T rev = math::lerp(revsLower, revsUpper + T(1), t) * (T(1) - T(1) / T(2 * nAzimuth)); // scale is an approximation ...
        T phi = rev * math::PiTwo<T>;
//std::cout << "t " << t << " r " << r << " z " << z << " rev " << rev << std::endl;

        // assuming smooth here ...
        typename M::point_type position = {r * cos(phi), r * sin(phi), z};
        typename M::vector_type direction = {position.x(), position.y(), r * -fdr(t)};
        T u = T(i % nAzimuth) / nAzimuth;
        T v = T(1) - t;
        typename M::vertex_attributes_type av = {position, normalize(direction), {u, v}};

        if (i >= nPanels)
        {
            // Store upper vertex reference attributes
            avs1[i - nPanels] = av;
        }

        if (i <= nAzimuth)
        {
            // Store lower vertex reference attributes
            avs0[i] = av;
            continue;
        }

        if (i == nAzimuth + 1)
        {
            // First face pair, no external connection

            id_type idSurface = bSmooth ? idSurface0 : ns++;
            id_type idFace = mesh.create_face(idSurface, nh);
            mesh.create_halfedge_vertex(nh + 5, nh + 1, nh + 2, idFace, avs0[0]);
            mesh.create_halfedge_vertex(nullid, nh + 2, nh + 0, idFace, av);
            mesh.create_halfedge_vertex(nullid, nh + 0, nh + 1, idFace, avs0[nAzimuth]);

            auto normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }

            idSurface = bSmooth ? idSurface0 : ns++;
            idFace = mesh.create_face(idSurface, nh + 3);
            mesh.create_halfedge_vertex(nullid, nh + 4, nh + 5, idFace, avs0[0]);
            mesh.create_halfedge_vertex(nullid, nh + 5, nh + 3, idFace, avs0[1]);
            mesh.create_halfedge_vertex(nh + 0, nh + 3, nh + 4, idFace, av);

            normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }
        }
        else if (i <= 2 * nAzimuth)
        {
            // First layer, connection on left only

            id_type idSurface = bSmooth ? idSurface0 : ns++;
            id_type idFace = mesh.create_face(idSurface, nh);
            mesh.create_halfedge_vertex(nh + 5, nh + 1, nh + 2, idFace, mesh.halfedge(nh - 2).vertex_attributes());
            mesh.create_halfedge_vertex(nullid, nh + 2, nh + 0, idFace, av);
            mesh.create_halfedge_vertex(nh - 2, nh + 0, nh + 1, idFace, mesh.halfedge(nh - 1).vertex_attributes());
            mesh.halfedge(nh - 2).set_partner_id(nh + 2);

            auto normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }

            idSurface = bSmooth ? idSurface0 : ns++;
            idFace = mesh.create_face(idSurface, nh + 3);
            mesh.create_halfedge_vertex(nullid, nh + 4, nh + 5, idFace, mesh.halfedge(nh - 2).vertex_attributes());
            mesh.create_halfedge_vertex(nullid, nh + 5, nh + 3, idFace, avs0[i - nAzimuth]);
            mesh.create_halfedge_vertex(nh + 0, nh + 3, nh + 4, idFace, av);

            normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }
        }
        else
        {
            // Subsequent layers, connection on both left and bottom

            id_type idSurface = bSmooth ? idSurface0 : ns++;
            id_type idFace = mesh.create_face(idSurface, nh);
            mesh.create_halfedge_vertex(nh + 5, nh + 1, nh + 2, idFace, mesh.halfedge(nh - 2).vertex_attributes());
            mesh.create_halfedge_vertex(nullid, nh + 2, nh + 0, idFace, av);
            mesh.create_halfedge_vertex(nh - 2, nh + 0, nh + 1, idFace, mesh.halfedge(nh - 1).vertex_attributes());
            mesh.halfedge(nh - 2).set_partner_id(nh + 2);

            auto normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }

            idSurface = bSmooth ? idSurface0 : ns++;
            idFace = mesh.create_face(idSurface, nh + 3);
            mesh.create_halfedge_vertex(nh - 6 * nAzimuth + 1, nh + 4, nh + 5, idFace, mesh.halfedge(nh - 2).vertex_attributes());
            mesh.create_halfedge_vertex(nullid, nh + 5, nh + 3, idFace, mesh.halfedge(nh - 6 * nAzimuth + 1).vertex_attributes());
            mesh.create_halfedge_vertex(nh + 0, nh + 3, nh + 4, idFace, av);
            mesh.halfedge(nh - 6 * nAzimuth + 1).set_partner_id(nh + 3);

            normal = geometry::normal(mesh.face(idFace));
            mesh.face(idFace).attributes().set_normal(normal);
            if (!bSmooth)
            {
                mesh.surface(idSurface).attributes().set_normal(normal);
            }
        }

        nh += 6;
    }

    // Correct texcoords at wraparound

    for (size_t i = 6 * nAzimuth - 2; i < 6 * nPanels; i += 6 * nAzimuth)
    {
        assert(mesh.halfedge(i).vertex_attributes().texcoord().x() == T(0));
        assert(mesh.halfedge(i + 1).vertex_attributes().texcoord().x() == T(0));
        assert(mesh.halfedge(i - 3).vertex_attributes().texcoord().x() == T(0));

        mesh.halfedge(i).vertex_attributes().texcoord().set_x(T(1));
        mesh.halfedge(i + 1).vertex_attributes().texcoord().set_x(T(1));
        mesh.halfedge(i - 3).vertex_attributes().texcoord().set_x(T(1));
    }

    if (revs < T(1))
    {
        // can't do below ...
        // first open bottom closure, then close off with top
    }

    // Close bottom

    size_t nhBody0 = nh;
    typename M::point_type center = {T(0), T(0), zLower + dz / (T(2) * revs)};

    id_type idSurface = mesh.create_surface(idSubmesh, "end0_0");
    id_type idFace = mesh.create_face(idSurface, nh);
    mesh.create_halfedge_vertex(nh + 5, nh + 1, nh + 2, idFace, {center, {}, {T(0.5), T(0)}});
    mesh.create_halfedge_vertex(2, nh + 2, nh + 0, idFace, {avs0[0].position(), {}, {T(0), T(1)}});
    mesh.create_halfedge_vertex(nh + 3 * nAzimuth, nh + 0, nh + 1, idFace, {avs0[nAzimuth].position(), {}, {T(1), T(1)}});
    mesh.halfedge(2).set_partner_id(nh + 1);

    set_facet_normal(mesh.face(idFace));

    nh += 3;

    auto normal = -M::vector_type::z_direction();

    idSurface = mesh.create_surface(idSubmesh, "end0_1", {normal});
    for (size_t i = 0; i < nAzimuth; ++i)
    {
        idFace = mesh.create_face(idSurface, nh, {normal});
        mesh.create_halfedge_vertex((i == nAzimuth - 1 ? nhBody0 + 2 : nh + 5), nh + 1, nh + 2, idFace, {center, normal, {}});
        mesh.create_halfedge_vertex(6 * i + 3, nh + 2, nh + 0, idFace, {avs0[i + 1].position(), normal, {}});
        mesh.create_halfedge_vertex((i == 0 ? nhBody0 : nh - 3), nh + 0, nh + 1, idFace, {avs0[i].position(), normal, {}});
        mesh.halfedge(6 * i + 3).set_partner_id(nh + 1);

        nh += 3;
    }

    calculate_surface_texcoords(mesh, idSurface);

    // Close top

    center = {T(0), T(0), zUpper - dz / (T(2) * revs)};

    idSurface = mesh.create_surface(idSubmesh, "end1_0");
    idFace = mesh.create_face(idSurface, nh);
    mesh.create_halfedge_vertex(nh + 3 * nAzimuth + 2, nh + 1, nh + 2, idFace, {center, {}, {T(0.5), T(0)}});
    mesh.create_halfedge_vertex(nhBody0 - 2, nh + 2, nh + 0, idFace, {avs1[nAzimuth].position(), {}, {T(0), T(1)}});
    mesh.create_halfedge_vertex(nh + 3, nh + 0, nh + 1, idFace, {avs1[0].position(), {}, {T(1), T(1)}});
    mesh.halfedge(nhBody0 - 2).set_partner_id(nh + 1);

    set_facet_normal(mesh.face(idFace));

    nh += 3;

    normal = M::vector_type::z_direction();

    idSurface = mesh.create_surface(idSubmesh, "end1_1", {normal});
    for (size_t i = 0; i < nAzimuth; ++i)
    {
        idFace = mesh.create_face(idSurface, nh, {normal});
        mesh.create_halfedge_vertex(nh - 1, nh + 1, nh + 2, idFace, {center, normal, {}});
        mesh.create_halfedge_vertex(nhBody0 - 6 * (nAzimuth - i) + 1, nh + 2, nh + 0, idFace, {avs1[i].position(), normal, {}});
        mesh.create_halfedge_vertex((i == nAzimuth - 1 ? nh - 3 * nAzimuth : nh + 3), nh + 0, nh + 1, idFace, {avs1[i + 1].position(), normal, {}});
        mesh.halfedge(nhBody0 - 6 * (nAzimuth - i) + 1).set_partner_id(nh + 1);

        nh += 3;
    }

    calculate_surface_texcoords(mesh, idSurface);
    return idSubmesh;
}

#endif // QUETZAL_MODEL_HELIX_ANTICYLINDER_HPP
