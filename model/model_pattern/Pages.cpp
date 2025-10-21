//------------------------------------------------------------------------------
// model_pattern
// Pages.cpp
//------------------------------------------------------------------------------

#include "Pages.hpp"
#include "Page.hpp"
#include "Pane.hpp"

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
Pages::Pages(vector_type page_size, double margin, double separation, const string& font, double font_size) :
    m_page_size(page_size),
    m_margin(margin),
    m_separation(separation),
    m_pages(),
    m_font(font),
    m_font_size(font_size)
{
}

//------------------------------------------------------------------------------
Pages::vector_type Pages::page_client_size() const
{
    return {m_page_size.x() - m_margin, m_page_size.y() - m_margin};
}

//------------------------------------------------------------------------------
Pages::pages_type::const_iterator Pages::begin() const
{
    return m_pages.begin();
}

//------------------------------------------------------------------------------
Pages::pages_type::iterator Pages::begin()
{
    return m_pages.begin();
}

//------------------------------------------------------------------------------
Pages::pages_type::const_iterator Pages::end() const
{
    return m_pages.end();
}

//------------------------------------------------------------------------------
Pages::pages_type::iterator Pages::end()
{
    return m_pages.end();
}

//------------------------------------------------------------------------------
size_t Pages::size() const
{
    return m_pages.size();
}

//------------------------------------------------------------------------------
bool Pages::holds(const Pane& pane)
{
    return pane.size().x() <= (m_page_size.x() - 2.0 * m_margin) && pane.size().y() <= (m_page_size.y() - 2.0 * m_margin);
}

//------------------------------------------------------------------------------
bool Pages::insert(const Pane& pane)
{
    if (!holds(pane)) // Larger than full page
    {
        // Split pane and insert each section
        auto ps = split(pane);
        for (const auto& p : ps)
        {
            insert(p);
        }
        return false;
    }

    for (auto& page : m_pages)
    {
        if (page.insert(pane))
        {
            return true;
        }
    }

    m_pages.emplace_back(m_page_size, m_margin, m_separation, m_font, m_font_size);
    return m_pages.back().insert(pane);
}

//------------------------------------------------------------------------------
vector<Pane> Pages::split(const Pane& pane)
{
    pane; // ...
    return {}; // with section info ...
}

//------------------------------------------------------------------------------
void Pages::draw(pdf::Document& document)
{
    for (auto& page : m_pages)
    {
        page.draw(document);
    }

    return;
}
