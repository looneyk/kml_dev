//------------------------------------------------------------------------------
// stratum_format
// references.cpp
//------------------------------------------------------------------------------

#include "references.hpp"

namespace
{

    using namespace quetzal::stratum_format;

    constexpr const char* entity_key = "entity";

    //--------------------------------------------------------------------------
    void add_references(const raw_values_type& v, std::vector<stored_entity_id_type>& ids)
    {
        if (std::optional<stored_entity_id_type> id = reference_id(v); id.has_value())
        {
            ids.push_back(*id);
            return;
        }

        if (v.is_object() || v.is_array())
        {
            for (const raw_values_type& value : v)
            {
                add_references(value, ids);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void translate_value(raw_values_type& v, const std::function<stored_entity_id_type(stored_entity_id_type)>& translate)
    {
        if (std::optional<stored_entity_id_type> id = reference_id(v); id.has_value())
        {
            v = make_reference(translate(*id));
            return;
        }

        if (v.is_object() || v.is_array())
        {
            for (raw_values_type& value : v)
            {
                translate_value(value, translate);
            }
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::raw_values_type quetzal::stratum_format::make_reference(stored_entity_id_type id)
{
    raw_values_type reference = raw_values_type::object();
    reference[entity_key] = id;
    return reference;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::stored_entity_id_type> quetzal::stratum_format::reference_id(const raw_values_type& v)
{
    if (!v.is_object() || v.size() != 1)
    {
        return std::nullopt;
    }

    auto i = v.find(entity_key);
    if (i == v.end() || !i->is_number_unsigned())
    {
        return std::nullopt;
    }

    return i->get<stored_entity_id_type>();
}

//------------------------------------------------------------------------------
std::vector<quetzal::stratum_format::stored_entity_id_type> quetzal::stratum_format::references(const raw_values_type& components)
{
    std::vector<stored_entity_id_type> ids;
    add_references(components, ids);
    return ids;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::translate_references(raw_values_type& components, const std::function<stored_entity_id_type(stored_entity_id_type)>& translate)
{
    translate_value(components, translate);
    return;
}
