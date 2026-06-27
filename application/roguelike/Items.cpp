//------------------------------------------------------------------------------
// roguelike
// Items.cpp
//------------------------------------------------------------------------------

#include "Items.hpp"
#include "Item.hpp"
#include "quetzal/XML.hpp"
#include "quetzal/Exception.hpp"
#include <fstream>
#include <sstream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Items::Items() :
    m_items()
{
}

//------------------------------------------------------------------------------
float quetzal::roguelike::Items::weight() const
{
    float total = 0.f;
    for (const auto& i : m_items)
    {
        total += i.second->weight();
    }

    return total;
}

//------------------------------------------------------------------------------
shared_ptr<quetzal::roguelike::Item> quetzal::roguelike::Items::find(id_type id)
{
    auto i = m_items.find(id);
    if (i != m_items.end())
    {
        return i->second;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Items::add(shared_ptr<Item> pitem, int quantity)
{
    assert(pitem != nullptr);

    auto i = m_items.find(pitem->id());

    if (pitem->quantified())
    {
        if (i != m_items.end())
        {
            pitem->set_quantity(pitem->quantity() + quantity);
        }
        else
        {
            pitem->set_quantity(quantity);
            m_items[pitem->id()] = pitem;
        }
    }
    else
    {
        assert(i == m_items.end());
        assert(quantity == 1);
        m_items[pitem->id()] = pitem;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Items::remove(id_type id, int quantity)
{
    auto i = m_items.find(id);
    assert(i != m_items.end());

    Item* pitem = i->second.get();
    assert(pitem != nullptr);
    if (pitem->quantified())
    {
        if (quantity == pitem->quantity())
        {
            m_items.erase(i);
        }
        else
        {
            assert(quantity < pitem->quantity());
            pitem->set_quantity(pitem->quantity() - quantity);
        }
    }
    else
    {
        assert(quantity == 1);
        m_items.erase(i);
    }

    return;
}
