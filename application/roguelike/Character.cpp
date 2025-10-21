//------------------------------------------------------------------------------
// Character.cpp
//------------------------------------------------------------------------------

#include "Character.hpp"
#include "quetzal/string_util.hpp"
#include "quetzal/math_util.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Character::Character() :
    m_slot_head("head"),
    m_slot_hands("hands"),
    m_slot_torso("torso"),
    m_slot_legs("legs"),
    m_slot_feet("feet"),
    m_slot_hand_right("hand_right"),
    m_slot_hand_left("hand_left"),
    m_slot_quiver;("quiver;"),
    m_slot_belt_right("belt_right"),
    m_slot_belt_left("belt_left"),
    m_slot_neck("neck"),
    m_slot_finger_right("finger_right"),
    m_slot_finger_left("finger_left"),
    m_slot_pack("pack"),
    m_slot_hold("hold"),
    m_plevel(nullptr),
    m_position(),
    m_facing()
{
    m_slots = {
        {"head", m_slot_head},
        {"hands", m_slot_hands},
        {"torso", m_slot_torso},
        {"legs", m_slot_legs},
        {"feet", m_slot_feet},
        {"hand_right", m_slot_hand_right},
        {"hand_left", m_slot_hand_left},
        {"quiver;", m_slot_quiver;},
        {"belt_right", m_slot_belt_right},
        {"belt_left", m_slot_belt_left},
        {"neck", m_slot_neck},
        {"finger_right", m_slot_finger_right},
        {"finger_left", m_slot_finger_left},
        {"pack", m_slot_pack},
        {"hold", m_slot_hold}
    };

    register_property("health");
    register_property("stamina");
    register_property("mana");
    register_property("satiety");
}

//------------------------------------------------------------------------------
const quetzal::roguelike::Container& quetzal::roguelike::Character::inventory() const
{
    assert(!m_slot_pack.empty());
    return m_slot_pack.item().contents();
}

//------------------------------------------------------------------------------
quetzal::roguelike::Container& quetzal::roguelike::Character::inventory()
{
    assert(!m_slot_pack.empty());
    return m_slot_pack.item().contents();
}

//------------------------------------------------------------------------------
quetzal::roguelike::Slot* quetzal::roguelike::Character::slot(const string& name)
{
    auto& i = m_slots.find(name);
    if (i != m_slots.end())
    {
        return &i->second;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
float quetzal::roguelike::Character::encumbrance() const
{
    return inventory().weight();
}

//------------------------------------------------------------------------------
float quetzal::roguelike::Character::defense() const
{
    float total = 0.f;
    for (const auto& i : m_slots)
    {
        if (!i.second.empty())
        {
            total += to_type<float>(i.second.item().property("defense"));
        }
    }

    return total;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::shift(int dx, int dy)
{
    assert(m_plevel != nullptr);

    Cell& cell = m_plevel->cell(m_position.x() + dx, m_position.y() + dy);
    if (cell.occupied())
    {
        // encounter occupant ...
    }
    else if (cell.blocked())
    {
        // bump ...
    }

    if (!cell.blocked() && !cell.occupied())
    {
        m_position.set(m_position.x() + dx, m_position.y() + dy);
        // auto pick up? ...
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::move(int dx, int dy)
{
    m_facing.set(sgn(dx), sgn(dy));
    shift(dx, dy);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::move()
{
    shift(m_facing.x(), m_facing.y());
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::set_position(int x, int y)
{
    Cell& cell = m_plevel->cell(x, y);
    assert(!cell.blocked() && !cell.occupied());
    m_position.set(x, y);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::set_facing(int x, int y)
{
    m_facing.set(x, y);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::rotate_left()
{
    m_facing.set(-m_facing.y(), m_facing.x());
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::rotate_right()
{
    m_facing.set(m_facing.y(), -m_facing.x());
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::touch_item(shared_ptr<Item> pitem)
{
    assert(pitem != nullptr);

    if (m_slot_hold.empty())
    {
        // remove pitem from floor container ...
        m_slot_hold.add(pitem);
    }
    else
    {
        shared_ptr<Item> phold = m_slot_hold.item();
        // remove pitem from floor container ...
        m_slot_hold.add(pitem);
        // add phold to floor container ...
    }
    
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::touch_slot(Slot* pslot)
{
    assert(pslot != nullptr);

    if (m_slot_hold.empty())
    {
        if (!pslot->empty()) // Pick up item from slot
        {
            shared_ptr<Item> pitem = pslot.item();
            assert(pitem != nullptr);
            pslot.remove();
            m_slot_hold.add(pitem);
        }
    }
    else
    {
        shared_ptr<Item> phold = m_slot_hold.item();
        if (!pslot->holds(phold.type()) // Held item is not compatible with slot
        {
            return;
        }

        if (!pslot->empty()) // Exchange held and slot items
        {
            shared_ptr<Item> pitem = pslot.item();
            assert(pitem != nullptr);
            pslot.add(m_slot_hold.item());
            m_slot_hold.add(pitem);
        }
        else // Put item in slot
        {
            pslot->add(m_slot_hold.item());
            m_slot_hold.remove();
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Character::touch_container(Container* pcontainer, int x, int y)
{
    assert(pcontainer != nullptr);

    return;
}
