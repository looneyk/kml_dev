#if !defined(QUETZAL_STRATUM_SIM_PROPERTIES_HPP)
#define QUETZAL_STRATUM_SIM_PROPERTIES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// properties.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <optional>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Set on the reflection type of a std::optional member. It has the reflection type of the optional's value and functions
    // that access the value
    struct OptionalField
    {
        flecs::entity_t type = 0;
        void* (*emplace)(void* optional) = nullptr; // Sets a default constructed value and returns it
        const void* (*value)(const void* optional) = nullptr; // Returns null if the optional is unset
    };

    //--------------------------------------------------------------------------
    // Registers std::optional<T> for reflection as a T, with its OptionalField. T must be registered already
    template<typename T>
    void register_optional(flecs::world& world);

    //--------------------------------------------------------------------------
    // Registers std::vector<T> for reflection as a list of T, which a zone file writes as an array. T must be registered
    // already
    template<typename T>
    void register_vector(flecs::world& world);

    //--------------------------------------------------------------------------
    // Sets each unset optional member of into from the same member of from, recursing into struct members. type is the
    // reflection type of both
    void merge_unset(const flecs::world& world, flecs::entity_t type, void* into, const void* from);

    //--------------------------------------------------------------------------
    // Returns true if the struct has an optional member, directly or in a struct member. A component that has one inherits
    bool has_optional_fields(const flecs::world& world, flecs::entity_t type);

    // Makes a component that has optional fields inherit, so an entity that IsA a type reads the type's value instead of the
    // copy flecs otherwise makes. A component without optional fields is left unchanged
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

    world.component<std::optional<T>>().template set<OptionalField>(OptionalField{world.id<T>(),
        [](void* optional) -> void* { return &static_cast<std::optional<T>*>(optional)->emplace(); },
        [](const void* optional) -> const void*
        {
            const auto* p = static_cast<const std::optional<T>*>(optional);
            return p->has_value() ? &**p : nullptr;
        }});

    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_sim::register_vector(flecs::world& world)
{
    world.component<std::vector<T>>().opaque([](flecs::world& w)
    {
        return flecs::opaque<std::vector<T>, T>()
            .as_type(w.vector<T>())
            .serialize([](const flecs::serializer* s, const std::vector<T>* data)
            {
                for (const T& element : *data)
                {
                    s->value(element);
                }

                return 0;
            })
            .count([](const std::vector<T>* data) { return data->size(); })
            .resize([](std::vector<T>* data, size_t size) { data->resize(size); })
            .ensure_element([](std::vector<T>* data, size_t slot) -> T*
            {
                if (data->size() <= slot)
                {
                    data->resize(slot + 1);
                }

                return &(*data)[slot];
            });
    });

    return;
}

#endif // QUETZAL_STRATUM_SIM_PROPERTIES_HPP
