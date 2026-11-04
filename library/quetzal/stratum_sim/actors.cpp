//------------------------------------------------------------------------------
// stratum_sim
// actors.cpp
//------------------------------------------------------------------------------

#include "actors.hpp"
#include "file_components.hpp"

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_actor_components(flecs::world& entities)
{
    entities.component<Blocking>();
    entities.component<ControlledActor>();
    entities.component<Dead>();
    entities.component<Speed>().member(&Speed::value, "value");

    register_file_component<Blocking>(entities, "blocking");
    register_file_component<Dead>(entities, "dead");
    register_file_component<Speed>(entities, "speed");

    return;
}
