//------------------------------------------------------------------------------
// stratum_format
// Document.cpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <array>
#include <utility>
#include <cassert>

namespace
{

    using quetzal::stratum_format::Encoding;
    using quetzal::stratum_format::FlatRole;
    using quetzal::stratum_format::Notation;
    using quetzal::stratum_format::Relation;

    constexpr std::array<std::pair<Notation, std::string_view>, 2> notation_names
    {{
        {Notation::Block, "block"},
        {Notation::Wall, "wall"}
    }};

    constexpr std::array<std::pair<Encoding, std::string_view>, 2> encoding_names
    {{
        {Encoding::Characters, "characters"},
        {Encoding::Runs, "runs"}
    }};

    constexpr std::array<std::pair<FlatRole, std::string_view>, 2> flat_role_names
    {{
        {FlatRole::Floor, "floor"},
        {FlatRole::Ceiling, "ceiling"}
    }};

    constexpr std::array<std::pair<Relation, std::string_view>, 2> relation_names
    {{
        {Relation::In, "in"},
        {Relation::On, "on"}
    }};

    template<typename T, size_t N>
    std::string_view value_name(const std::array<std::pair<T, std::string_view>, N>& names, T value)
    {
        for (const auto& [v, s] : names)
        {
            if (v == value)
            {
                return s;
            }
        }

        assert(false); // Every value of the enumeration is named
        return {};
    }

    template<typename T, size_t N>
    std::optional<T> named_value(const std::array<std::pair<T, std::string_view>, N>& names, std::string_view name)
    {
        for (const auto& [v, s] : names)
        {
            if (s == name)
            {
                return v;
            }
        }

        return std::nullopt;
    }

} // namespace

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_format::name(Notation notation)
{
    return value_name(notation_names, notation);
}

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_format::name(Encoding encoding)
{
    return value_name(encoding_names, encoding);
}

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_format::name(FlatRole role)
{
    return value_name(flat_role_names, role);
}

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_format::name(Relation relation)
{
    return value_name(relation_names, relation);
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::Notation> quetzal::stratum_format::to_notation(std::string_view name)
{
    return named_value(notation_names, name);
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::Encoding> quetzal::stratum_format::to_encoding(std::string_view name)
{
    return named_value(encoding_names, name);
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::FlatRole> quetzal::stratum_format::to_flat_role(std::string_view name)
{
    return named_value(flat_role_names, name);
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::Relation> quetzal::stratum_format::to_relation(std::string_view name)
{
    return named_value(relation_names, name);
}
