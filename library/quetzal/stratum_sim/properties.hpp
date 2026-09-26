#if !defined(QUETZAL_STRATUM_SIM_PROPERTIES_HPP)
#define QUETZAL_STRATUM_SIM_PROPERTIES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// properties.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <optional>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // On the reflection type of a std::optional member: the reflection type of its value, and access to that value
    struct OptionalField
    {
        flecs::entity_t type = 0;
        void* (*emplace)(void* optional) = nullptr; // Sets a default constructed value and returns it
        const void* (*value)(const void* optional) = nullptr; // Null where it is unset
    };

    //--------------------------------------------------------------------------
    // Registers std::optional<T> for reflection as a T, with its OptionalField. T must be registered already
    template<typename T>
    void register_optional(flecs::world& world);

    //--------------------------------------------------------------------------
    // Fills each optional member of into that is unset from the same member of from, and does the same within each struct
    // member. type is the reflection type of both
    void merge_unset(const flecs::world& world, flecs::entity_t type, void* into, const void* from);

    //--------------------------------------------------------------------------
    // Whether the struct has an optional member, or a struct member that has one, which makes a component one that inherits
    bool has_optional_fields(const flecs::world& world, flecs::entity_t type);

    // Makes a component with optional fields inherit: an entity IsA a type reads the type's value rather than a copy of it,
    // which is what flecs otherwise gives an entity when it becomes one. A component without them is left as it is
    void register_inheritance(flecs::world& world, flecs::entity_t component);

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_sim::register_optional(flecs::world& world)
{
    world.component<OptionalField>();

    world.component<std::optional<T>>()
        .opaque(static_cast<flecs::entity_t>(world.id<T>()))
        .serialize([](const flecs::serializer* s, const std::optional<T>* data)
        {
            return data->has_value() ? s->value(**data) : 0;
        });

    world.component<std::optional<T>>().template set<OptionalField>(OptionalField {world.id<T>(),
        [](void* optional) -> void* { return &static_cast<std::optional<T>*>(optional)->emplace(); },
        [](const void* optional) -> const void*
        {
            const auto* p = static_cast<const std::optional<T>*>(optional);
            return p->has_value() ? &**p : nullptr;
        }});

    return;
}

#endif // QUETZAL_STRATUM_SIM_PROPERTIES_HPP
