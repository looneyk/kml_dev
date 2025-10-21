//------------------------------------------------------------------------------
// model_pattern
// Page.cpp
//------------------------------------------------------------------------------

#include "Page.hpp"
#include "quetzal/geometry/point_util.hpp"

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
Page::Page(const vector_type& size, double margin, double separation, const string& font, double font_size) :
    m_size(size),
    m_margin(margin),
    m_separation(separation),
    m_font(font),
    m_font_size(font_size),
    m_placements(),
    m_free()
{
    point_type lower = {m_margin, m_margin};
    point_type upper = {size.x() - m_margin * 2.0, size.y() - m_margin * 2.0};
    m_free.emplace_back(lower, upper);
}

//------------------------------------------------------------------------------
bool Page::insert(const Pane& pane)
{
    vector_type separation = {m_separation, m_separation};

    auto iFreeMin = m_free.end();
    double areaFreeMin = m_size.x() * m_size.y();

    // Find the free space that leaves the least remaining after placement, if any
    for (auto i = m_free.begin(); i != m_free.end(); ++i)
    {
        box_type box = *i;
        if (box.contains(box.lower() + pane.size()))
        {
            auto free = box.upper() - box.lower() - pane.size() - separation;
            auto area = free.x() * free.y();
            if (area < areaFreeMin)
            {
                areaFreeMin = area;
                iFreeMin = i;
            }
        }
    }

    if (iFreeMin != m_free.end())
    {
        // Use the best fitting free space for the placement and add new free space to replace it

        box_type box = *iFreeMin;
        m_free.erase(iFreeMin);
        m_placements.emplace_back(pane, box.lower());
        auto free = box.upper() - box.lower() - pane.size() - separation;
        if (free.y() >= free.x())
        {
            if (free.y() > 0.0)
            {
                // Add free space below
                point_type lower = {box.lower().x(), box.upper().y() - free.y()};
                point_type upper = box.upper();
                m_free.emplace_back(lower, upper);
            }

            if (free.x() > 0.0)
            {
                // Add free space on right
                point_type lower = {box.upper().x() - free.x(), box.lower().y()};
                point_type upper = {box.upper().x(), box.upper().y() - free.y() - m_separation};
                m_free.emplace_back(lower, upper);
            }
        }
        else
        {
            if (free.x() > 0.0)
            {
                // Add free space on right
                point_type lower = {box.upper().x() - free.x(), box.lower().y()};
                point_type upper = box.upper();
                m_free.emplace_back(lower, upper);
            }

            if (free.y() > 0.0)
            {
                // Add free space below
                point_type lower = {box.lower().x(), box.upper().y() - free.y()};
                point_type upper = {box.upper().x() - free.x() - m_separation, box.upper().x()};
                m_free.emplace_back(lower, upper);
            }
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void Page::draw(pdf::Document& document) const
{
    document.set_font(m_font, m_font_size);

    auto to_x = [&](double x) -> double { return x * document.page_width() / m_size.x(); };
    auto to_y = [&](double y) -> double { return (m_size.y() - y) * document.page_height() / m_size.y(); };
    auto to_page = [&](const point_type& point) -> pdf::point_type { return {to_x(point.x()), to_y(point.y())}; };

    for (const auto& placement : m_placements)
    {
        const auto& pane = placement.m_pane;
        auto lower = pane.bounds().lower();
        auto position = placement.m_position;
        auto center = placement.m_position + (centroid(pane.polygon()) - lower);

        auto pc = to_page(center);

        // Center point for reference
        document.draw_polygon({
            {pc.x() + 1, pc.y() + 0},
            {pc.x() + 0, pc.y() + 1},
            {pc.x() - 1, pc.y() + 0},
            {pc.x() + 0, pc.y() - 1}});

        // Draw pane polygon
        pdf::points_type points;
        for (const auto& vertex : pane.polygon().vertices())
        {
            points.emplace_back(to_cartesian(to_page(position + (vertex - lower))));
        }
        document.draw_polygon(points);

        // Draw grain direction
        auto direction = pane.direction();
        auto scaled = direction * 10.0;
        auto root = to_page(center - scaled);
        auto tip = to_page(center + scaled);
        vector_type direction_ccw = {-direction.y(), direction.x()};
        auto left = to_page(center + scaled * 0.8 + direction_ccw);
        auto right = to_page(center + scaled * 0.8 - direction_ccw);

        document.draw_line(root, tip);
        document.draw_line(tip, left);
        document.draw_line(tip, right);

        // Instance text, wrap to fit
        vector<string> lines;
        double wMaxMin = 10.0;
        double wMax = to_x(pane.bounds().extent().x() / 3.0);
        if (pane.bounds().extent().x() < wMaxMin * 3.0)
        {
            wMax = to_x(wMaxMin);
        }
        ostringstream oss;
        for (const auto& reference : pane.references())
        {
            if (!oss.str().empty())
            {
                oss << " ";
            }

            oss << reference;
            if (document.text_width(oss.str()) > wMax)
            {
                lines.push_back(oss.str());
                oss.str("");
            }
        }

        if (!oss.str().empty()) // Last line
        {
            lines.push_back(oss.str());
        }

        document.show_text_centered(lines, pc.x(), pc.y() - 3);
//        document.show_text_centered(lines, pc);

        // Reference count and section text
        oss.str("");
        oss << "\xD7" << pane.reference_count();
        if (pane.section_count() > 1)
        {
            oss << " (" << pane.section() << " of " << pane.section_count() << ")";
        }

        auto text_position = to_page(center - direction_ccw * 2.5);
        document.show_text_centered(oss.str(), text_position.x(), text_position.y()); // line spacing? ...
//        document.show_text_centered(oss.str(), to_page(center - direction_ccw * 2.5)); // line spacing? ...
    }

    return;
}
