//------------------------------------------------------------------------------
// stratum_sim
// DisplayName.cpp
//------------------------------------------------------------------------------

#include "DisplayName.hpp"

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_display_name(flecs::world& entities)
{
    entities.component<DisplayName>()
        .opaque(flecs::String)
        .serialize([](const flecs::serializer* s, const DisplayName* data)
        {
            const char* name = data->name.c_str();
            return s->value(flecs::String, &name);
        })
        .assign_string([](DisplayName* data, const char* value)
        {
            data->name = value;
        });

    return;
}
