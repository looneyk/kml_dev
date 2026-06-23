//------------------------------------------------------------------------------
// Light Matrix Simulation
// Pattern.cpp
//------------------------------------------------------------------------------

#include "Animation.hpp"
#include "Panel.hpp"
#include "Pattern.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/xml/Document.hpp"
#include <fstream>
#include <ranges>
#include <sstream>
#include <cassert>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    template<typename T>
    void parse2(const string& str, T& x, T& y)
    {
        istringstream iss(str);
        iss >> x >> y;
        return;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    string format2(T x, T y)
    {
        ostringstream oss;
        oss << x << " " << y;
        return oss.str();
    }

    //--------------------------------------------------------------------------
    void load_colors(const quetzal::xml::Element& elm, quetzal::lightmatrix::Pattern& pattern)
    {
        auto pe = elm.sub_element("size");
        assert(pe);
        size_t w = quetzal::to_type<size_t>(*pe->attribute_value("width"));
        size_t h = quetzal::to_type<size_t>(*pe->attribute_value("height"));
        pattern.set_size({w, h});

        pe = elm.sub_element("colors");
        assert(pe);
        istringstream iss(pe->value());
        for (size_t j = 0; j < h; ++j)
        {
            for (size_t i = 0; i < w; ++i)
            {
                iss >> pattern.color(i, j);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void store_colors(quetzal::xml::Element& elm, const quetzal::lightmatrix::Pattern& pattern)
    {
        auto pe = elm.add_element("size");
        const auto [w, h] = pattern.size();
        pe->set_attribute("width", quetzal::to_string(w));
        pe->set_attribute("height", quetzal::to_string(h));

        pe = elm.add_element("colors");
        size_t depth = pe->depth();

        ostringstream oss;
        oss << endl << string((depth + 1) * 4, ' ');
        for (size_t j = 0; j < h; ++j)
        {
            for (size_t i = 0; i < w; ++i)
            {
                oss << pattern.color(i, j);

                if (i < w - 1)
                {
                    oss << " ";
                }
                else
                {
                    if (j < h - 1)
                    {
                        oss << endl << string((depth + 1) * 4, ' ');
                    }
                    else
                    {
                        oss << endl << string(depth * 4, ' ');
                    }
                }
            }
        }

        pe->set_value(oss.str());

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pattern::Pattern(size_type size) :
    m_size(size),
    m_colors(size[0] * size[1])
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pattern::size_type quetzal::lightmatrix::Pattern::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pattern::set_size(size_type size)
{
    m_size = size;
    m_colors.resize(m_size[0] * m_size[1]);
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Color& quetzal::lightmatrix::Pattern::color(size_t x, size_t y) const
{
    return m_colors[y * m_size[0] + x];
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Color& quetzal::lightmatrix::Pattern::color(size_t x, size_t y)
{
    return m_colors[y * m_size[0] + x];
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Pattern::colors_type& quetzal::lightmatrix::Pattern::colors() const
{
    return m_colors;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pattern::colors_type& quetzal::lightmatrix::Pattern::colors()
{
    return m_colors;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pattern::load(const filesystem::path& file_path)
{
    xml::Document document;

    ifstream is(file_path);
    is >> document;

    const xml::Element& elm = document.element();
    if (elm.name() != "pattern")
    {
        return;
    }

    load_colors(elm, *this);

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pattern::store(const filesystem::path& file_path)
{
    xml::Document document;

    xml::Element& elm = document.element();
    elm.set_name("pattern");

    store_colors(elm, *this);

    ofstream os(file_path);
    os << document;

    return;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pattern quetzal::lightmatrix::lerp(const Pattern& pattern0, const Pattern& pattern1, float t)
{
    assert(pattern0.size() == pattern1.size());

    Pattern pattern(pattern0.size());

    for (auto [c0, c1, c] : views::zip(pattern0.colors(), pattern1.colors(), pattern.colors()))
    {
        c = lerp(c0, c1, t);
    }

    return pattern;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pattern quetzal::lightmatrix::load_pattern(const filesystem::path& file_path)
{
    Pattern pattern;
    pattern.load(file_path);
    return pattern;
}

//------------------------------------------------------------------------------
std::tuple<quetzal::lightmatrix::Pattern, quetzal::lightmatrix::Panel, quetzal::lightmatrix::Animation> quetzal::lightmatrix::load_pattern_layout(const std::filesystem::path& file_path)
{
    Pattern pattern;
    Panel panel;
    Animation animation;

    xml::Document document;

    ifstream is(file_path);
    is >> document;

    const xml::Element& elm = document.element();
    if (elm.name() != "pattern")
    {
        return {pattern, panel, animation};
    }

    load_colors(elm, pattern);

    // Panel
    auto pe = elm.sub_element("panel");
    assert(pe);
    panel.set_arrangement(to_arrangement(*pe->element_value("arrangement")));

    // Animation
    pe = elm.sub_element("animation");
    assert(pe);
    animation.set_shift_color(to_type<bool>(*pe->element_value("shift_color")));
    animation.set_shift_brightness(to_type<bool>(*pe->element_value("shift_brightness")));
    animation.set_smoothing(to_smoothing(*pe->element_value("smoothing")));

    float dx = 0.0f, dy = 0.0f;
    parse2(*pe->element_value("direction"), dx, dy);
    animation.set_direction({dx, dy});

    float vx = 0.0f, vy = 0.0f;
    parse2(*pe->element_value("velocity"), vx, vy);
    animation.set_velocity({vx, vy});

    return {pattern, panel, animation};
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::store_pattern_layout(const std::filesystem::path& file_path, const Pattern& pattern, const Panel& panel, const Animation& animation)
{
    xml::Document document;

    xml::Element& elm = document.element();
    elm.set_name("pattern");

    store_colors(elm, pattern);

    // Panel
    auto pe = elm.add_element("panel");
    pe->add_element("arrangement", to_string(panel.arrangement()));

    // Animation
    pe = elm.add_element("animation");
    pe->add_element("shift_color", quetzal::to_string(animation.shift_color()));
    pe->add_element("shift_brightness", quetzal::to_string(animation.shift_brightness()));
    pe->add_element("smoothing", to_string(animation.smoothing()));
    pe->add_element("direction", format2(animation.direction().x(), animation.direction().y()));
    pe->add_element("velocity", format2(animation.velocity().x(), animation.velocity().y()));

    ofstream os(file_path);
    os << document;

    return;
}
