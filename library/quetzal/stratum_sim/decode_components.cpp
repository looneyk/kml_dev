//------------------------------------------------------------------------------
// stratum_sim
// decode_components.cpp
//------------------------------------------------------------------------------

#include "decode_components.hpp"
#include "file_components.hpp"
#include "properties.hpp"
#include "quetzal/stratum_format/references.hpp"
#include <cstdint>
#include <format>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // Sets the field at the cursor from a value in the file, resolving each reference. If the value does not fit the field,
    // returns its path and the reason
    std::optional<std::string> decode_value(const flecs::world& world, ecs_meta_cursor_t& cursor, const quetzal::stratum_format::raw_values_type& value,
        const std::string& path, const quetzal::stratum_sim::resolve_function_type& resolve)
    {
        using namespace quetzal::stratum_sim;

        const ecs_world_t* w = world.c_ptr();
        ecs_entity_t type = ecs_meta_get_type(&cursor);

        if (const auto* optional_field = static_cast<const OptionalField*>(ecs_get_id(w, type, world.id<OptionalField>())))
        {
            ecs_meta_cursor_t value_cursor = ecs_meta_cursor(w, optional_field->type, optional_field->emplace(ecs_meta_get_ptr(&cursor)));
            return decode_value(world, value_cursor, value, path, resolve);
        }

        if (type == world.id<EntityReference>())
        {
            std::optional<quetzal::stratum_format::stored_entity_id_type> id = quetzal::stratum_format::reference_id(value);
            if (!id.has_value())
            {
                return std::format("{}: not a reference to an entity", path);
            }

            *static_cast<EntityReference*>(ecs_meta_get_ptr(&cursor)) = resolve(*id);
            return {};
        }

        if (value.is_object())
        {
            if (ecs_meta_push(&cursor) != 0)
            {
                return std::format("{}: an object, which its field is not", path);
            }

            for (const auto& [key, member] : value.items())
            {
                std::string member_path = std::format("{}.{}", path, key);

                if (ecs_meta_try_member(&cursor, key.c_str()) != 0)
                {
                    return std::format("{}: not a field of the component", member_path);
                }

                if (std::optional<std::string> error = decode_value(world, cursor, member, member_path, resolve))
                {
                    return error;
                }
            }

            ecs_meta_pop(&cursor);
            return {};
        }

        if (value.is_array())
        {
            if (ecs_meta_push(&cursor) != 0)
            {
                return std::format("{}: an array, which its field is not", path);
            }

            for (size_t i = 0; i < value.size(); ++i)
            {
                std::string element_path = std::format("{}[{}]", path, i);

                if (i > 0 && ecs_meta_next(&cursor) != 0)
                {
                    return std::format("{}: past the end of its field", element_path);
                }

                if (std::optional<std::string> error = decode_value(world, cursor, value[i], element_path, resolve))
                {
                    return error;
                }
            }

            ecs_meta_pop(&cursor);
            return {};
        }

        int result = value.is_boolean() ? ecs_meta_set_bool(&cursor, value.get<bool>())
            : value.is_number_unsigned() ? ecs_meta_set_uint(&cursor, value.get<std::uint64_t>())
            : value.is_number_integer() ? ecs_meta_set_int(&cursor, value.get<std::int64_t>())
            : value.is_number_float() ? ecs_meta_set_float(&cursor, value.get<double>())
            : value.is_string() ? ecs_meta_set_string(&cursor, value.get_ref<const std::string&>().c_str())
            : ecs_meta_set_null(&cursor);

        return result == 0 ? std::nullopt : std::optional<std::string>(std::format("{}: a value its field cannot take", path));
    }

} // namespace

//------------------------------------------------------------------------------
std::vector<std::pair<std::string, const quetzal::stratum_format::raw_values_type*>> quetzal::stratum_sim::component_objects(
    const stratum_format::raw_values_type& components)
{
    std::vector<std::pair<std::string, const stratum_format::raw_values_type*>> objects;

    if (components.is_object())
    {
        for (const auto& [name, value] : components.items())
        {
            objects.emplace_back(name, &value);
        }
    }

    return objects;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::component_path(const std::string& path, const std::string& name)
{
    return std::format("{}.components.{}", path, name);
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::check_decodable(const flecs::world& world, const stratum_format::raw_values_type& components,
    const std::string& path)
{
    auto unbound = [](stratum_format::stored_entity_id_type id) { return EntityReference {PersistentId {id}, flecs::entity()}; };

    for (const auto& [name, value] : component_objects(components))
    {
        flecs::entity_t component = file_component(world, name);
        if (component == 0)
        {
            return std::format("{}: no component is registered under the name", component_path(path, name));
        }

        if (is_tag(world, component))
        {
            if (!value->is_object() || !value->empty())
            {
                return std::format("{}: a tag, which is written as an empty object", component_path(path, name));
            }

            continue;
        }

        ecs_value_t decoded = ecs_value_new(world.c_ptr(), component);
        ecs_meta_cursor_t cursor = ecs_meta_cursor(world.c_ptr(), component, decoded.ptr);
        std::optional<std::string> error = decode_value(world, cursor, *value, component_path(path, name), unbound);
        ecs_value_fini(world.c_ptr(), &decoded);

        if (error.has_value())
        {
            return error;
        }
    }

    return {};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::decode_components(flecs::entity e, const stratum_format::raw_values_type& components, const std::string& path,
    const resolve_function_type& resolve)
{
    flecs::world world = e.world();
    ecs_world_t* w = world.c_ptr();

    for (const auto& [name, value] : component_objects(components))
    {
        flecs::entity_t component = file_component(world, name);
        assert(component != 0);

        if (is_tag(world, component))
        {
            e.add(component);
            continue;
        }

        size_t size = static_cast<size_t>(ecs_get_type_info(w, component)->size);

        if (ecs_has_pair(w, component, EcsOnInstantiate, EcsInherit))
        {
            ecs_value_t decoded = ecs_value_new(w, component);
            ecs_meta_cursor_t cursor = ecs_meta_cursor(w, component, decoded.ptr);

            [[maybe_unused]] std::optional<std::string> error = decode_value(world, cursor, *value, component_path(path, name), resolve);
            assert(!error.has_value());

            ecs_set_id(w, e, component, size, decoded.ptr);
            ecs_value_fini(w, &decoded);
        }
        else
        {
            ecs_meta_cursor_t cursor = ecs_meta_cursor(w, component, ecs_ensure_id(w, e, component, size));

            [[maybe_unused]] std::optional<std::string> error = decode_value(world, cursor, *value, component_path(path, name), resolve);
            assert(!error.has_value());

            ecs_modified_id(w, e, component);
        }
    }

    return;
}
