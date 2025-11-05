#if !defined(QUETZAL_BREP_VISUALIZER_HPP)
#define QUETZAL_BREP_VISUALIZER_HPP
//------------------------------------------------------------------------------
// brep
// visualizer.hpp
//------------------------------------------------------------------------------

//#include "quetzal/geometry/point_util.hpp"
#include "quetzal/svg/Document.hpp"
#include <filesystem>
#include <functional>
#include <string>

namespace quetzal::brep
{

    template<typename M>
    class Visualizer
    {
    public:

        using value_type = M::value_type;
        using submesh_type = M::submesh_type;
        using face_type = M::face_type;
        using halfedge_type = M::halfedge_type;
        using projected_traits = M::vector_traits::template resized_traits<M::vector_traits::dimension - 1>;
        using projected_type = geometry::Point<projected_traits>;
        using projection_type = std::function<projected_type(const typename M::point_type&)>;

        Visualizer(const projection_type& projection, value_type detail_scale, value_type font_scale);
        Visualizer(const Visualizer&) = default;
        Visualizer(Visualizer&&) noexcept = default;
        ~Visualizer() = default;

        Visualizer& operator=(const Visualizer&) = default;
        Visualizer& operator=(Visualizer&&) = default;

        void set_projection(const projection_type& projection);
        void set_scale(value_type detail_scale, value_type font_scale);

        // arrowhead_fill should be replaced with a more general style parameter ...
        void draw_submesh(const submesh_type& submesh, bool arrowhead_fill = false);
        void draw_face(const face_type& face, bool arrowhead_fill);
        void draw_face_id(const face_type& face);
        void draw_halfedge(const halfedge_type& halfedge, bool arrowhead_fill);

        void write(const std::filesystem::path& file_path);

        void clear();

    private:

        void init_document();

        // Performs the projection and converts to SVG point type
        svg::Document::point_type projection(const typename M::point_type& position) const;

        svg::Document m_document;
        projection_type m_projection;
        value_type m_detail_scale;
        value_type m_font_scale;
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::Visualizer<M>::Visualizer(const projection_type& projection, value_type detail_scale, value_type font_scale) :
    m_document(),
    m_projection(projection),
    m_detail_scale(detail_scale),
    m_font_scale(font_scale)
{
    init_document();
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::set_projection(const projection_type& projection)
{
    m_projection = projection;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::set_scale(value_type detail_scale, value_type font_scale)
{
    m_detail_scale = detail_scale;
    m_font_scale = font_scale;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_submesh(const submesh_type& submesh, bool arrowhead_fill)
{
    for (const auto& face : submesh.faces())
    {
        draw_face(face, arrowhead_fill);
    }

    for (const auto& hole : submesh.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            draw_halfedge(halfedge, arrowhead_fill);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_face(const face_type& face, bool arrowhead_fill)
{
    draw_face_id(face);

    for (const auto& halfedge : face.halfedges())
    {
        draw_halfedge(halfedge, arrowhead_fill);
    }

    for (const auto& hole : face.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            draw_halfedge(halfedge, arrowhead_fill);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_face_id(const face_type& face)
{
    auto position = vertex_centroid(face);
    auto direction = typename M::vector_type::x_direction(); // this should be calculated from normal and up vectors ...
    auto position0 = position - direction * m_font_scale;
    auto position1 = position + direction * m_font_scale * value_type(100.0);
    // ideally -/+ the same large value and text centered ...

    auto p0 = projection(position0);
    auto p1 = projection(position1);
    auto path_text = std::format("M {} {} L {} {}", p0.x(), p0.y(), p1.x(), p1.y());

    // Draw face id
    auto text = std::to_string(face.id());
    m_document.text_path(text, xml::Attribute("path", path_text), xml::Attribute("font-size", std::to_string(m_font_scale)));
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_halfedge(const halfedge_type& halfedge, bool arrowhead_fill)
{
    // Half arrow

    auto position0 = vertex_position(halfedge);
    auto position1 = vertex_position(halfedge.next());
    auto normal = face_normal(halfedge);
    auto forward = normalize(position1 - position0);
    auto left = normalize(cross(normal, forward)); // directed toward interior of face

    m_document.update_size(projection(position0)); // All drawing will be within the face bounds, so just update with each halfedge original vertex position here

    value_type detail_scale_two = m_detail_scale * value_type(2);
    value_type detail_scale_half = m_detail_scale / value_type(2);

    auto indent_scale = m_detail_scale;
    auto length_half = (position1 - position0).norm() / value_type(2);
    if (length_half < detail_scale_two) // Don't indent more than one quarter of the length
    {
        indent_scale *= length_half / detail_scale_two;
    }
    auto endpoint_indent = forward * indent_scale;
    position0 += endpoint_indent;
    position1 -= endpoint_indent;

    auto edge_spacing = left * detail_scale_half;
    position0 += edge_spacing;
    position1 += edge_spacing;

    auto arrow_width = edge_spacing;
    auto arrow_length = forward * m_detail_scale;
    auto position2 = position1 + arrow_width - arrow_length; // arrowhead barb position
    auto position3 = position1 - arrow_length; // arrowhead base position

    auto p0 = projection(position0);
    auto p1 = projection(position1);
    auto p2 = projection(position2);
    auto p3 = projection(position3);

    m_document.line(p0, p1, xml::Attribute("style", "stroke:black;vector-effect:non-scaling-stroke;")); // shaft
    m_document.line(p1, p2, xml::Attribute("style", "stroke:black;vector-effect:non-scaling-stroke;")); // barb
    m_document.line(p2, p3, xml::Attribute("style", "stroke:black;vector-effect:non-scaling-stroke;")); // base
    if (arrowhead_fill)
    {
        m_document.polygon({p1, p2, p3}, xml::Attribute("style", "fill:black;"));
    }

    // Caption

    indent_scale *= value_type(2);
    auto caption_position0 = position0 + left * detail_scale_half + forward * indent_scale;
    auto caption_position1 = caption_position0 + forward * value_type(100.0); // Really, no end limit ...

    auto cp0 = projection(caption_position0);
    auto cp1 = projection(caption_position1);
    auto path_text = std::format("M {} {} L {} {}", cp0.x(), cp0.y(), cp1.x(), cp1.y());

    size_t digit_count = 1; // 2 (with better point formatting support, no need for breaking out by components here) ...
    auto caption = std::format("{} {} ({:.{}f}, {:.{}f}, {:.{}f})", halfedge.id(), halfedge.face_id(), position0.x(), digit_count, position0.y(), digit_count, position0.z(), digit_count);
    m_document.text_path(caption, xml::Attribute("path", path_text), xml::Attribute("font-size", std::to_string(m_font_scale)));
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::write(const std::filesystem::path& file_path)
{
    m_document.write(file_path);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::clear()
{
    m_document.clear();
    init_document();
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::init_document()
{
    m_document.set_viewport({1000.0, 1000.0});
    m_document.set_view_box_border_size({0.05, 0.05});
    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::svg::Document::point_type quetzal::brep::Visualizer<M>::projection(const typename M::point_type& position) const
{
    auto projected = m_projection(position);
//    return to_cartesian(projected); // vector traits are the same but not being recognized as such ...
    return {projected.x(), projected.y()}; // Should be -y for SVG coordinate system, why not? ...
}

#endif // QUETZAL_BREP_VISUALIZER_HPP
