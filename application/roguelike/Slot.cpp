//------------------------------------------------------------------------------
// Slot.cpp
//------------------------------------------------------------------------------

#include "Slot.hpp"
#include "Container.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Slot::Slot(const string& type) :
    m_type(type),
    m_pitem(nullptr)
{
}

//------------------------------------------------------------------------------
string quetzal::roguelike::Slot::type() const
{
    return m_type;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::Slot::holds(Item* pitem) const
{
    assert(pitem != nullptr);
    return pitem->slot_types().find(m_type) != string::npos;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::Slot::empty() const
{
    return m_pitem != nullptr;
}

//------------------------------------------------------------------------------
const shared_ptr<quetzal::roguelike::Item> quetzal::roguelike::Slot::item() const
{
    return m_pitem;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Slot::add(shared_ptr<Item> pitem)
{
    m_pitem = pitem;
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Slot::remove()
{
    m_pitem = nullptr;
    return;
}
