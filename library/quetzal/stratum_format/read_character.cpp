//------------------------------------------------------------------------------
// stratum_format
// read_character.cpp
//------------------------------------------------------------------------------

#include "read_character.hpp"
#include "checked_values.hpp"
#include "references.hpp"
#include "saved_game_format.hpp"
#include <cstddef>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    // The key of the character's name in the file's header, and of the name component that it is a copy of
    constexpr std::string_view name_key = "name";

    //--------------------------------------------------------------------------
    // Takes the file's header, the format, the version and the name, from the top level as a SAX parser reports them, and
    // stops the parse once it has all three, so the rest of the file is not parsed. Every other value is passed over
    class HeaderStringsHandler : public nlohmann::json_sax<raw_values_type>
    {
    public:

        bool null() override { return pass_over(); }
        bool boolean(bool) override { return pass_over(); }
        bool number_integer(number_integer_t) override { return pass_over(); }
        bool number_unsigned(number_unsigned_t) override { return pass_over(); }
        bool number_float(number_float_t, const string_t&) override { return pass_over(); }
        bool binary(binary_t&) override { return pass_over(); }

        bool string(string_t& value) override
        {
            if (m_depth == 1 && m_key != nullptr)
            {
                *m_key = value;
                m_key = nullptr;
            }

            return !has_all_header_values();
        }

        bool start_object(std::size_t) override
        {
            m_key = nullptr;
            ++m_depth;
            return true;
        }

        bool key(string_t& key) override
        {
            if (m_depth == 1)
            {
                m_key = key == "format" ? &format : key == "version" ? &version : key == name_key ? &name : nullptr;
            }

            return true;
        }

        bool end_object() override
        {
            --m_depth;
            return true;
        }

        bool start_array(std::size_t) override
        {
            m_key = nullptr;
            ++m_depth;
            return true;
        }

        bool end_array() override
        {
            --m_depth;
            return true;
        }

        bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& e) override
        {
            throw std::runtime_error(e.what());
        }

        bool has_all_header_values() const
        {
            return format.has_value() && version.has_value() && name.has_value();
        }

        std::optional<std::string> format;
        std::optional<std::string> version;
        std::optional<std::string> name;

    private:

        // A value of another type at a key the handler takes is left unset, for the caller to report
        bool pass_over()
        {
            m_key = nullptr;
            return true;
        }

        int m_depth = 0;
        std::optional<std::string>* m_key = nullptr;
    };

    //--------------------------------------------------------------------------
    std::string read_name(std::istream& is, const std::string& stream_name)
    {
        HeaderStringsHandler handler;

        try
        {
            raw_values_type::sax_parse(is, &handler);
        }
        catch (const std::runtime_error& e)
        {
            throw std::runtime_error(std::format("{}: {}", stream_name, e.what()));
        }

        auto required = [&](const std::optional<std::string>& value, std::string_view key, std::string_view required_value) -> const std::string&
        {
            if (!value.has_value())
            {
                throw std::runtime_error(std::format("{}: {}: required and absent, or not a string", stream_name, key));
            }

            if (!required_value.empty() && *value != required_value)
            {
                throw std::runtime_error(std::format("{}: {}: '{}' is not what this reads", stream_name, key, *value));
            }

            return *value;
        };

        required(handler.format, "format", saved_game_format_name);
        required(handler.version, "version", saved_game_format_version);
        return required(handler.name, name_key, {});
    }

    //--------------------------------------------------------------------------
    // A site's address belongs to its own zone, so an entity of the inventory has none
    Site read_inventory_site(const raw_values_type& v, const std::string& path)
    {
        object(v, path);

        for (std::string_view key : {"wall", "flat", "cell_position"})
        {
            if (member(v, key) != nullptr)
            {
                fail(member_path(path, key), "an entity of the inventory is sited on its parent, with no address");
            }
        }

        Site site;

        std::string relation_path = member_path(path, "relation");
        std::optional<Relation> relation = to_relation(string_value(required_member(v, path, "relation"), relation_path));
        if (!relation.has_value())
        {
            fail(relation_path, "not a relation the format defines");
        }

        site.relation = *relation;

        if (const raw_values_type* p = member(v, "position"); p != nullptr)
        {
            site.position = read_surface_position(*p, member_path(path, "position"));
        }

        if (const raw_values_type* p = member(v, "orientation"); p != nullptr)
        {
            site.orientation = read_direction(*p, member_path(path, "orientation"));
        }

        return site;
    }

    //--------------------------------------------------------------------------
    Entity read_inventory_entity(const raw_values_type& v, const std::string& path)
    {
        object(v, path);

        Entity entity;
        entity.id = id_value(required_member(v, path, "id"), member_path(path, "id"));
        entity.type = string_value(required_member(v, path, "type"), member_path(path, "type"));
        entity.components = read_components(v, path);

        std::string components_path = member_path(path, "components");
        const raw_values_type* parent = entity.components.is_object() ? member(entity.components, "parent") : nullptr;
        if (parent == nullptr)
        {
            fail(member_path(components_path, "parent"), "an entity of the inventory has a parent, and this has none");
        }

        std::optional<stored_entity_id_type> parent_id = reference_id(*parent);
        if (!parent_id.has_value())
        {
            fail(member_path(components_path, "parent"), "not a reference to an entity");
        }

        entity.parent_id = *parent_id;

        if (const raw_values_type* p = member(entity.components, "site"); p != nullptr)
        {
            entity.site = read_inventory_site(*p, member_path(components_path, "site"));
        }

        if (member(entity.components, "access_point") != nullptr)
        {
            fail(member_path(components_path, "access_point"), "an entity of the inventory is not an access point");
        }

        entity.components.erase("site");
        entity.components.erase("parent");
        return entity;
    }

    //--------------------------------------------------------------------------
    // Each entity's parent is the character or another entity of the inventory, and following parents from any entity
    // reaches the character
    void check_inventory_parents(const CharacterDocument& document)
    {
        std::map<stored_entity_id_type, stored_entity_id_type> parents; // <entity_id, parent_id>
        for (const Entity& entity : document.inventory)
        {
            parents[entity.id] = *entity.parent_id;
        }

        for (size_t i = 0; i < document.inventory.size(); ++i)
        {
            std::string parent_path = member_path(element_path("character.inventory", i), "components.parent");
            std::set<stored_entity_id_type> visited;

            for (stored_entity_id_type id = document.inventory[i].id; id != document.id; id = parents.at(id))
            {
                if (!visited.insert(id).second)
                {
                    fail(parent_path, "its parents form a cycle");
                }

                if (!parents.contains(parents.at(id)) && parents.at(id) != document.id)
                {
                    fail(parent_path, std::format("entity {} is neither the character nor an entity of the inventory", parents.at(id)));
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    CharacterDocument read_character_document(const raw_values_type& v)
    {
        check_file_header(v, saved_game_format_name, saved_game_format_version);

        CharacterDocument document;
        document.version = std::string(saved_game_format_version);
        document.name = string_value(required_member(v, "", name_key), std::string(name_key));

        const std::string path = "character";
        const raw_values_type& character = object(required_member(v, "", path), path);
        document.id = id_value(required_member(character, path, "id"), member_path(path, "id"));
        document.components = read_components(character, path);

        const raw_values_type* name_component = document.components.is_object() ? member(document.components, name_key) : nullptr;
        if (name_component == nullptr || *name_component != document.name)
        {
            fail(std::string(name_key), "not the same as the character's name component");
        }
        document.zone_id = id_value(required_member(character, path, "zone"), member_path(path, "zone"));

        std::string placement_path = member_path(path, "placement");
        const raw_values_type& placement = object(required_member(character, path, "placement"), placement_path);
        document.placement = discrete_directed_point_type(
            read_cell_position(required_member(placement, placement_path, "cell_position"), member_path(placement_path, "cell_position")),
            read_direction(required_member(placement, placement_path, "orientation"), member_path(placement_path, "orientation")));

        std::string inventory_path = member_path(path, "inventory");
        const raw_values_type& inventory = array(required_member(character, path, "inventory"), inventory_path);
        std::set<stored_entity_id_type> ids {document.id};

        for (size_t i = 0; i < inventory.size(); ++i)
        {
            std::string entity_path = element_path(inventory_path, i);
            Entity entity = read_inventory_entity(inventory[i], entity_path);

            if (!ids.insert(entity.id).second)
            {
                fail(member_path(entity_path, "id"), std::format("a second entity with the id {}", entity.id));
            }

            document.inventory.push_back(std::move(entity));
        }

        check_inventory_parents(document);
        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::CharacterDocument quetzal::stratum_format::read_character(std::istream& is, const std::string& stream_name)
{
    return read_document<CharacterDocument>(is, stream_name, read_character_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::CharacterDocument quetzal::stratum_format::read_character(const std::filesystem::path& filepath)
{
    return read_document<CharacterDocument>(filepath, read_character_document);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::read_character_name(std::istream& is, const std::string& stream_name)
{
    return read_name(is, stream_name);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::read_character_name(const std::filesystem::path& filepath)
{
    std::ifstream is(filepath, std::ios::binary);
    if (!is)
    {
        throw std::runtime_error(std::format("{}: cannot be opened", filepath.string()));
    }

    return read_name(is, filepath.string());
}
