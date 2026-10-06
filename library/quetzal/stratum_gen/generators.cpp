//------------------------------------------------------------------------------
// stratum_gen
// generators.cpp
//------------------------------------------------------------------------------

#include "generators.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/Parameters.hpp"
#include "quetzal/dungen/write_zone.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::stratum_gen::generate_dungen(const stratum_sim::ZoneSpec& zone_spec)
{
    dungen::Parameters parameters;
    parameters.level = zone_spec.value<dungen::level_type>(stratum_sim::level_key);
    parameters.seed = dungen::random_seed();

    std::string name = zone_spec.value_or(name_key, std::string());
    return dungen::make_document(dungen::generate(0, name, parameters), parameters);
}

//------------------------------------------------------------------------------
void quetzal::stratum_gen::register_generators(stratum_sim::Generation& generation)
{
    generation.m_generators[std::string(dungen_generator_name)] = generate_dungen;
    return;
}
