#if !defined(QUETZAL_MODEL_VERTICES_ATTRIBUTES_HPP)
#define QUETZAL_MODEL_VERTICES_ATTRIBUTES_HPP
//------------------------------------------------------------------------------
// model
// vertices_attributes.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/PositionAccessor.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <array>
#include <functional>
#include <optional>
#include <ranges>
#include <vector>
#include <cassert>

// remove normal placeholders? ...
// default ts parameters to x [0, 1]? see if y can be defaulted ... 

// nAzimuth here refers to the number of edges and nAzimuth + 1 attributes ard produced in most cases here so that we will have the starting and ending attributes for each face generated from this without the need to deal with the seam edge case 
// The first and last attributes will have the same position and possibly normal, but the texcoord will be different for the seam
// texcoords are calculated linearly distributed along the curve, and not for a polygonal surface section
// texcoords for that case or for the single face/surface per edge case can easily be calulated after

namespace quetzal::model
{

    template<typename M>
    using data_value_type = M::value_type;

    template<typename M>
    using azimuth_interval_type = math::Interval<data_value_type<M>>;

    // The surface vertex normal at a cusp is defined as the vector perpendicular to the edge and directed outward
    // This is the surface normal of a perfect cone in that direction, what the face normal would converge to as nAzimuth approaches infinity

    // typically used for prototype normal vector at the turn 0 position
    template<typename M>
    using normal_vector_type = M::vector_type;

    template<typename M>
    using texture_span_type = std::array<data_value_type<M>, 3>;

    template<typename M>
    using vertex_normal_function_type = std::function<normal_vector_type<M>(data_value_type<M>)>;

    template<typename M>
    class vertices_attributes_type : public std::vector<typename M::vertex_attributes_type>
    {
    public:
        using std::vector<typename M::vertex_attributes_type>::vector;
    };

    template<typename M>
    using vertices_attributes_sections_type = std::vector<vertices_attributes_type<M>>;

    // Precalculate vertex values for a circle or circular section parallel to the xy-plane
    // Vertex normals are set to a placeholder value so this can be used when vertex normals cannot be precalculated, automatic surface normal calculation will be used
    // Calculates texture coordinates appropriate for a single encompassing surface where ts defines the full texture range
    // Produces nAzimuth + 1 vertex attributes
    template<typename M>
    vertices_attributes_type<M> vertices_attributes(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_type<M> vertices_attributes(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts, vertex_normal_function_type<M> vertex_normal);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces polygon edge_count + 1 vertex attributes
    template<typename M>
    vertices_attributes_type<M> vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces polygon edge_count + 1 vertex attributes
    template<typename M>
    vertices_attributes_type<M> vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces vertices_attributes per polygon section
    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces vertices_attributes per polygon section
    // This is used in only one place, the rest transform the input polygon, but this is just a bit more efficient
    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts);

    // Precalculate vertex values for a circle or circular section parallel to the xy-plane
    // Calculates texture coordinates appropriate for a surface for each aimuth edge where ts defines the texture range for each surface
    // Produces nAzimuth + 1 vertex attributes
    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_edges(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces vertices_attributes per polygon edge
    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Calculates normals and texture coordinates appropriate for a section of a single encompassing surface
    // Produces vertices_attributes per polygon edge
    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts);

    // Precalculate vertex values for a layer
    // Produces polygon edge_count + 1 points
    template<typename M>
    vertices_attributes_type<M> vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_sections_type<M> vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Produces a single smooth polygon section
    template<typename M>
    vertices_attributes_type<M> vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, std::optional<data_value_type<M>> z, texture_span_type<M> ts);

    // Precalculate vertex values for a layer parallel to the xy-plane
    // Produces a single smooth polygon section
    template<typename M>
    vertices_attributes_type<M> vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts);

    // Having trouble with calculated normal ...
    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes(const vertices_attributes_type<M>& avs_base, data_value_type<M> z, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes(size_t nAzimuth, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts, vertex_normal_function_type<M> vertex_normal);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces vertex attributes for the whole azimuth interval
    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes(size_t nAzimuth, data_value_type<M> z, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces polygon edge_count vertex attributes
    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces separate vertex attributes for each azimuth section
    template<typename M>
    vertices_attributes_sections_type<M> cusp_vertices_attributes_edges(size_t nAzimuth, data_value_type<M> z, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces separate vertex attributes for each polygon edge
    template<typename M>
    vertices_attributes_sections_type<M> cusp_vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces separate vertex attributes for each polygon section
    template<typename M>
    vertices_attributes_sections_type<M> cusp_vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces polygon edge_count vertex attributes
    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_sections_type<M> cusp_vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    template<typename M>
    vertices_attributes_sections_type<M> cusp_vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts);

    // Precalculate vertex values for a base or apex cusp
    // Normals are not calculated, must be set by caller
    // Produces section vertex_count vertex attributes
    template<typename M>
    vertices_attributes_type<M> cusp_vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, data_value_type<M> z, texture_span_type<M> ts);

    // Produces one vertex_attributes_type per border vertex
    // These are returned in the order needed for a face closing off the open border
    template<typename M>
    vertices_attributes_type<M> border_vertices_attributes(const M& mesh, id_type idHalfedgeBorder);

    template<typename Traits>
    vertices_attributes_type<Traits> vertices_attributes(const brep::Face<Traits>& face);

    template<typename Traits>
    vertices_attributes_type<Traits> vertices_attributes(const brep::Surface<Traits>& surface);

    // note that Interval shift is relative, but this is absolute ...

    template<typename M>
    vertices_attributes_type<M> shift(vertices_attributes_type<M>& avs, data_value_type<M> z, std::optional<texture_span_type<M>> ts = std::nullopt);

    template<typename M>
    vertices_attributes_type<M> shift(vertices_attributes_type<M>& avs, data_value_type<M> radius, data_value_type<M> z, std::optional<texture_span_type<M>> ts = std::nullopt);

    template<typename M>
    std::ostream& operator<<(std::ostream& os, const vertices_attributes_type<M>& avs);

    template<typename M>
    std::ostream& operator<<(std::ostream& os, const vertices_attributes_sections_type<M>& avss);

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct geometry::PositionAccessor<vertices_attributes_type<Traits>>
    {
        using point_type = model::vertices_attributes_type<Traits>::value_type::point_type;
        using vector_type = point_type::vector_type;
        using value_type = point_type::value_type;
        static constexpr size_t dimension = point_type::dimension;

        static size_t position_count(const model::vertices_attributes_type<Traits>& t)
        {
            return t.size();
        }

        static point_type position(const model::vertices_attributes_type<Traits>& t, size_t n)
        {
            if (n >= t.size())
            {
                throw std::out_of_range("PositionAccessor::range_check");
            }

            return t[n].position();
        }
    
        static bool constant_time_acccess()
        {
            return true;
        }
    };

namespace internal
{

    //--------------------------------------------------------------------------
    // Returns start position and length of a polygon section
    template<typename M>
    std::array<size_t, 2> section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t n)
    {
        assert(polygon.sections());
        const auto& sections = polygon.sections().value();
        size_t section_count = sections.size();

        size_t edge_count = polygon.edge_count();
        if (section_count == 0)
        {
            assert(n == 0);
            return {0, edge_count};
        }

        assert(n < section_count);
        size_t section_edge_count = (n < section_count - 1 ? sections[n + 1] : edge_count) - sections[n];
        return {sections[n], section_edge_count};
    }

    //--------------------------------------------------------------------------
    // Returns vertex position treating polygon.vertices() as a circular buffer
    template<typename M>
    typename geometry::Polygon<typename M::vector_traits>::point_type polygon_vertex(const geometry::Polygon<typename M::vector_traits>& polygon, size_t n)
    {
        return polygon.vertices()[n % polygon.vertex_count()];
    }

} // namespace internal

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts)
{
    return vertices_attributes<M>(nAzimuth, radius, z, azimuth_interval, ts, [](data_value_type<M>) -> normal_vector_type<M> { return M::vector_type::z_direction(); });
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts, vertex_normal_function_type<M> vertex_normal)
{
    using T = data_value_type<M>;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    auto rotate_z = [](const normal_vector_type<M>& v, T cos_phi, T sin_phi) -> normal_vector_type<M>
    {
        return normalize(denoise0(normal_vector_type<M>{v.x() * cos_phi - v.y() * sin_phi, v.x() * sin_phi + v.y() * cos_phi, v.z()}));
    };

    const auto& [tx0, tx1, ty] = ts;

    vertices_attributes_type<M> avs(nAzimuth + 1);

    // optimizations can be made in cases other than strict unit based on symmetry ...

    if (azimuth_interval.strict_unit())
    {
        size_t nAzimuth2 = nAzimuth / 2;

        if (math::even(nAzimuth)) // Use first quadrant vertex attributes for reference
        {
            size_t nAzimuth4 = nAzimuth2 / 2;

            if (math::even(nAzimuth2)) // Use first octant vertex attributes for reference
            {
                size_t nAzimuth8 = nAzimuth4 / 2;

                size_t i0 = 0;
                size_t i1 = nAzimuth8;
                for (size_t i = i0; i <= i1; ++i)
                {
                    T t = T(i) / T(nAzimuth);
                    auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));

                    T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                    T cos_phi = math::denoise011(cos(phi));
                    T sin_phi = math::denoise011(sin(phi));

                    avs[i].set_position({radius * cos_phi, radius * sin_phi, z});
                    avs[i].set_normal(vertex_normal(phi));
                    avs[i].set_texcoord(texcoord);

                    size_t j = nAzimuth4 - i;
                    if (j != i)
                    {
                        T tj = T(nAzimuth4 - i) / T(nAzimuth);
                        texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, tj), ty}));
                        avs[j].set_position({avs[i].position().y(), avs[i].position().x(), avs[i].position().z()});
                        avs[j].set_normal({avs[i].normal().y(), avs[i].normal().x(), avs[i].normal().z()});
                        avs[j].set_texcoord(texcoord);
                    }
                }
            }
            else
            {
                size_t i0 = 0;
                size_t i1 = nAzimuth4;
                for (size_t i = i0; i <= i1; ++i)
                {
                    T t = T(i) / T(nAzimuth);
                    auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));

                    T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                    T cos_phi = math::denoise011(cos(phi));
                    T sin_phi = math::denoise011(sin(phi));

                    avs[i].set_position({radius * cos_phi, radius * sin_phi, z});
                    avs[i].set_normal(vertex_normal(phi));
                    avs[i].set_texcoord(texcoord);
                }
            }

            size_t i0 = nAzimuth4 + 1;
            size_t i1 = nAzimuth2;
            for (size_t i = i0; i <= i1; ++i)
            {
                T t = T(i) / T(nAzimuth);
                auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));

                size_t j = nAzimuth2 - i;
                avs[i].set_position({-avs[j].position().x(), avs[j].position().y(), avs[j].position().z()});
                avs[i].set_normal({-avs[j].normal().x(), avs[j].normal().y(), avs[j].normal().z()});
                avs[i].set_texcoord(texcoord);
            }

            i0 = nAzimuth2 + 1;
            i1 = nAzimuth;
            for (size_t i = i0; i <= i1; ++i)
            {
                T t = T(i) / T(nAzimuth);
                auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));

                size_t j = nAzimuth - i;
                avs[i].set_position({avs[j].position().x(), -avs[j].position().y(), avs[j].position().z()});
                avs[i].set_normal({avs[j].normal().x(), -avs[j].normal().y(), avs[j].normal().z()});
                avs[i].set_texcoord(texcoord);
            }
        }
        else // Use first and second quadrant vertex attributes for reference
        {
            for (size_t i = 0; i <= nAzimuth2; ++i)
            {
                T t = T(i) / T(nAzimuth);
                auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));
  
                T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                T cos_phi = math::denoise011(cos(phi));
                T sin_phi = math::denoise011(sin(phi));

                avs[i].set_position({radius * cos_phi, radius * sin_phi, z});
                avs[i].set_normal(vertex_normal(phi));
                avs[i].set_texcoord(texcoord);

                size_t j = nAzimuth - i;
                T tj = T(j) / T(nAzimuth);
                texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, tj), ty}));
                avs[j].set_position({avs[i].position().x(), -avs[i].position().y(), avs[i].position().z()});
                avs[j].set_normal({avs[i].normal().x(), -avs[i].normal().y(), avs[i].normal().z()});
                avs[j].set_texcoord(texcoord);
            }
        }
    }
    else
    {
        for (size_t i = 0; i <= nAzimuth; ++i)
        {
            T t = T(i) / T(nAzimuth);
            auto texcoord = denoise01(texcoord_type({math::lerp(tx0, tx1, t), ty}));

            T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
            T cos_phi = math::denoise011(cos(phi));
            T sin_phi = math::denoise011(sin(phi));

            avs[i].set_position({radius * cos_phi, radius * sin_phi, z});
            avs[i].set_normal(vertex_normal(phi));
            avs[i].set_texcoord(texcoord);
        }

        // If azimuth is one turn, ensure that first and last values are the same except for tx
        if (azimuth_interval.unit())
        {
            avs[nAzimuth] = avs[0];
            avs[nAzimuth].texcoord().set_x(ts[1]);
        }
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t edge_count = polygon.edge_count();
    vertices_attributes_type<M> avs(edge_count + 1);

    for (size_t i = 0; i < edge_count; ++i)
    {
        T t = T(i) / edge_count;
        avs[i].set_position(polygon.vertex(i));
        if (z)
        {
            avs[i].position().set_z(z.value());
        }
        avs[i].set_normal(M::vector_type::z_direction()); // placeholder, surface vertex normal cannot be calculated at this point
        avs[i].set_texcoord({math::lerp(ts[0], ts[1], t), ts[2]});
    }

    avs[edge_count] = avs[0];
    avs[edge_count].texcoord().set_x(ts[1]);
    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t edge_count = polygon.edge_count();
    vertices_attributes_type<M> avs(edge_count + 1);

    T phi = math::PiTwo<T> * revs;
    T cos_phi = math::denoise011(cos(phi));
    T sin_phi = math::denoise011(sin(phi));

    for (size_t i = 0; i < edge_count; ++i)
    {
        T t = T(i) / edge_count;

        auto position = polygon.vertex(i);
        position = {scale * (position.x() * cos_phi - position.y() * sin_phi), scale * (position.x() * sin_phi + position.y() * cos_phi), z.value_or(position.z())};

        avs[i].set_position(position);
        avs[i].set_normal(M::vector_type::z_direction()); // placeholder, surface vertex normal cannot be calculated at this point
        avs[i].set_texcoord({math::lerp(ts[0], ts[1], t), ts[2]});
    }

    avs[edge_count] = avs[0];
    avs[edge_count].texcoord().set_x(ts[1]);
    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts)
{
    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();

    vertices_attributes_sections_type<M> avss(section_count);

    if (section_count == 1)
    {
        avss[0] = vertices_attributes<M>(polygon, z, ts);
    }
    else
    {
        for (size_t i = 0; i < section_count; ++i)
        {
            auto [edge0, edge_count] = internal::section<M>(polygon, i);
            avss[i] = vertices_attributes_section<M>(polygon, edge0, edge_count, z, ts);
        }
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts)
{
    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();

    vertices_attributes_sections_type<M> avss(section_count);

    if (section_count == 1)
    {
        avss[0] = vertices_attributes<M>(polygon, z, scale, revs, ts);
    }
    else
    {
        for (size_t i = 0; i < section_count; ++i)
        {
            auto [edge0, edge_count] = internal::section<M>(polygon, i);
            avss[i] = vertices_attributes_section<M>(polygon, edge0, edge_count, z, scale, revs, ts);
        }
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_edges(size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts)
{
    assert(nAzimuth > 0);

    using T = data_value_type<M>;

    size_t section_count = nAzimuth;
    vertices_attributes_sections_type<M> avss(section_count);

    auto polygon_interval = geometry::regular_polygon_section<typename M::vector_traits>(nAzimuth, radius, z, azimuth_interval);
    // use the above instead of calculations (symmetry) ...

    auto [tx0, tx1, ty] = ts;

    for (size_t i = 0; i < section_count; ++i)
    {
        vertices_attributes_type<M> avs(2); // section_edge_count is always 1
        typename M::vertex_attributes_type::point_type position;

        if (i == 0)
        {
            auto t = T(i) / section_count;
            T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
            position = {radius * math::denoise011(cos(phi)), radius * math::denoise011(sin(phi)), z};
//            avs[0].set_position(denoise0(position));
            avs[0].set_position(polygon_interval.vertices()[i]);
            avs[0].set_normal(M::vector_type::z_direction());
            avs[0].set_texcoord({tx0, ty});
        }
        else
        {
            avs[0] = avss[i - 1][1];
            avs[0].texcoord().set_x(tx0);
        }
        
        if (i == section_count - 1 && azimuth_interval.unit())
        {
            position = avss[0][0].position();
        }
        else
        {
//            auto t = T(i + 1) / section_count;
//            T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
//            position = {radius * math::denoise011(cos(phi)), radius * math::denoise011(sin(phi)), z};
            position = polygon_interval.vertices()[i + 1];
        }

        avs[1].set_position(position);
        avs[1].set_normal(M::vector_type::z_direction()); // placeholder, surface vertex normal cannot be calculated at this point
        avs[1].set_texcoord({tx1, ty});

        avss[i] = avs;
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, texture_span_type<M> ts)
{
    assert(polygon.sections() && polygon.sections().value().empty());

    size_t section_count = polygon.edge_count();
    vertices_attributes_sections_type<M> avss(section_count);

    for (size_t i = 0; i < section_count; ++i)
    {
        avss[i] = vertices_attributes_section<M>(polygon, i, 1, z, ts);
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts)
{
    assert(polygon.sections() && polygon.sections().value().empty());

    size_t section_count = polygon.edge_count();
    vertices_attributes_sections_type<M> avss(section_count);

    for (size_t i = 0; i < section_count; ++i)
    {
        avss[i] = vertices_attributes_section<M>(polygon, i, 1, z, scale, revs, ts);
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t edge_count = polygon.edge_count();
    vertices_attributes_type<M> avs(edge_count + 1);

    auto center = denoise0(polygon.centroid());

    auto matrix = geometry::translation(-center.vector());
    matrix *= geometry::alignment2(polygon.orientation().direction(), polygon.orientation().right(), op.orientation().direction(), op.orientation().right());
    matrix *= geometry::translation(op.position().vector());

    for (size_t i = 0; i < edge_count; ++i)
    {
        T t = T(i) / edge_count;
        avs[i].set_position(geometry::to_point(polygon.vertex(i) - center) * matrix);
        avs[i].set_normal(normalize(avs[i].position() - op.position())); // not enough info to properly calculate normal here ...
        avs[i].set_texcoord({math::lerp(ts[0], ts[1], t), ts[2]});
    }

    avs[edge_count] = avs[0];
    avs[edge_count].texcoord().set_x(ts[1]);
    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    // ...
    return vertices_attributes_sections_type<M>(); // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    // ...
    return vertices_attributes_sections_type<M>(); // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, std::optional<data_value_type<M>> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    vertices_attributes_type<M> avs(edge_count + 1);

    for (size_t i = 0; i <= edge_count; ++i)
    {
        T t = T(i) / edge_count;
        auto position = internal::polygon_vertex<M>(polygon, edge0 + i);
        if (z)
        {
            position.set_z(z.value());
        }
        avs[i].set_position(position);
        avs[i].set_normal(M::vector_type::z_direction()); // placeholder, surface vertex normal cannot be calculated at this point
        avs[i].set_texcoord({math::lerp(ts[0], ts[1], t), ts[2]});
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, std::optional<data_value_type<M>> z, data_value_type<M> scale, data_value_type<M> revs, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    vertices_attributes_type<M> avs(edge_count + 1);

    T phi = math::PiTwo<T> * revs;
    T cos_phi = math::denoise011(cos(phi));
    T sin_phi = math::denoise011(sin(phi));

    for (size_t i = 0; i <= edge_count; ++i)
    {
        T t = T(i) / edge_count;

        auto position = internal::polygon_vertex<M>(polygon, edge0 + i);
        position = {scale * (position.x() * cos_phi - position.y() * sin_phi), scale * (position.x() * sin_phi + position.y() * cos_phi), z.value_or(position.z())};
        avs[i].set_position(denoise0(position));
        avs[i].set_normal(M::vector_type::z_direction()); // placeholder, surface vertex normal cannot be calculated at this point
        avs[i].set_texcoord({math::lerp(ts[0], ts[1], t), ts[2]});
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes(const vertices_attributes_type<M>& avs_base, data_value_type<M> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t nAzimuth = avs_base.size() - 1;
    vertices_attributes_type<M> avs(nAzimuth);

// apex and base versions? normal needs to be reversed in one of them ...

    typename M::point_type position = {T(0), T(0), z};

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        typename M::point_type center_position = math::slerp(avs_base[i].position().vector(), avs_base[i + 1].position().vector(), T(0.5));
        typename M::vector_type edge = center_position - position;
        auto plane_normal = cross(M::vector_type::z_direction(), edge);
        auto normal = normalize(denoise0(cross(plane_normal, edge)));

        avs[i] = {position, normal, {T(0.5), ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes(size_t nAzimuth, data_value_type<M> z, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts, vertex_normal_function_type<M> vertex_normal)
{
    using T = data_value_type<M>;

    vertices_attributes_type<M> avs(nAzimuth);
    typename M::point_type position = {T(0), T(0), z};

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        T t = (i + T(0.5)) / T(nAzimuth);
        T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
        auto normal = vertex_normal(phi);

        avs[i] = {position, normal, {T(0.5), ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes(size_t nAzimuth, data_value_type<M> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    vertices_attributes_type<M> avs(nAzimuth);

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        // Normal vector is a placeholder, surface vertex normal cannot be calculated at this point
        avs[i] = {{T(0), T(0), z}, M::vector_type::z_direction(), {T(0.5), ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t edge_count = polygon.edge_count();
    assert(edge_count > 2);

    vertices_attributes_type<M> avs(edge_count);
    T tx = T(0.5);

    for (size_t i = 0; i < edge_count; ++i)
    {
        // Normal vector is a placeholder, surface vertex normal cannot be calculated at this point
        avs[i] = {{T(0), T(0), z}, M::vector_type::z_direction(), {tx, ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::cusp_vertices_attributes_edges(size_t nAzimuth, data_value_type<M> z, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    vertices_attributes_sections_type<M> avss(nAzimuth);

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        vertices_attributes_type<M>& avs = avss[i];
        // Normal vector is a placeholder, surface vertex normal cannot be calculated at this point
        avs.push_back({{T(0), T(0), z}, M::vector_type::z_direction(), {T(0.5), ts[2]}});
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::cusp_vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts)
{
    size_t section_count = polygon.edge_count();
    assert(section_count > 2);
    vertices_attributes_sections_type<M> avss(section_count);

    for (size_t i = 0; i < section_count; ++i)
    {
        avss[i] = cusp_vertices_attributes_section<M>(polygon, i, 1, z, ts);
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::cusp_vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, data_value_type<M> z, texture_span_type<M> ts)
{
    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();

    vertices_attributes_sections_type<M> avss(section_count);

    for (size_t i = 0; i < section_count; ++i)
    {
        auto [edge0, edge_count] = internal::section<M>(polygon, i);
        avss[i] = cusp_vertices_attributes_section<M>(polygon, edge0, edge_count, z, ts);
    }

    return avss;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    using T = data_value_type<M>;

    size_t edge_count = polygon.edge_count();
    assert(edge_count > 2);

    vertices_attributes_type<M> avs(edge_count);
    T tx = T(0.5);

    for (size_t i = 0; i < edge_count; ++i)
    {
        // Normal vector is a placeholder, surface vertex normal cannot be calculated at this point
        avs[i] = {op.position(), M::vector_type::z_direction(), {tx, ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::cusp_vertices_attributes_edges(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    // ...
    return vertices_attributes_sections_type<M>(); // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_sections_type<M> quetzal::model::cusp_vertices_attributes_sections(const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::OrientedPoint<typename M::vector_traits>& op, texture_span_type<M> ts)
{
    // ...
    return vertices_attributes_sections_type<M>(); // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::cusp_vertices_attributes_section(const geometry::Polygon<typename M::vector_traits>& polygon, size_t edge0, size_t edge_count, data_value_type<M> z, texture_span_type<M> ts)
{
    // These are not used in the cusp av calculation
    polygon;
    edge0;

    using T = data_value_type<M>;

    vertices_attributes_type<M> avs(edge_count);
    T tx = T(0.5); // ts ...

    for (size_t i = 0; i < edge_count; ++i)
    {
        // Normal vector is a placeholder, surface vertex normal cannot be calculated at this point
        avs[i] = {{T(0), T(0), z}, M::vector_type::z_direction(), {tx, ts[2]}};
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::border_vertices_attributes(const M& mesh, id_type halfedge_id)
{
    assert(mesh.halfedge(halfedge_id).border());

    using T = data_value_type<M>;

    vertices_attributes_type<M> avs;

// re-analyze, then validate laminaria_20 to determine which way this should go ...
    halfedge_id = prev_border_halfedge_id(mesh, halfedge_id);
//    halfedge_id = next_border_halfedge_id(mesh, halfedge_id);

    id_type id = halfedge_id;
    do
    {
        avs.emplace_back(mesh.halfedge(id).vertex_attributes());
        id = prev_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return avs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::model::vertices_attributes_type<Traits> quetzal::model::vertices_attributes(const brep::Face<Traits>& face)
{
    vertices_attributes_type<Traits> avs(face.halfedge_count());
    std::ranges::transform(face.halfedges(), avs.begin(), [](const auto& halfedge) -> Traits::vertex_attributes_type { return halfedge.vertex_attributes(); });

    return avs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::model::vertices_attributes_type<Traits> quetzal::model::vertices_attributes(const brep::Surface<Traits>& surface)
{
    assert(surface.seam_count() == 1); // this is really vertices_attributes(seam) ...
    const auto& seam = surface.seams().front();

    vertices_attributes_type<Traits> avs(seam.halfedge_count());
    std::ranges::transform(seam.halfedges(), avs.begin(), [](const auto& halfedge) -> Traits::vertex_attributes_type { return halfedge.vertex_attributes(); });

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::shift(vertices_attributes_type<M>& avs, data_value_type<M> z, std::optional<texture_span_type<M>> ts)
{
    size_t nAzimuth = avs.size() - 1;
    auto av0 = avs[1];

    for (size_t i = 0; i <= nAzimuth; ++i)
    {
        auto& av = avs[i];

        auto position = i < nAzimuth ? avs[i + 1].position() : av0.position();
        position.set_z(z);
        av.set_position(position);

        auto normal = i < nAzimuth ? avs[i + 1].normal() : av0.normal();
        av.set_normal(normal);

        if (ts)
        {
            av.texcoord().set_y(ts.value()[2]);
        }
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::vertices_attributes_type<M> quetzal::model::shift(vertices_attributes_type<M>& avs, data_value_type<M> radius, data_value_type<M> z, std::optional<texture_span_type<M>> ts)
{
    size_t nAzimuth = avs.size() - 1;
    auto av0 = avs[1];

    for (size_t i = 0; i <= nAzimuth; ++i)
    {
        auto& av = avs[i];

        auto position = i < nAzimuth ? avs[i + 1].position() : av0.position();
        position.set_z(data_value_type<M>(0));
        position = radius * normalize(position.vector());
        position.set_z(z);
        av.set_position(position);

        auto normal = i < nAzimuth ? avs[i + 1].normal() : av0.normal();
        av.set_normal(normal);

        if (ts)
        {
            av.texcoord().set_y(ts.value()[2]);
        }
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename M>
std::ostream& quetzal::model::operator<<(std::ostream& os, const vertices_attributes_type<M>& avs)
{
    for (const auto& av : avs)
    {
        os << av << '\n';
    }
    return os;
}

//------------------------------------------------------------------------------
template<typename M>
std::ostream& quetzal::model::operator<<(std::ostream& os, const vertices_attributes_sections_type<M>& avss)
{
    size_t i = 0;
    for (const auto& avs : avss)
    {
        os << i << '\t' << avss[i].size() << '\n';
        os << avs;
    }
    return os;
}

#endif // QUETZAL_MODEL_VERTICES_ATTRIBUTES_HPP
