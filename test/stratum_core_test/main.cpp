//------------------------------------------------------------------------------
// stratum_core_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_core/AccessPoint.hpp"
#include "quetzal/stratum_core/Directionality.hpp"
#include "quetzal/stratum_core/Notation.hpp"
#include "quetzal/stratum_core/RegionTree.hpp"
#include "quetzal/stratum_core/Worldspace.hpp"
#include "quetzal/stratum_core/ZoneExtendedName.hpp"
#include "quetzal/stratum_core/ZoneSpec.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    using range_type = math::Interval<stratum_core::coord_type>;

    //--------------------------------------------------------------------------
    stratum_core::Region region(stratum_core::PersistentId::type id, const string& name)
    {
        stratum_core::Region r;
        r.region_id = stratum_core::PersistentId{id};
        r.name = name;
        return r;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("access point destinations")
    {
        using stratum_core::AccessPoint;
        using stratum_core::RegionSpecName;
        using stratum_core::ZoneExtendedName;

        const vector<pair<string, AccessPoint::destination_type>> destinations{
            {"../[-1]", ZoneExtendedName{"../[-1]"}},
            {"../[+2]", ZoneExtendedName{"../[+2]"}},
            {"../[+0]", ZoneExtendedName{"../[+0]"}},
            {"../[1]", ZoneExtendedName{"../[1]"}},
            {"../[=-3]", ZoneExtendedName{"../[=-3]"}},
            {"../[=4]", ZoneExtendedName{"../[=4]"}},
            {"../../crypt/[-1]", ZoneExtendedName{"../../crypt/[-1]"}},
            {"[-1]", ZoneExtendedName{"[-1]"}},
            {"town/market", ZoneExtendedName{"town/market"}},
            {"/surface/surface_3_4", ZoneExtendedName{"/surface/surface_3_4"}},
            {"/town/market@(12,7)", ZoneExtendedName{"/town/market@(12,7)"}},
            {"../[-1]@(-1.5,+2)", ZoneExtendedName{"../[-1]@(-1.5,+2)"}},
            {"#812", ZoneExtendedName{"#812"}},
            {"#40/market", ZoneExtendedName{"#40/market"}},
            {"#40/../[=0]", ZoneExtendedName{"#40/../[=0]"}},
            {"sewers", RegionSpecName{"sewers"}},
            {"_sewers", RegionSpecName{"_sewers"}},
            {"_town/__market", ZoneExtendedName{"_town/__market"}}
        };

        for (const auto& [text, expected] : destinations)
        {
            optional<AccessPoint::destination_type> parsed = stratum_core::parse_access_point_destination(text);
            CHECK_CASE(format("{} is read and written back the same", text), parsed == expected
                && stratum_core::format_access_point_destination(*parsed) == text);
        }

        for (const string& text : {"", "-1", "+1", "../[-]", "../[+-1]", "../[+1.5]", "../[=]", "../[==1]", "../[-1", "../-1]", "./[-1]", "sewers/",
            "sewers/-1", "1a", "../[-1]@3,5", "../[-99999999999]", "/", "//town", "town//market", "town/", "/#12", "town/#12", "#", "#x", "#-1",
            "#99999999999999999999", "[-1]/market", "../[-1]/[+1]", "market@", "market@(1)", "market@(1,2", "market@(1,2)x", "market@(1.,2)",
            "market@(.5,2)", "market@(1,2)@(3,4)", "@(1,2)", "town/mar-ket"})
        {
            CHECK_CASE(format("\"{}\" is not a destination", text), !stratum_core::parse_access_point_destination(text).has_value());
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("relative zone extended names")
    {
        for (int32_t k : {-3, 0, 2})
        {
            const stratum_core::ZoneExtendedName name = stratum_core::relative_zone_extended_name(k);
            CHECK_CASE(format("{} is written as {} and read back", k, name.value), stratum_core::relative_zone_offset(name.value) == k);
        }

        CHECK_CASE("an absolute level is not relative", !stratum_core::relative_zone_offset("../[=1]").has_value());
        CHECK_CASE("a named zone is not relative", !stratum_core::relative_zone_offset("town/market").has_value());
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("enumeration names")
    {
        for (stratum_core::Directionality directionality : {stratum_core::Directionality::Up, stratum_core::Directionality::Down})
        {
            CHECK_CASE(format("the directionality {} is read back", stratum_core::name(directionality)),
                stratum_core::to_directionality(stratum_core::name(directionality)) == directionality);
        }

        for (stratum_core::Notation notation : {stratum_core::Notation::Block, stratum_core::Notation::Wall})
        {
            CHECK_CASE(format("the notation {} is read back", stratum_core::name(notation)),
                stratum_core::to_notation(stratum_core::name(notation)) == notation);
        }

        CHECK_CASE("an unknown directionality is rejected", !stratum_core::to_directionality("Sideways").has_value());
        CHECK_CASE("an unknown notation is rejected", !stratum_core::to_notation("hex").has_value());
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("zone spec position")
    {
        stratum_core::ZoneSpec zone_spec;
        CHECK_CASE("a spec without position keys is at the origin", zone_spec.position().zero());

        const stratum_core::zone_position_type position{3, -4, -2};
        zone_spec.set_position(position);
        CHECK_CASE("a position set is read back", zone_spec.position() == position);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("region tree")
    {
        stratum_core::RegionTree tree(region(1, "surface"));
        tree.add_subregion(stratum_core::PersistentId{1}, region(2, "town"));
        tree.add_subregion(stratum_core::PersistentId{2}, region(3, "market"));

        CHECK_CASE("the top-level region is first", tree.regions().front().region_id == stratum_core::PersistentId{1});
        CHECK_CASE("a subregion has its parent", tree.region(stratum_core::PersistentId{3}).parent_id == stratum_core::PersistentId{2});
        CHECK_CASE("a parent has its subregion", tree.region(stratum_core::PersistentId{2}).subregion_ids
            == vector<stratum_core::PersistentId>{stratum_core::PersistentId{3}});
        CHECK_CASE("a region is contained", tree.contains(stratum_core::PersistentId{3}));
        CHECK_CASE("a region not added is not contained", !tree.contains(stratum_core::PersistentId{4}));
        CHECK_CASE_EQUAL("the extended name runs from the top-level region", tree.extended_name(stratum_core::PersistentId{3}),
            string("/surface/town/market"));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("worldspace")
    {
        stratum_core::RegionSpec region_spec;
        region_spec.extent = stratum_core::RegionExtent{stratum_core::RegionExtent::Arrangement::Boxes,
            {stratum_core::RegionExtent::Box{range_type(-2, 2), range_type(0, 3), range_type(-5, 0)}}, {}};

        const stratum_core::Worldspace worldspace = stratum_core::Worldspace::create(stratum_core::PersistentId{1}, "overworld", 7, region_spec,
            region(2, "surface"));
        CHECK_CASE("a position on the extent's upper bound is contained", worldspace.contains({2, 3, 0}));
        CHECK_CASE("a position on the extent's lower bound is contained", worldspace.contains({-2, 0, -5}));
        CHECK_CASE("a position past the upper bound is not contained", !worldspace.contains({3, 0, 0}));
        CHECK_CASE("the root region is the tree's top-level region", worldspace.regions.root().region_id == stratum_core::PersistentId{2});

        CHECK_THROWS_AS(stratum_core::Worldspace::create(stratum_core::PersistentId{1}, "overworld", 7, stratum_core::RegionSpec{},
            region(2, "surface")), runtime_error);

        stratum_core::RegionSpec positions_spec;
        positions_spec.extent = stratum_core::RegionExtent{stratum_core::RegionExtent::Arrangement::Positions, {}, {}};
        CHECK_THROWS_AS(stratum_core::Worldspace::create(stratum_core::PersistentId{1}, "overworld", 7, positions_spec, region(2, "surface")),
            runtime_error);

        stratum_core::RegionSpec partial_spec;
        partial_spec.extent = stratum_core::RegionExtent{stratum_core::RegionExtent::Arrangement::Boxes,
            {stratum_core::RegionExtent::Box{range_type(-2, 2), range_type(0, 3), nullopt}}, {}};
        CHECK_THROWS_AS(stratum_core::Worldspace::create(stratum_core::PersistentId{1}, "overworld", 7, partial_spec, region(2, "surface")),
            runtime_error);
        return;
    }

} // namespace
