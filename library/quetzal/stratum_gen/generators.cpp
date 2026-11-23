//------------------------------------------------------------------------------
// stratum_gen
// generators.cpp
//------------------------------------------------------------------------------

#include "generators.hpp"
#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/Parameters.hpp"
#include "quetzal/catagen/write_zone.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/Parameters.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include <type_traits>

static_assert(std::is_same_v<quetzal::stratum_gen::zone_position_type, quetzal::dungen::zone_position_type>);
static_assert(std::is_same_v<quetzal::stratum_gen::seed_type, quetzal::dungen::Parameters::seed_type>);
static_assert(std::is_same_v<quetzal::stratum_gen::zone_position_type, quetzal::catagen::zone_position_type>);
static_assert(std::is_same_v<quetzal::stratum_gen::seed_type, quetzal::catagen::Parameters::seed_type>);

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::stratum_gen::generate_dungen(const ZoneRequest& request)
{
    dungen::Parameters parameters;

    // Every generator is given the same parameters tokens, so each skips the keys that are not its parameters
    for (const auto& [key, text] : request.parameters)
    {
        dungen::set_parameter(parameters, key, text);
    }

    parameters.position = request.position;
    parameters.seed = request.seed;

    return dungen::make_document(dungen::generate(0, request.name, parameters), parameters);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::stratum_gen::generate_catagen(const ZoneRequest& request)
{
    catagen::Parameters parameters;

    // Every generator is given the same parameters tokens, so each skips the keys that are not its parameters
    for (const auto& [key, text] : request.parameters)
    {
        catagen::set_parameter(parameters, key, text);
    }

    parameters.position = request.position;
    parameters.seed = request.seed;

    return catagen::make_document(catagen::generate(request.name, parameters), parameters);
}

//------------------------------------------------------------------------------
std::map<std::string, quetzal::stratum_gen::generator_function_type, std::less<>> quetzal::stratum_gen::generators()
{
    return {{std::string(dungen_generator_name), generate_dungen}, {std::string(catagen_generator_name), generate_catagen}};
}

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_gen::generator_name(stratum_core::Notation notation)
{
    return notation == stratum_core::Notation::Block ? dungen_generator_name : catagen_generator_name;
}
