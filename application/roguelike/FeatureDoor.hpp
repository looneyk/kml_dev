#if !defined(QUETZAL_ROGUELIKE_FEATUREDOOR_HPP)
#define QUETZAL_ROGUELIKE_FEATUREDOOR_HPP
//------------------------------------------------------------------------------
// FeatureDoor.hpp
//------------------------------------------------------------------------------

#include "Feature.hpp"
#include "FactoryRegistering.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class FeatureDoor : public Feature, public FactoryRegistering<id_type, Feature, FeatureDoor, id_type, id_type>
    {
    public:

        FeatureDoor(id_type template_id, id_type id);
        FeatureDoor(const FeatureDoor&) = default;
        virtual ~FeatureDoor() = default;

        FeatureDoor& operator=(const FeatureDoor&) = default;

        static id_type factory_id();

        virtual bool blocked() const;
        virtual bool visible() const;

        virtual bool activated() const;
        void activate(bool b);

    private:

        bool m_bActivated; // open
        Wall m_walls[2]; // front, back
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_FEATUREDOOR_HPP
