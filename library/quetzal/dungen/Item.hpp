#if !defined(QUETZAL_DUNGEN_ITEM_HPP)
#define QUETZAL_DUNGEN_ITEM_HPP
//------------------------------------------------------------------------------
// dungen
// Item.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>

namespace quetzal::dungen
{

// probably want to use ray instead of point, point is then the closest intersection of ray with bounds ...
// why bother with ancestor/descendants then? just check each item in scene ...

    //--------------------------------------------------------------------------
    class Item
    {
    public:

        using value_type = float;
        using traits_type = math::CartesianTraits<value_type, 3>;
        using point_type = geometry::Point<traits_type>;
        using items_type = std::map<int, Item>; // z-order, should be item_id ...
        using action_handler_type = std::function<bool(Item&, const std::string&, point_type)>;
        using actions_type = std::map<std::string, action_handler_type>;
        using bounds_type = geometry::AxisAlignedBoundingBox<traits_type>;

        Item();
        Item(const std::optional<Item>& item_ancestor, bool cascade = true, bool single = true);
        Item(const std::optional<Item>& item_ancestor, const std::optional<bounds_type>& bounds, bool cascade = true, bool single = true);
        Item(const Item&) = default;
        Item(Item&&) = default;
        virtual ~Item() = default;

        Item& operator=(const Item&) = default;
        Item& operator=(Item&&) = default;

        const std::optional<Item>& ancestor() const;
        void set_ancestor(const Item& item_ancestor);

        const items_type descendants() const;
        items_type descendants();

        const std::optional<bounds_type>& bounds() const;
        void set_bounds(const bounds_type& bounds);

        void set_action(const std::string& action, action_handler_type handler);
        void set_applied(action_handler_type handler);

        virtual bool intersects(point_type point); // test against bounds, whatever they are
        virtual bool apply(const std::string& action, point_type point);
        virtual bool apply_self(const std::string& action, point_type point); // check if action applies and if so, apply it
        virtual void applied_descendant(const std::string& action, point_type point);
        virtual void print(std::ostream& os) const;

    private:

        std::optional<Item> m_item_ancestor;
        items_type m_items_descendant;
        bool m_cascade;
        bool m_single; // Only first z-order intersection considered

        std::optional<bounds_type> m_bounds;
        actions_type m_actions;
        action_handler_type m_applied;
    };

    std::ostream& operator<<(std::ostream& os, const Item& item);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_ITEM_HPP
