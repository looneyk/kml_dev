//------------------------------------------------------------------------------
// svg
// Document.cpp
//------------------------------------------------------------------------------

#include "Document.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::svg::Document::Document(const size_type& viewport_size) :
    m_document(),
    m_pelement(&m_document.element()),
    m_viewport_size(viewport_size),
    m_view_box(),
    m_view_size(),
    m_view_box_border_size()
{
    set_svg();
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::set_viewport(const size_type& size)
{
    m_viewport_size = size;
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::set_view_box(const point_type& position, const size_type& size)
{
    m_view_box = {position, size};
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::set_view_box_border_size(size_type ratio)
{
    m_view_box_border_size = ratio;
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::write(const filesystem::path& file_path)
{
    finalize();

    ofstream ofs(file_path, ios_base::binary);
    assert(ofs.good());
    ofs << m_document << flush;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::clear()
{
    m_document.clear();
    m_pelement = &m_document.element();
    m_view_size = {};
    m_viewport_size = {};
    m_view_box = std::nullopt;
    m_view_box_border_size = {};
    set_svg();
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::close_symbol()
{
    m_pelement = m_pelement->parent();
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::close_group()
{
    m_pelement = m_pelement->parent();
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::close_defs()
{
    m_pelement = m_pelement->parent();
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::close_svg()
{
    m_pelement = m_pelement->parent();
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::update_size(point_type point)
{
    m_view_size.insert(point);
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::set_svg()
{
    m_document.element().set_name("svg");
    m_document.element().set_attribute("xmlns", "http://www.w3.org/2000/svg");
    m_document.element().set_attribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
    return;
}

//------------------------------------------------------------------------------
void quetzal::svg::Document::finalize()
{
    m_document.element().set_attribute("width", std::to_string(m_viewport_size.x()));
    m_document.element().set_attribute("height", std::to_string(m_viewport_size.y()));

    point_type view_box_position;
    size_type view_box_size;
    if (m_view_box)
    {
        auto view_box = m_view_box.value();
        view_box_position = std::get<0>(view_box);
        view_box_size = std::get<1>(view_box);
    }
    else
    {
        auto border_size = m_view_size.extent() * m_view_box_border_size;
        view_box_position = m_view_size.lower() - border_size;
        view_box_size = m_view_size.extent() + 2.0 * border_size;
    }

    auto view_box_text = std::format("{} {} {} {}", view_box_position.x(), view_box_position.y(), view_box_size.x(), view_box_size.y());
    m_document.element().set_attribute("viewBox", view_box_text);

    return;
}
