//------------------------------------------------------------------------------
// stratum_sim
// Name.cpp
//------------------------------------------------------------------------------

#include "Name.hpp"
#include "quetzal/expression/Parser.hpp"
#include "quetzal/log/Log.hpp"
#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
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
std::string quetzal::stratum_sim::zone_name(std::string_view pattern, std::string_view region_name, const ZonePosition& position)
{
    auto fail = [pattern](std::string_view reason)
    {
        const std::string message = std::format("the zone name pattern '{}' {}", pattern, reason);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    };

    const expression::SymbolTable symbols {{"region_name", std::string(region_name)}, {"x", position.x}, {"y", position.y}, {"z", position.z}};
    expression::Parser parser;

    std::string name;
    size_t i = 0;

    for (size_t open = pattern.find('{'); open != std::string_view::npos; open = pattern.find('{', i))
    {
        name += pattern.substr(i, open - i);

        const size_t close = pattern.find('}', open);
        if (close == std::string_view::npos)
        {
            fail("has a { with no }");
        }

        const std::string placeholder(pattern.substr(open + 1, close - open - 1));
        const std::optional<expression::Expression> parsed = parser.parse(placeholder, symbols);
        if (!parsed.has_value())
        {
            fail(std::format("has {{{}}}, which is not an expression of region_name, x, y, and z", placeholder));
        }

        // parsed was just checked
        name += parsed->node().type == expression::DataType::String ? parsed->evaluate<std::string>(symbols)
            : std::to_string(parsed->evaluate<coord_type>(symbols));

        i = close + 1;
    }

    name += pattern.substr(i);
    return name;
}
