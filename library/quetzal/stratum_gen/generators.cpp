//------------------------------------------------------------------------------
// stratum_gen
// generators.cpp
//------------------------------------------------------------------------------

#include "generators.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/Parameters.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include <type_traits>

static_assert(std::is_same_v<quetzal::stratum_gen::level_type, quetzal::dungen::level_type>);
static_assert(std::is_same_v<quetzal::stratum_gen::seed_type, quetzal::dungen::Parameters::seed_type>);

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::stratum_gen::generate_dungen(const ZoneRequest& request)
{
    dungen::Parameters parameters;
    parameters.level = request.level;
    parameters.seed = request.seed;

    return dungen::make_document(dungen::generate(0, request.name, parameters), parameters);
}

//------------------------------------------------------------------------------
std::map<std::string, quetzal::stratum_gen::generator_function_type, std::less<>> quetzal::stratum_gen::generators()
{
    return {{std::string(dungen_generator_name), generate_dungen}};
}
