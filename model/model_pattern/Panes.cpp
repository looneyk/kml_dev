//------------------------------------------------------------------------------
// model_pattern
// Panes.cpp
//------------------------------------------------------------------------------

#include "Panes.hpp"
#include <algorithm>
#include <vector>

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
Panes::panes_type::const_iterator Panes::begin() const
{
    return m_panes.begin();
}

//------------------------------------------------------------------------------
Panes::panes_type::iterator Panes::begin()
{
    return m_panes.begin();
}

//------------------------------------------------------------------------------
Panes::panes_type::const_iterator Panes::end() const
{
    return m_panes.end();
}

//------------------------------------------------------------------------------
Panes::panes_type::iterator Panes::end()
{
    return m_panes.end();
}

//------------------------------------------------------------------------------
size_t Panes::instance_count() const
{
    return m_panes.size();
}

//------------------------------------------------------------------------------
size_t Panes::reference_count() const
{
    size_t n = 0;
    for (const auto& pane : m_panes)
    {
        n += pane.references().size();
    }

    return n;
}

//------------------------------------------------------------------------------
double Panes::area() const
{
    double a = 0;
    for (const auto& pane : m_panes)
    {
        a += pane.area() * pane.references().size();
    }

    return a;
}

//------------------------------------------------------------------------------
Panes::panes_type::iterator Panes::find(const Pane& pane)
{
    for (auto i = m_panes.begin(); i != m_panes.end(); ++i)
    {
        if (float_eq(*i, pane))
        {
            return i;
        }
    }

    return m_panes.end();
}

//------------------------------------------------------------------------------
void Panes::insert(const Pane& pane)
{
    m_panes.push_back(pane);
    return;
}

//------------------------------------------------------------------------------
void Panes::sort()
{
    std::ranges::sort(m_panes, [](const Pane& lhs, const Pane& rhs) { return lhs.area() > rhs.area(); });
    return;
}
