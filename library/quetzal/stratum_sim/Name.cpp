//------------------------------------------------------------------------------
// stratum_sim
// Name.cpp
//------------------------------------------------------------------------------

#include "Name.hpp"
#include <algorithm>
#include <cctype>
#include <string>

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

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::zone_name(std::string_view pattern, const ZonePosition& position)
{
    std::string name;

    for (size_t i = 0; i < pattern.size(); ++i)
    {
        std::string_view rest = pattern.substr(i);
        if (rest.starts_with("{x}") || rest.starts_with("{y}") || rest.starts_with("{z}"))
        {
            name += std::to_string(rest[1] == 'x' ? position.x : rest[1] == 'y' ? position.y : position.z);
            i += 2;
        }
        else
        {
            name += pattern[i];
        }
    }

    return name;
}
