//------------------------------------------------------------------------------
// stratum_sim
// Name.cpp
//------------------------------------------------------------------------------

#include "Name.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace
{

    //--------------------------------------------------------------------------
    // Reflects the component, whose one member is name, as a string
    template<typename T>
    void register_string(flecs::world& entities)
    {
        entities.component<T>()
            .opaque(flecs::String)
            .serialize([](const flecs::serializer* s, const T* data)
            {
                const char* name = data->name.c_str();
                return s->value(flecs::String, &name);
            })
            .assign_string([](T* data, const char* value)
            {
                data->name = value;
            });

        return;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_names(flecs::world& entities)
{
    register_string<Name>(entities);
    register_string<DisplayName>(entities);

    entities.observer<const Name>()
        .event(flecs::OnSet)
        .each([](flecs::entity e, const Name& name)
        {
            if (e.has<DisplayName>())
            {
                return;
            }

            std::string capitals = name.name;
            std::ranges::transform(capitals, capitals.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            e.set<DisplayName>(DisplayName {std::move(capitals)});
        });

    return;
}
