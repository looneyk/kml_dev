//------------------------------------------------------------------------------
// stratum_format
// WorldDefinitionDocument.cpp
//------------------------------------------------------------------------------

#include "WorldDefinitionDocument.hpp"
#include "checked_values.hpp"
#include "json_util.hpp"
#include <array>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    using document_type = WorldDefinitionDocument;

    constexpr std::string_view starting_region_key = "starting_region";
    constexpr std::string_view region_specs_key = "region_specs";
    constexpr std::array<std::string_view, 4> file_keys {"format", "version", starting_region_key, region_specs_key};

    //--------------------------------------------------------------------------
    document_type read_world_definition_document(const raw_values_type& v)
    {
        check_file_header(v, document_type::format_name, document_type::format_version);
        check_keys(v, "", file_keys, "world definition file");

        document_type document;
        document.version = std::string(document_type::format_version);
        document.starting_region = string_value(required_member(v, "", starting_region_key), std::string(starting_region_key));

        const std::string trees_path(region_specs_key);
        const raw_values_type& trees = array(required_member(v, "", region_specs_key), trees_path);
        for (size_t i = 0; i < trees.size(); ++i)
        {
            const std::string tree_path = element_path(trees_path, i);
            const raw_values_type& tree = array(trees[i], tree_path);

            document_type::region_spec_tree_type specs;
            for (size_t j = 0; j < tree.size(); ++j)
            {
                specs.push_back(text_values(tree[j], element_path(tree_path, j)));
            }

            document.region_specs.push_back(std::move(specs));
        }

        return document;
    }

    //--------------------------------------------------------------------------
    std::string tree_text(const document_type::region_spec_tree_type& tree)
    {
        if (tree.empty())
        {
            return indentation(entry_indent) + "[]";
        }

        std::string text = indentation(entry_indent) + "[\n";
        for (size_t i = 0; i < tree.size(); ++i)
        {
            text += std::format("{}{}{}\n", indentation(entry_indent + 2), text_values_text(tree[i]), i + 1 == tree.size() ? "" : ",");
        }

        return text + indentation(entry_indent) + "]";
    }

    //--------------------------------------------------------------------------
    std::string document_text(const document_type& document)
    {
        std::string trees = std::format("{}\"{}\": [", indentation(section_indent), region_specs_key);

        if (document.region_specs.empty())
        {
            trees += "]";
        }
        else
        {
            trees += "\n";

            for (size_t i = 0; i < document.region_specs.size(); ++i)
            {
                trees += std::format("{}{}\n", tree_text(document.region_specs[i]), i + 1 == document.region_specs.size() ? "" : ",");
            }

            trees += indentation(section_indent) + "]";
        }

        std::vector<std::string> sections = file_header_sections(document_type::format_name, document_type::format_version);
        sections.push_back(std::format("{}\"{}\": {}", indentation(section_indent), starting_region_key, string_text(document.starting_region)));
        sections.push_back(trees);
        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::WorldDefinitionDocument quetzal::stratum_format::read_world_definition(const std::filesystem::path& filepath)
{
    return read_document<WorldDefinitionDocument>(filepath, read_world_definition_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::WorldDefinitionDocument quetzal::stratum_format::read_world_definition(std::istream& is, const std::string& stream_name)
{
    return read_document<WorldDefinitionDocument>(is, stream_name, read_world_definition_document);
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const WorldDefinitionDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const WorldDefinitionDocument& document)
{
    os << document_text(document);
    return;
}
