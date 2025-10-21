#if !defined(QUETZAL_ROGUELIKE_CHARACTER_HPP)
#define QUETZAL_ROGUELIKE_CHARACTER_HPP
//------------------------------------------------------------------------------
// Character.hpp
//------------------------------------------------------------------------------

#include <map>
#include "PropertyTarget.hpp"
#include "Container.hpp"
#include "Slot.hpp"
#include "Level.hpp"
#include "DiscretePosition.hpp"
#include "DiscreteDirection.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class Character : public PropertyTarget
    {
    public:

        Character();
        Character(const Character&) = default;
        ~Character() = default;

        Character& operator=(const Character&) = default;

        const Container& inventory() const;
        Container& inventory();
        Slot* slot(const std::string& name);

        float encumbrance() const;
        float defense() const;

        void shift(int dx, int dy); // Move with no change in facing
        void move(int dx, int dy); // Change facing in direction of movement
        void move(); // One step in direction of facing
        void set_position(int x, int y); // Target position must be empty
        void set_facing(int x, int y);
        void rotate_left();
        void rotate_right();

//        void touch(...);
        void touch_item(std::shared_ptr<Item> pitem); // environment item
        void touch_slot(Slot* pslot); // equipment slot
        void touch_container(Container* pcontainer, int x, int y); // container block (includes inventory)

    private:

        // Armor slots
        Slot m_slot_head;
        Slot m_slot_hands;
        Slot m_slot_torso;
        Slot m_slot_legs;
        Slot m_slot_feet;
        // Weapon/shield slots
        Slot m_slot_hand_right;
        Slot m_slot_hand_left;
        // Ammo/potion slots
        Slot m_slot_quiver; 
        Slot m_slot_belt_right;
        Slot m_slot_belt_left;
        // Jewelry slots
        Slot m_slot_neck;
        Slot m_slot_finger_right;
        Slot m_slot_finger_left;
        // Inventory
        Slot m_slot_pack;
        // Holdable, moveable, placeable, ...
        Slot m_slot_hold;
        // Slots indexed by name
        std::map<std::string, Slot&> m_slots;

        Level* m_plevel;
        DiscretePosition m_position;
        DiscreteDirection m_facing;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_CHARACTER_HPP
