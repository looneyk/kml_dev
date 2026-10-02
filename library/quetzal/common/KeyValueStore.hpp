#if !defined(QUETZAL_KEYVALUESTORE_HPP)
#define QUETZAL_KEYVALUESTORE_HPP
//------------------------------------------------------------------------------
// quetzal common
// KeyValueStore.hpp
//------------------------------------------------------------------------------

#include "string_util.hpp"
#include <filesystem>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace quetzal
{

    //--------------------------------------------------------------------------
    // A map from string keys to string values, in key order. Its text form is a line of key = value per entry, with
    // blank lines and lines beginning with # or ; skipped; a value's leading and trailing whitespace is not kept
    class KeyValueStore
    {
    public:

        using map_type = std::map<std::string, std::string, std::less<>>;
        using const_iterator = map_type::const_iterator;

        // In the list/span constructors, later values with the same key overwrite earlier values

        KeyValueStore() = default;

        // Throws std::runtime_error if the file cannot be opened, or for the first line that is not in the text form
        KeyValueStore(const std::filesystem::path& path);

        KeyValueStore(std::initializer_list<map_type::value_type> key_values);
        KeyValueStore(std::span<const map_type::value_type> key_values);
        KeyValueStore(const KeyValueStore&) = default;
        KeyValueStore(KeyValueStore&&) = default;
        ~KeyValueStore() = default;

        KeyValueStore& operator=(const KeyValueStore&) = default;
        KeyValueStore& operator=(KeyValueStore&&) = default;

        KeyValueStore& operator|=(const KeyValueStore& rhs);

        const_iterator begin() const;
        const_iterator end() const;

        bool contains(std::string_view key) const;

        std::optional<std::string> value(std::string_view key) const;
        std::string value_or(std::string_view key, const std::string& default_value) const;

        template<typename T>
        std::optional<T> value(std::string_view key) const;

        template<typename T>
        T value_or(std::string_view key, const T& default_value) const;

        // Overwriting insertion replaces any existing value
        void insert(const map_type::value_type& key_value);
        void insert(const std::string& key, const std::string& value);

        void remove(std::string_view key);

    private:

        map_type m_values;
    };

    KeyValueStore operator|(KeyValueStore lhs, const KeyValueStore& rhs);

    std::ostream& operator<<(std::ostream& os, const KeyValueStore& key_value_store);

    // Reads to the end of the stream, and sets failbit at the first line that is not in the text form
    std::istream& operator>>(std::istream& is, KeyValueStore& key_value_store);

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename T>
std::optional<T> quetzal::KeyValueStore::value(std::string_view key) const
{
    auto i = m_values.find(key);
    if (i != m_values.end())
    {
        auto t = try_type<T>(i->second);
        if (!t)
        {
            throw std::runtime_error{std::format("Type conversion error with key: {}, value: {}", i->first, i->second)};
        }
        return t;
    }
    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::KeyValueStore::value_or(std::string_view key, const T& default_value) const
{
    return value<T>(key).value_or(default_value);
}

#endif // QUETZAL_KEYVALUESTORE_HPP
