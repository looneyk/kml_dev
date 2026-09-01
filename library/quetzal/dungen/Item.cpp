//------------------------------------------------------------------------------
// dungen
// Item.cpp
//------------------------------------------------------------------------------

#include "Item.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::dungen::Item::Item() :
    Item(std::nullopt, std::nullopt, true, true)
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Item::Item(const std::optional<Item>& item_ancestor, bool cascade, bool single) :
    Item(item_ancestor, std::nullopt, cascade, single)
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Item::Item(const std::optional<Item>& item_ancestor, const std::optional<bounds_type>& bounds, bool cascade, bool single) :
    m_item_ancestor(item_ancestor),
    m_items_descendant(),
    m_cascade(cascade),
    m_single(single),
    m_bounds(bounds),
    m_actions(),
    m_applied()
{
}

//------------------------------------------------------------------------------
const std::optional<quetzal::dungen::Item>& quetzal::dungen::Item::ancestor() const
{
    return m_item_ancestor;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Item::set_ancestor(const Item& item_ancestor)
{
    m_item_ancestor.emplace(item_ancestor);
    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Item::items_type quetzal::dungen::Item::descendants() const
{
    return m_items_descendant;
}

//------------------------------------------------------------------------------
quetzal::dungen::Item::items_type quetzal::dungen::Item::descendants()
{
    return m_items_descendant;
}

//------------------------------------------------------------------------------
const std::optional<quetzal::dungen::Item::bounds_type>& quetzal::dungen::Item::bounds() const
{
    return m_bounds;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Item::set_bounds(const bounds_type& bounds)
{
    m_bounds.emplace(bounds);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Item::set_action(const std::string& action, action_handler_type handler)
{
    m_actions[action] = handler;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::dungen::Item::intersects(point_type point)
{
    assert(m_bounds);
    return m_bounds->compare(point) <= 0;
}

//------------------------------------------------------------------------------
bool quetzal::dungen::Item::apply(const std::string& action, point_type point)
{
    bool applied = false;

    if (m_cascade)
    {
        for (auto& item : m_items_descendant)
        {
            if (item.second.intersects(point))
            {
                if (item.second.apply(action, point))
                {
                    applied = true;
                    if (m_single)
                    {
                        break;
                    }
                }
            }
        }

        if (applied)
        {
            applied_descendant(action, point);
        }
    }

    if (!applied || !m_single)
    {
        applied |= apply_self(action, point);
    }

    return applied;
}

//------------------------------------------------------------------------------
bool quetzal::dungen::Item::apply_self(const std::string& action, point_type point)
{
    if (!m_actions.contains(action))
    {
        return false;
    }

    return m_actions[action](*this, action, point);
}

//------------------------------------------------------------------------------
void quetzal::dungen::Item::applied_descendant(const std::string& action, point_type point)
{
    m_applied(*this, action, point);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Item::print(std::ostream& os) const
{
    return;
}

//------------------------------------------------------------------------------
std::ostream& quetzal::dungen::operator<<(std::ostream& os, const Item& item)
{
    item.print(os);
    return os;
}
