#if !defined(QUETZAL_PROPERTY_PROPERTYTARGET_HPP)
#define QUETZAL_PROPERTY_PROPERTYTARGET_HPP
//------------------------------------------------------------------------------
// PropertyTarget.hpp
//
//------------------------------------------------------------------------------

#include <map>
#include <memory>
#include "Property.hpp"

namespace quetzal
{

namespace roguelike
{

    //----------------------------------------------------------------------
    class PropertyTarget
    {
    public:

        typedef std::map<std::string, std::shared_ptr<Property>> Properties; // std::string is Property name

        PropertyTarget();
        PropertyTarget(const PropertyTarget&) = default;
        virtual ~PropertyTarget() = default;

        PropertyTarget& operator=(const PropertyTarget&) = default;

        std::string property(const std::string& name) const;
        void set_property(const std::string& name, const std::string& value);

        void register_property(const std::string& name, const std::string& value = std::string());
        void remove_property(const std::string& name);
        void clear_properties();

    private:

        Properties m_properties;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_PROPERTY_PROPERTYTARGET_HPP
