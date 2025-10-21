#if !defined(QUETZAL_PROPERTY_PROPERTY_HPP)
#define QUETZAL_PROPERTY_PROPERTY_HPP
//------------------------------------------------------------------------------
// Property.hpp
//
//------------------------------------------------------------------------------

#include <string>

namespace quetzal
{

namespace roguelike
{

    //----------------------------------------------------------------------
    class Property
    {
    public:

        Property(const std::string& name, const std::string& value = std::string());
        Property(const Property&) = default;
        virtual ~Property();

        Property& operator=(const Property&) = default;

        std::string name() const;
        std::string value() const;

        void set_name(const std::string& name);
        void set_value(const std::string& value);

    private:

        std::string m_name;
        std::string m_value;
    };

} // namespace roguelike

} // namespace quetzal

#endif // QUETZAL_PROPERTY_PROPERTY_HPP
