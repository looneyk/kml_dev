#if !defined(QUETZAL_ROGUELIKE_ITEMTEMPLATE_HPP)
#define QUETZAL_ROGUELIKE_ITEMTEMPLATE_HPP
//------------------------------------------------------------------------------
// ItemTemplate
//------------------------------------------------------------------------------

#include <vector>
#include "types.hpp"
#include "PropertyTarget.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class ItemTemplate : public PropertyTarget
    {
    public:

        ItemTemplate();
        ItemTemplate(const ItemTemplate&) = default;
        ~ItemTemplate() = default;

        ItemTemplate& operator=(const ItemTemplate&) = default;

        std::string slot_types() const;
        bool quantified() const;
        bool container() const;

    private:

        id_type m_id;
        std::string m_type; // for grouping (armor, weapon, potion, food, ...) or just leave it to scripts; this should be a default script for the common types
        std::string m_slot_types;
        bool m_bQuantified;
        int m_nItemsSlotCapacity;
        bool m_bContainer;
        std::vector<id_type> m_events; // list of supported events
        // PropertyTarget holds list of supported properties and default values, fallback or for initialization?

        // allowable positions (coordinate constraints)
        // allowable orientations (transformation matrix constraints)
        // allowable states (map<id_type, Geometry>)
        // geometry/imagery by state
        // initialization code
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_ITEMTEMPLATE_HPP
