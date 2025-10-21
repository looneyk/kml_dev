#if !defined(QUETZAL_ROGUELIKE_FEATURESOLID_HPP)
#define QUETZAL_ROGUELIKE_FEATURESOLID_HPP
//------------------------------------------------------------------------------
// FeatureSolid.hpp
//------------------------------------------------------------------------------

#include "Feature.hpp"
#include "FactoryRegistering.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class FeatureSolid : public Feature, public FactoryRegistering<id_type, Feature, FeatureSolid, id_type, id_type>
    {
    public:

        FeatureSolid(id_type template_id, id_type id);
        FeatureSolid(const FeatureSolid&) = default;
        virtual ~FeatureSolid() = default;

        FeatureSolid& operator=(const FeatureSolid&) = default;

        static id_type factory_id();

        virtual bool blocked() const;
        virtual bool visible() const;

    private:

        Wall m_walls[6]; // east, north, west, south, top, bottom
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_FEATURESOLID_HPP
