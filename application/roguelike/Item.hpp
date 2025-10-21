#if !defined(QUETZAL_ROGUELIKE_ITEM_HPP)
#define QUETZAL_ROGUELIKE_ITEM_HPP
//------------------------------------------------------------------------------
// roguelike
// Item.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "PropertyTarget.hpp"
#include "Items.hpp"

namespace quetzal
{

namespace roguelike
{

    class Slot;

    //--------------------------------------------------------------------------
    class Item : public PropertyTarget
    {
    public:

        Item();
        Item(const Item&) = default;
        virtual ~Item() = default;

        Item& operator=(const Item&) = default;

        id_type id() const;
        Item* parent();
        // position ...
        // orientation ...
//        std::string slot_types() const; // compatible slot types
//        Slot* slot();
        bool quantified() const;
        int quantity() const;
        virtual float weight() const;
        bool activated() const; // state ...

        void set_quantity(int n);
        virtual void activate();

    private:

        id_type m_id;
        Item* m_pparent;
//        Slot* m_pslot; // If this is an inventory item loaded into a slot (always populated except for cell items?)

        float m_weight;
        bool m_bQuantified;
        int m_quantity;
        bool m_bActivated; // state ...

        // event handler code, activation code ...
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_ITEM_HPP
