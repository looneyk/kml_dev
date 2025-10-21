#if !defined(QUETZAL_ROGUELIKE_SLOT_HPP)
#define QUETZAL_ROGUELIKE_SLOT_HPP
//------------------------------------------------------------------------------
// Slot.hpp
//------------------------------------------------------------------------------

#include <memory>
#include "Item.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class Slot
    {
    public:

        Slot(const std::string& type);
        Slot(const Slot&) = default;
        ~Slot() = default;

        Slot& operator=(const Slot&) = default;

        std::string type() const;
        bool holds(Item* pitem) const;

        bool empty() const;
        std::shared_ptr<Item> item();

        void add(std::shared_ptr<Item> pitem);
        void remove();

    private:

        std::string m_type;
        std::shared_ptr<Item> m_pitem;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_SLOT_HPP
