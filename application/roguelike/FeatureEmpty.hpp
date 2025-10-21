#if !defined(QUETZAL_ROGUELIKE_FEATUREEMPTY_HPP)
#define QUETZAL_ROGUELIKE_FEATUREEMPTY_HPP
//------------------------------------------------------------------------------
// FeatureEmpty.hpp
//------------------------------------------------------------------------------

#include "Feature.hpp"
#include "FactoryRegistering.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class FeatureEmpty : public Feature, public FactoryRegistering<id_type, Feature, FeatureEmpty, id_type, id_type>
    {
    public:

        FeatureEmpty(id_type template_id, id_type id);
        FeatureEmpty(const FeatureEmpty&) = default;
        virtual ~FeatureEmpty() = default;

        FeatureEmpty& operator=(const FeatureEmpty&) = default;

        static id_type factory_id();

        virtual bool blocked() const;
        virtual bool visible() const;

    private:
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_FEATUREEMPTY_HPP
