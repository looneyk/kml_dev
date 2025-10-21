#if !defined(QUETZAL_ROGUELIKE_FEATURE_HPP)
#define QUETZAL_ROGUELIKE_FEATURE_HPP
//------------------------------------------------------------------------------
// Feature.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "FeatureTemplate.hpp"

namespace quetzal
{

namespace roguelike
{

    //--------------------------------------------------------------------------
    class Feature
    {
    public:

        Feature(id_type template_id, id_type id);
        Feature(const Feature&) = default;
        virtual ~Feature() = default;

        Feature& operator=(const Feature&) = default;

        id_type id() const;
        virtual bool blocked() const = 0;
        virtual bool visible() const = 0;

        // Null implementation for the majority of features
        virtual bool activated() const;
        void activate(bool b);

    private:

        id_type m_id;
        shared_ptr<FeatureTemplate> m_ptemplate;
    };

    typedef quetzal::SingletonHolder<Factory<id_type, Feature, id_type, id_type>> FeatureFactory; // or declaration here and definition with instance in cpp

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_ROGUELIKE_FEATURE_HPP
