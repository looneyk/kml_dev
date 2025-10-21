//------------------------------------------------------------------------------
// ItemTemplate
//------------------------------------------------------------------------------

#include "ItemTemplate.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::ItemTemplate::ItemTemplate() :
    m_id(),
    m_type(),
    m_slot_types(),
    m_bQuantified(false),
    m_nItemsSlotCapacity(1),
    m_bContainer(false),
    m_events()
{
}

//------------------------------------------------------------------------------
string quetzal::roguelike::ItemTemplate::slot_types() const
{
    return m_slot_types;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::ItemTemplate::quantified() const
{
    return m_bQuantified;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::ItemTemplate::container() const
{
    return m_bContainer;
}
