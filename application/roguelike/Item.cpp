//------------------------------------------------------------------------------
// roguelike
// Item.cpp
//------------------------------------------------------------------------------

#include "Item.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Item::Item() :
    m_id(),
    m_pparent(nullptr),
//    m_pslot(nullptr),
    m_weight(0.f),
    m_bQuantified(false),
    m_quantity(0),
    m_bActivated(false)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::id_type quetzal::roguelike::Item::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
quetzal::roguelike::Item* quetzal::roguelike::Item::parent()
{
    return m_pparent;
}
/*
//------------------------------------------------------------------------------
string quetzal::roguelike::Item::slot_types() const
{
    return m_item_template.slot_types();
}

//------------------------------------------------------------------------------
quetzal::roguelike::Slot* quetzal::roguelike::Item::slot()
{
    return m_pslot;
}
*/
//------------------------------------------------------------------------------
bool quetzal::roguelike::Item::quantified() const
{
    return m_bQuantified;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::Item::quantity() const
{
    return m_quantity;
}

//------------------------------------------------------------------------------
float quetzal::roguelike::Item::weight() const
{
    return m_weight;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::Item::activated() const
{
    return m_bActivated;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Item::set_quantity(int n)
{
    m_quantity = n;
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Item::activate()
{
/*
    if owner
        if food
            owner.eat(this)
            remove this
    else if held
        if backpack
            add this
        else
            throw/drop this
    else
        if not holding
            player.hold(this)

    // ...
*/

    return;
}
