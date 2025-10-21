#if !defined(QUETZAL_CONFIG_HPP)
#define QUETZAL_CONFIG_HPP
//------------------------------------------------------------------------------
// quetzal common
// Config.hpp
//------------------------------------------------------------------------------

#include <map>
#include <string>

namespace quetzal
{

    class Config
    {
    public:

        using map_type = std::map<std::string, std::string>;

        Config() = default;
        Config(const Config&) = default;
        Config(Config&&) = default;
        ~Config() = default;

        Config& operator=(const Config&) = default;
        Config& operator=(Config&&) = default;

        std::string operator[](const std::string& key) const;

        void insert(const std::string& key, const std::string& value);

    private:

        map_type m_values;
    };

} // namespace quetzal

#endif // QUETZAL_CONFIG_HPP
