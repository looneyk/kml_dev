#if !defined(QUETZAL_ROGUELIKE_ITEMS_HPP)
#define QUETZAL_ROGUELIKE_ITEMS_HPP
//------------------------------------------------------------------------------
// roguelike
// Items.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <map>
#include <memory>

namespace quetzal
{

namespace roguelike
{

    class Item;

    //--------------------------------------------------------------------------
    class Items
    {
    public:

        Items();
        Items(const Items&) = default;
        ~Items() = default;

        Items& operator=(const Items&) = default;

        Item* parent();
        float weight() const;
        std::shared_ptr<Item> find(id_type id);
        void add(std::shared_ptr<Item> pitem, int quantity = 1);
        void remove(id_type id, int quantity = 1);

    private:

        Item* m_pitem;
        std::map<id_type, std::shared_ptr<Item>> m_items;
        float m_weight;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_ITEMS_HPP
