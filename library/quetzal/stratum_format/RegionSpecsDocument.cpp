//------------------------------------------------------------------------------
// stratum_format
// RegionSpecsDocument.cpp
//------------------------------------------------------------------------------

#include "RegionSpecsDocument.hpp"
#include "RegionSpec.hpp"
#include "checked_values.hpp"
#include <array>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    using document_type = RegionSpecsDocument;

    constexpr std::array<std::string_view, 3> file_keys{"format", "version", "region_specs"};

    //--------------------------------------------------------------------------
    document_type read_region_specs_document(const raw_values_type& v)
    {
        check_file_header(v, document_type::format_name, document_type::format_version);
        check_keys(v, "", file_keys, "region spec file");

        document_type document;
        document.version = std::string(document_type::format_version);

        const std::string region_specs_path = "region_specs";
        for (const auto& [name, region_spec] : object(required_member(v, "", region_specs_path), region_specs_path).items())
        {
            document.region_specs.emplace(name, read_region_spec(region_spec, member_path(region_specs_path, name)));
        }

        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::RegionSpecsDocument quetzal::stratum_format::read_region_specs(const std::filesystem::path& filepath)
{
    return read_document<RegionSpecsDocument>(filepath, read_region_specs_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::RegionSpecsDocument quetzal::stratum_format::read_region_specs(std::istream& is, const std::string& stream_name)
{
    return read_document<RegionSpecsDocument>(is, stream_name, read_region_specs_document);
}
