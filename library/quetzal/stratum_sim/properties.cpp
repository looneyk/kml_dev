//------------------------------------------------------------------------------
// stratum_sim
// properties.cpp
//------------------------------------------------------------------------------

#include "properties.hpp"
#include <span>
#include <cstring>

//------------------------------------------------------------------------------
void quetzal::stratum_sim::merge_unset(const flecs::world& world, flecs::entity_t type, void* into, const void* from)
{
    const ecs_world_t* w = world.c_ptr();

    const EcsStruct* struct_type = ecs_get(w, type, EcsStruct);
    if (struct_type == nullptr)
    {
        return;
    }

    flecs::entity_t optional_field_id = world.id<OptionalField>();

    for (const ecs_member_t& member : std::span(ecs_vec_first_t(&struct_type->members, ecs_member_t),
        static_cast<size_t>(ecs_vec_count(&struct_type->members))))
    {
        void* into_member = static_cast<char*>(into) + member.offset;
        const void* from_member = static_cast<const char*>(from) + member.offset;

        if (const auto* optional_field = static_cast<const OptionalField*>(ecs_get_id(w, member.type, optional_field_id)))
        {
            if (optional_field->value(into_member) == nullptr && optional_field->value(from_member) != nullptr)
            {
                const ecs_type_info_t* type_info = ecs_get_type_info(w, member.type);
                if (type_info->hooks.copy != nullptr)
                {
                    type_info->hooks.copy(into_member, from_member, 1, type_info);
                }
                else
                {
                    std::memcpy(into_member, from_member, static_cast<size_t>(type_info->size));
                }
            }
        }
        else if (member.count <= 1 && ecs_has(w, member.type, EcsStruct))
        {
            merge_unset(world, member.type, into_member, from_member);
        }
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::has_optional_fields(const flecs::world& world, flecs::entity_t type)
{
    const ecs_world_t* w = world.c_ptr();

    const EcsStruct* struct_type = ecs_get(w, type, EcsStruct);
    if (struct_type == nullptr)
    {
        return false;
    }

    flecs::entity_t optional_field_id = world.id<OptionalField>();

    for (const ecs_member_t& member : std::span(ecs_vec_first_t(&struct_type->members, ecs_member_t),
        static_cast<size_t>(ecs_vec_count(&struct_type->members))))
    {
        if (ecs_has_id(w, member.type, optional_field_id) || has_optional_fields(world, member.type))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_inheritance(flecs::world& world, flecs::entity_t component)
{
    if (has_optional_fields(world, component))
    {
        ecs_add_pair(world.c_ptr(), component, EcsOnInstantiate, EcsInherit);
    }

    return;
}
