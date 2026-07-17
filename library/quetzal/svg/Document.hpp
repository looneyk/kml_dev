#if !defined(QUETZAL_SVG_DOCUMENT_HPP)
#define QUETZAL_SVG_DOCUMENT_HPP
//------------------------------------------------------------------------------
// svg
// Document.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/xml/Document.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace quetzal::svg
{

    //--------------------------------------------------------------------------
    class Document
    {
    public:

        using value_type = double;
        using vector_traits = math::CartesianTraits<value_type, 2>;
        using point_type = geometry::Point<vector_traits>;
        using vector_type = point_type::vector_type;
        using size_type = vector_type;

        Document(const size_type& viewport_size = {800.0, 800.0});
        Document(const Document&) = delete;
        Document(Document&&) = delete;
        ~Document() = default;

        Document& operator=(const Document&) = delete;
        Document& operator=(Document&&) = delete;

        // If these are set explicitly, they will take precedence over the auto-calculation values below
        void set_viewport(const size_type& size);
        void set_view_box(const point_type& position, const size_type& size);

        // Along with update_size, this is used for automatic calculation of view box
        void set_view_box_border_size(size_type ratio);

        void write(const std::filesystem::path& file_path);

        void clear();

        // return Element* here instead? ...
        // just write to document rather than exposing xml publicly? ...
        // fine for single xml element svg elements ...
        // any need to return anything at all? not currently, except to allow for additional attributes (text args in text_path)

        template<typename... Args>
        std::shared_ptr<xml::Element> rectangle(point_type position, value_type dx, value_type dy, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> circle(point_type position, value_type radius, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> ellipse(point_type position, value_type rx, value_type ry, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> line(point_type p0, point_type p1, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> polyline(const std::vector<point_type>& points, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> polygon(const std::vector<point_type>& points, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> path(const std::string& commands, Args... args);

        // expand text elements to handle nodes instead of just a single string (interleaved text and elements) ...
        template<typename... Args>
        std::shared_ptr<xml::Element> text(point_type position, const std::string& s, Args... args);

        // as with text, but also separate args for text and textPath elements ...
        template<typename... Args>
        std::shared_ptr<xml::Element> text_path(const std::string& s, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> use(const std::string& href, point_type position, size_type size, Args... args);

        template<typename... Args>
        std::shared_ptr<xml::Element> open_symbol(const std::string& id, size_type size, Args... args);

        void close_symbol();

        template<typename... Args>
        std::shared_ptr<xml::Element> open_group(Args... args);

        void close_group();

        template<typename... Args>
        std::shared_ptr<xml::Element> open_defs(Args... args);

        void close_defs();

        // no need for these, except possibly for custom args perhaps? ...
        template<typename... Args>
        std::shared_ptr<xml::Element> open_svg(Args... args);

        void close_svg();

        template<typename F, typename... Args>
        std::shared_ptr<xml::Element> defs(F&& f, Args&&... args);

        void update_size(point_type point);

    private:

        void set_svg();

        template<typename... Args>
        void set_attributes(std::shared_ptr<xml::Element> pelement, Args... args);

        void finalize(); // Set final viewport and view box positions and sizes

        xml::Document m_document;
        xml::Element* m_pelement;

        size_type m_viewport_size;
        std::optional<std::tuple<point_type, size_type>> m_view_box;

         geometry::AxisAlignedBoundingBox<vector_traits> m_view_size;

        // With view box border size provided, calculated viewBox = m_view_size + 2 * m_view_box_border_size
        size_type m_view_box_border_size; // The ratio of the border to the view size in each dimension
    };

} // namespace quetzal::svg

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::rectangle(point_type position, value_type dx, value_type dy, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("rect");
    set_attributes(pelement, xml::Attribute("x", quetzal::to_string(position.x())), xml::Attribute("y", quetzal::to_string(position.y())), xml::Attribute("width", quetzal::to_string(dx)), xml::Attribute("height", quetzal::to_string(dy)), args...);
    update_size(position);
    update_size(position + vector_type(dx, dy));
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::circle(point_type position, value_type radius, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("circle");
    set_attributes(pelement, xml::Attribute("cx", quetzal::to_string(position.x())), xml::Attribute("cy", quetzal::to_string(position.y())), xml::Attribute("r", quetzal::to_string(radius)), args...);
    update_size(position + vector_type(-radius, -radius));
    update_size(position + vector_type(radius, radius));
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::ellipse(point_type position, value_type rx, value_type ry, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("ellipse");
    set_attributes(pelement, xml::Attribute("cx", quetzal::to_string(position.x())), xml::Attribute("cy", quetzal::to_string(position.y())), xml::Attribute("rx", quetzal::to_string(rx)), xml::Attribute("ry", quetzal::to_string(ry)), args...);
    update_size(position + vector_type(-rx, -ry));
    update_size(position + vector_type(rx, ry));
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::line(point_type p0, point_type p1, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("line");
    set_attributes(pelement, xml::Attribute("x1", quetzal::to_string(p0.x())), xml::Attribute("y1", quetzal::to_string(p0.y())), xml::Attribute("x2", quetzal::to_string(p1.x())), xml::Attribute("y2", quetzal::to_string(p1.y())), args...);
    update_size(p0);
    update_size(p1);
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::polyline(const std::vector<point_type>& points, Args... args)
{
    auto points_text = points
        | std::views::transform([this](const point_type& p) { update_size(p); return p; })
        | std::views::transform([](const point_type& p) { return std::format("{},{}", p.x(), p.y()); })
        | std::views::join_with(' ')
        | std::ranges::to<std::string>();

    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("polyline");
    set_attributes(pelement, xml::Attribute("points", points_text), args...);
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::polygon(const std::vector<point_type>& points, Args... args)
{
    auto points_text = points
        | std::views::transform([this](const point_type& p) { update_size(p); return p; })
        | std::views::transform([](const point_type& p) { return std::format("{},{}", p.x(), p.y()); })
        | std::views::join_with(' ')
        | std::ranges::to<std::string>();

    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("polygon");
    set_attributes(pelement, xml::Attribute("points", points_text), args...);
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::path(const std::string& commands, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("path");
    set_attributes(pelement, xml::Attribute("d", commands), args...);
    // no update_size here, would need to parse the path commands? leave to caller ...
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::text(point_type position, const std::string& s, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("text", s);
    set_attributes(pelement, xml::Attribute("x", quetzal::to_string(position.x())), xml::Attribute("y", quetzal::to_string(position.y())), args...);
    update_size(position);
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::text_path(const std::string& s, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("text");
    std::shared_ptr<xml::Element> pelementPath = pelement->add_element("textPath", s);
    set_attributes(pelementPath, args...); // can there be text args, not just text path args? ...
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::use(const std::string& href, point_type position, size_type size, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("use");
    set_attributes(pelement, xml::Attribute("href", href), xml::Attribute("x", quetzal::to_string(position.x())), xml::Attribute("y", quetzal::to_string(position.y())), xml::Attribute("width", quetzal::to_string(size.x())), xml::Attribute("height", quetzal::to_string(size.y())), args...);
    update_size(position);
    update_size(position + size); // is there a possible offset? ...
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::open_symbol(const std::string& id, size_type size, Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("symbol");
//    set_attributes(pelement, xml::Attribute("id", id), xml::Attribute("width", quetzal::to_string(size.x())), xml::Attribute("height", quetzal::to_string(size.y())), xml::Attribute("refX", "center"), xml::Attribute("refY", "center"), args...);
size;
    set_attributes(pelement, xml::Attribute("id", quetzal::to_string(id)), xml::Attribute("refX", "center"), xml::Attribute("refY", "center"), args...);
    m_pelement = pelement.get();
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::open_group(Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("g");
    set_attributes(pelement, args...);
    m_pelement = pelement.get();
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::open_defs(Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("defs");
    set_attributes(pelement, args...);
    m_pelement = pelement.get();
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::open_svg(Args... args)
{
    std::shared_ptr<xml::Element> pelement = m_pelement->add_element("svg");
    set_attributes(pelement, args...);
    m_pelement = pelement.get();
    return pelement;
}

//------------------------------------------------------------------------------
template<typename F, typename... Args>
std::shared_ptr<quetzal::xml::Element> quetzal::svg::Document::defs(F&& f, Args&&... args)
{
    std::shared_ptr<xml::Element> pelementPrev = m_pelement;

    std::shared_ptr<xml::Element> pelementDefs = m_document.element().sub_element("defs");
    if (!pelementDefs)
    {
        if (m_document.element().elements().empty())
        {
            pelementDefs = open_defs();
            close_defs();
        }
        else
        {
            pelementDefs = m_document.element().elements().insert(m_document.element().elements().begin(), "defs");
        }
    }

    assert(pelementDefs);
    m_pelement = pelementDefs;
    std::shared_ptr<xml::Element> pelement = std::bind(std::forward<F>(f), std::forward<Args>(args)...)();
    m_pelement = pelementPrev;
    return pelement;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::svg::Document::set_attributes(std::shared_ptr<xml::Element> pelement, Args... args)
{
    ([&pelement](auto& arg) { pelement->set_attribute(arg.name(), arg.value()); }(args), ...);
    return;
}

#endif // QUETZAL_SVG_DOCUMENT_HPP
