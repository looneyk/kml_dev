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

        using submesh_type = M::submesh_type;
        using face_type = M::face_type;
        using halfedge_type = M::halfedge_type;
        using projected_traits = M::vector_traits::template resized_traits<M::vector_traits::dimension - 1>;
        using projected_type = geometry::Point<projected_traits>;
        using projection_type = std::function<projected_type(const typename M::point_type&)>;

        Visualizer(const projection_type& projection, double detail_scale, double font_scale);
        Visualizer(const Visualizer&) = default;
        Visualizer(Visualizer&&) noexcept = default;
        ~Visualizer() = default;

        Visualizer& operator=(const Visualizer&) = default;
        Visualizer& operator=(Visualizer&&) = default;

        void set_projection(const projection_type& projection);
        void set_scale(double detail_scale, double font_scale);

        void draw_submesh(const submesh_type& submesh);
        void draw_face(const face_type& face);
        void draw_halfedge(const halfedge_type& halfedge);

        void write(const std::filesystem::path& file_path);

        void clear();

    private:

        void init_document();

        svg::Document::point_type projection(const typename M::point_type& position) const;

        svg::Document m_document;
        projection_type m_projection;
        double m_detail_scale;
        double m_font_scale;
    };

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::brep::Visualizer<M>::Visualizer(const projection_type& projection, double detail_scale, double font_scale) :
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
void quetzal::brep::Visualizer<M>::set_scale(double detail_scale, double font_scale)
{
    m_detail_scale = detail_scale;
    m_font_scale = font_scale;
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_submesh(const submesh_type& submesh)
{
    for (const auto& face : submesh.faces())
    {
        draw_face(face);
    }

    for (const auto& hole : submesh.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            draw_halfedge(halfedge);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_face(const face_type& face)
{
    using p2_type = svg::Document::point_type;
    using v2_type = svg::Document::vector_type;
  
    auto position = vertex_centroid(face);
    auto p = projection(position);
    p2_type p0 = p - v2_type{m_font_scale, 0.0};
    p2_type p1 = p + v2_type{m_font_scale * 100.0, 0.0};
    auto path_text = std::format("M {} {} L {} {}", p0.x(), p0.y(), p1.x(), p1.y());

    // Draw face id
    auto text = std::to_string(face.id());
    m_document.text_path(text, xml::Attribute("path", path_text), xml::Attribute("font-size", std::to_string(m_font_scale)));
    // a start, but incomplete ...
    m_document.update_size(p0);
//    m_document.update_size(p1);

    for (const auto& halfedge : face.halfedges())
    {
        draw_halfedge(halfedge);
    }

    for (const auto& hole : face.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            draw_halfedge(halfedge);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::Visualizer<M>::draw_halfedge(const halfedge_type& halfedge)
{
    using p2_type = svg::Document::point_type;
    using v2_type = svg::Document::vector_type;

    auto position0 = vertex_position(halfedge);
    auto position1 = vertex_position(halfedge.next());
    auto normal = face_normal(halfedge);

    auto forward = normalize(position1 - position0);
    auto left = normalize(cross(normal, forward)); // directed toward interior of face

    // project everything just before drawing instead? ...
    // that might help with halfedge arrows ...
    // use forward instead of forward2 for arrow positions, then project, and similarly for text positions ...
    // still need a better way to orient text ...

    auto p0 = projection(position0);
    auto p1 = projection(position1);
    
    // Does not take text into account ...
    m_document.update_size(p0);
    m_document.update_size(p1);

    auto forward2 = projection(forward).vector();
    auto left2 = projection(left).vector();

    double detail_scale_two = m_detail_scale * 2.0;
    double detail_scale_half = m_detail_scale / 2.0;

    auto indent_scale = m_detail_scale;
    auto length_half = (p1 - p0).norm() / 2.0;
    if (length_half < detail_scale_two) // Don't indent more than one quarter of the length
    {
        indent_scale *= length_half / detail_scale_two;
    }
    auto arrow_indent = forward2 * indent_scale;
    p0 += arrow_indent;
    p1 -= arrow_indent;

    // Arrowheads are in projection space, but would make more sense in model space. Changing this would be at the expense of detail due to viewing angle ...
    // Changing left offset would not make anything worse however ...
    auto arrow_left_offset = left2 * detail_scale_half;
    p0 += arrow_left_offset;
    p1 += arrow_left_offset;
//    auto p2 = p1 + m_detail_scale * left2 - detail_scale_two * forward2;

    m_document.line(p0, p1, xml::Attribute("style", "stroke:black;vector-effect:non-scaling-stroke;"));

    auto p2 = p1 + detail_scale_half * left2 - m_detail_scale * forward2;
    m_document.line(p1, p2, xml::Attribute("style", "stroke:black;vector-effect:non-scaling-stroke;"));
//    auto p3 = p1 - m_detail_scale * forward2;
//    m_document.polygon({p1, p2, p3}, xml::Attribute("style", "fill:black;"));

    indent_scale *= 2.0;
    auto text0 = p0 + detail_scale_half * left2 + indent_scale * forward2;
    auto text1 = text0 + forward2 * 100.0; // Really, no end limit
    auto path_text = std::format("M {} {} L {} {}", text0.x(), text0.y(), text1.x(), text1.y());

    size_t digit_count = 1; // 2 (with better point formatting support, no need for breaking out by components here) ...
    auto text = std::format("{} {} ({:.{}f}, {:.{}f}, {:.{}f})", halfedge.id(), halfedge.face_id(), position0.x(), digit_count, position0.y(), digit_count, position0.z(), digit_count);
    m_document.text_path(text, xml::Attribute("path", path_text), xml::Attribute("font-size", std::to_string(m_font_scale)));
// determine actual text bounds, p0 and p1 form the base line, but p1 is not the actual text length, and the height is not accounted for ...
//    m_document.update_size(p0 + text_height);
//    m_document.update_size(p1 + text_height);
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
