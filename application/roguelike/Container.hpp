#if !defined(QUETZAL_ROGUELIKE_CONTAINER_HPP)
#define QUETZAL_ROGUELIKE_CONTAINER_HPP
//------------------------------------------------------------------------------
// roguelike
// Container.hpp
//------------------------------------------------------------------------------

#include "Item.hpp"
#include "Items.hpp"
#include <map>
#include <memory>

namespace quetzal
{

namespace roguelike
{

    class Item;

    //--------------------------------------------------------------------------
    class Container : public Item
    {
    public:

        Container();
        Container(const Container&) = default;
        virtual ~Container() = default;

        Container& operator=(const Container&) = default;

        virtual float weight() const;
        Items& contents();

    private:

        Items m_items;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_CONTAINER_HPP
