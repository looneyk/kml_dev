#if !defined(QUETZAL_KEYVALUESTORE_HPP)
#define QUETZAL_KEYVALUESTORE_HPP
//------------------------------------------------------------------------------
// quetzal common
// KeyValueStore.hpp
// Immutable (simply no non const functionality)
// Base class for Config, Properties (Settings, Options, etc.)
//------------------------------------------------------------------------------

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace quetzal
{

    class KeyValueStore
    {
    public:

        using map_type = std::unordered_map<std::string, std::string>;
        using const_iterator = map_type::const_iterator;

        KeyValueStore() = default;
        KeyValueStore(const std::filesystem::path& path);
        KeyValueStore(const KeyValueStore&) = default;
        KeyValueStore(KeyValueStore&&) = default;
        ~KeyValueStore() = default;

        KeyValueStore& operator=(const KeyValueStore&) = default;
        KeyValueStore& operator=(KeyValueStore&&) = default;

        const_iterator begin() const;
        const_iterator end() const;

        bool contains(const std::string& key) const;

        std::optional<std::string> value(const std::string& key) const;
        std::string value_or(const std::string& key, const std::string& default_value) const;

        void insert(const typename map_type::value_type& key_value);
        void insert(const std::string& key, const std::string& value);
        void remove(const std::string& key);

    private:

        map_type m_values;
    };

} // namespace quetzal

#endif // QUETZAL_KEYVALUESTORE_HPP
