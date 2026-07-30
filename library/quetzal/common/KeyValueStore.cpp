//------------------------------------------------------------------------------
// quetzal common
// KeyValueStore.cpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"
#include "file_util.hpp"
#include <fstream>
#include <sstream>

using namespace std;

namespace quetzal
{

//------------------------------------------------------------------------------
    KeyValueStore::KeyValueStore(std::initializer_list<map_type::value_type> key_values)
    {
        m_values.insert(key_values);
        return;
    }

//------------------------------------------------------------------------------
    KeyValueStore::KeyValueStore(std::span<const typename map_type::value_type> key_values)
    {
        m_values.insert_range(key_values);
        return;
    }

//------------------------------------------------------------------------------
KeyValueStore::KeyValueStore(const filesystem::path& path)
{
    ifstream file(path, ios_base::binary);
    if (!file.is_open())
    {
        return;
    }

    string line;
    while (read_line(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // Find the equals sign
        size_t pos = line.find('=');
        if (pos == string::npos)
        {
            continue;
        }

        // Extract key and value, trimming whitespace
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        // Trim leading whitespace from key
        size_t start = key.find_first_not_of(" \t\r\n");
        if (start == string::npos)
        {
            continue;
        }
        key = key.substr(start);

        // Trim trailing whitespace from key
        size_t end = key.find_last_not_of(" \t\r\n");
        if (end != string::npos)
        {
            key = key.substr(0, end + 1);
        }

        // Trim leading whitespace from value
        start = value.find_first_not_of(" \t\r\n");
        if (start != string::npos)
        {
            value = value.substr(start);
        }
        else
        {
            value.clear();
        }

        // Trim trailing whitespace from value
        end = value.find_last_not_of(" \t\r\n");
        if (end != string::npos)
        {
            value = value.substr(0, end + 1);
        }

        if (!key.empty())
        {
            m_values[key] = value;
        }
    }
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore::const_iterator quetzal::KeyValueStore::begin() const
{
    return m_values.begin();
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore::const_iterator quetzal::KeyValueStore::end() const
{
    return m_values.end();
}

//------------------------------------------------------------------------------
bool quetzal::KeyValueStore::contains(const string& key) const
{
    return m_values.contains(key);
}

//------------------------------------------------------------------------------
optional<string> quetzal::KeyValueStore::value(const string& key) const
{
    auto it = m_values.find(key);
    return it != m_values.end() ? make_optional(it->second) : nullopt;
}

//------------------------------------------------------------------------------
string quetzal::KeyValueStore::value_or(const string& key, const string& default_value) const
{
    auto it = m_values.find(key);
    return it != m_values.end() ? it->second : default_value;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::insert(const map_type::value_type& key_value)
{
    m_values.insert(key_value);
    return;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::insert(const string& key, const string& value)
{
    m_values[key] = value;
    return;
}

//------------------------------------------------------------------------------
void KeyValueStore::remove(const string& key)
{
    m_values.erase(key);
    return;
}

} // namespace quetzal
