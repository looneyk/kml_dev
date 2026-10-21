//------------------------------------------------------------------------------
// quetzal common
// KeyValueStore.cpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"
#include "file_util.hpp"
#include <format>
#include <fstream>
#include <istream>
#include <ostream>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    string_view trim(string_view text)
    {
        const size_t first = text.find_first_not_of(" \t");
        if (first == string_view::npos)
        {
            return {};
        }

        return text.substr(first, text.find_last_not_of(" \t") - first + 1);
    }

    //--------------------------------------------------------------------------
    // Returns the number of the first line that is not in the text form
    optional<size_t> read_key_values(istream& is, quetzal::KeyValueStore& key_value_store)
    {
        string line;
        size_t line_number = 0;
        while (quetzal::read_line(is, line))
        {
            ++line_number;

            const string_view text = trim(line);
            if (text.empty() || text.front() == '#' || text.front() == ';')
            {
                continue;
            }

            const size_t position = text.find('=');
            if (position == string_view::npos)
            {
                return line_number;
            }

            const string_view key = trim(text.substr(0, position));
            if (key.empty())
            {
                return line_number;
            }

            key_value_store.insert(string(key), string(trim(text.substr(position + 1))));
        }

        return nullopt;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::KeyValueStore::KeyValueStore(const filesystem::path& path)
{
    ifstream file(path, ios_base::binary);
    if (!file.is_open())
    {
        throw runtime_error(format("Cannot open {}", path.string()));
    }

    if (const optional<size_t> line_number = read_key_values(file, *this))
    {
        throw runtime_error(format("{}({}): expected key = value", path.string(), *line_number));
    }
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore::KeyValueStore(std::initializer_list<map_type::value_type> key_values)
{
    for (const auto& key_value : key_values)
    {
        insert(key_value);
    }
    return;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore::KeyValueStore(std::span<const map_type::value_type> key_values)
{
    for (const auto& key_value : key_values)
    {
        insert(key_value);
    }
    return;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore& quetzal::KeyValueStore::operator|=(const KeyValueStore& rhs)
{
    for (const auto& property : rhs)
    {
        insert(property);
    }

    return *this;
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
bool quetzal::KeyValueStore::contains(string_view key) const
{
    return m_values.contains(key);
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::KeyValueStore::value(string_view key) const
{
    auto i = m_values.find(key);
    return i != m_values.end() ? make_optional(i->second) : std::nullopt;
}

//------------------------------------------------------------------------------
std::string quetzal::KeyValueStore::value_or(string_view key, const std::string& default_value) const
{
    auto i = m_values.find(key);
    return i != m_values.end() ? i->second : default_value;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::insert(const map_type::value_type& key_value)
{
    insert(key_value.first, key_value.second);
    return;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::insert(const string& key, const string& value)
{
    m_values[key] = value;
    return;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::remove(string_view key)
{
    auto i = m_values.find(key);
    if (i != m_values.end())
    {
        m_values.erase(i);
    }
    return;
}

//------------------------------------------------------------------------------
void quetzal::KeyValueStore::clear()
{
    m_values.clear();
    return;
}

//------------------------------------------------------------------------------
bool quetzal::KeyValueStore::empty() const
{
    return m_values.empty();
}

//------------------------------------------------------------------------------
size_t quetzal::KeyValueStore::size() const
{
    return m_values.size();
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::operator|(KeyValueStore lhs, const KeyValueStore& rhs)
{
    return lhs |= rhs;
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const KeyValueStore& key_value_store)
{
    for (const auto& [key, value] : key_value_store)
    {
        os << key << " = " << value << '\n';
    }

    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, KeyValueStore& key_value_store)
{
    if (read_key_values(is, key_value_store))
    {
        is.setstate(ios_base::failbit);
        return is;
    }

    is.clear(ios_base::eofbit);
    return is;
}
