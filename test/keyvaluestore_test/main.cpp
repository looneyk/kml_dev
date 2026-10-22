//------------------------------------------------------------------------------
// keyvaluestore_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/Config.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/common/Properties.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    static const filesystem::path data_directory = "data";
    static const filesystem::path out_directory = "out";

    using entries_type = map<string, string>;

    //--------------------------------------------------------------------------
    void check_case_value(const string& name, const optional<string>& value, const string& value_expected)
    {
        if (!value)
        {
            CHECK_CASE(name, false, format("expected \"{}\", got no value", value_expected));
            return;
        }

        CHECK_CASE_EQUAL(name, *value, value_expected);
        return;
    }

    //--------------------------------------------------------------------------
    entries_type entries(const KeyValueStore& key_value_store)
    {
        entries_type result;
        for (const auto& [key, value] : key_value_store)
        {
            result[key] = value;
        }

        return result;
    }

    //--------------------------------------------------------------------------
    string to_text(const entries_type& entries)
    {
        string text;
        for (const auto& [key, value] : entries)
        {
            text += format("{}{}=\"{}\"", text.empty() ? "" : ", ", key, value);
        }

        return "{" + text + "}";
    }

    //--------------------------------------------------------------------------
    void check_case_entries(const string& name, const KeyValueStore& key_value_store, const entries_type& entries_expected)
    {
        const entries_type entries_actual = entries(key_value_store);
        CHECK_CASE(name, entries_actual == entries_expected, format("expected {}, got {}", to_text(entries_expected), to_text(entries_actual)));
        return;
    }

    //--------------------------------------------------------------------------
    template<typename F>
    bool throws_runtime_error(F f)
    {
        try
        {
            f();
        }
        catch (const runtime_error&)
        {
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("construction")
    {
        const KeyValueStore empty_store;
        CHECK_CASE("default is empty", empty_store.begin() == empty_store.end());

        const KeyValueStore listed {{"alpha", "1"}, {"beta", "2"}};
        check_case_entries("initializer list", listed, {{"alpha", "1"}, {"beta", "2"}});

        const KeyValueStore listed_duplicate {{"alpha", "1"}, {"alpha", "2"}};
        check_case_entries("initializer list keeps the last duplicate", listed_duplicate, {{"alpha", "2"}});

        const vector<KeyValueStore::map_type::value_type> pairs {{"alpha", "1"}, {"beta", "2"}, {"alpha", "3"}};
        const KeyValueStore spanned(pairs);
        check_case_entries("span keeps the last duplicate", spanned, {{"alpha", "3"}, {"beta", "2"}});

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("insert and remove")
    {
        KeyValueStore key_value_store;
        key_value_store.insert("alpha", "1");
        check_case_value("insert a key and value", key_value_store.value("alpha"), "1");

        key_value_store.insert("alpha", "2");
        check_case_value("insert a key and value overwrites", key_value_store.value("alpha"), "2");

        key_value_store.insert({"beta", "1"});
        check_case_value("insert a pair", key_value_store.value("beta"), "1");

        key_value_store.insert({"beta", "2"});
        check_case_value("insert a pair overwrites", key_value_store.value("beta"), "2");

        KeyValueStore removing {{"alpha", "1"}, {"beta", "2"}};
        removing.remove("alpha");
        check_case_entries("remove a key", removing, {{"beta", "2"}});

        removing.remove("absent");
        check_case_entries("remove an absent key changes nothing", removing, {{"beta", "2"}});

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("lookup")
    {
        const KeyValueStore key_value_store {{"alpha", "1"}, {"empty", ""}};

        CHECK_CASE("contains a key", key_value_store.contains("alpha"));
        CHECK_CASE("does not contain an absent key", !key_value_store.contains("absent"));
        check_case_value("value", key_value_store.value("alpha"), "1");
        check_case_value("value that is empty", key_value_store.value("empty"), "");
        CHECK_CASE("value of an absent key is empty", !key_value_store.value("absent").has_value());
        CHECK_CASE_EQUAL("value_or", key_value_store.value_or("alpha", "default"), "1");
        CHECK_CASE_EQUAL("value_or of an absent key", key_value_store.value_or("absent", "default"), "default");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("order")
    {
        KeyValueStore key_value_store;
        key_value_store.insert("gamma", "3");
        key_value_store.insert("alpha", "1");
        key_value_store.insert("beta", "2");

        string keys;
        for (const auto& [key, value] : key_value_store)
        {
            keys += (keys.empty() ? "" : ",") + key;
        }

        CHECK_CASE_EQUAL("iteration in key order", keys, "alpha,beta,gamma");
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("union")
    {
        const Properties defaults {{"alpha", "1"}, {"beta", "2"}};
        const Properties overrides {{"beta", "3"}, {"gamma", "4"}};

        const Properties combined = defaults | overrides;
        CHECK_CASE("a | b has the keys of both", combined.contains("alpha") && combined.contains("beta") && combined.contains("gamma"));
        check_case_value("a | b takes b's value", combined.value("beta"), "3");

        Properties assigned = defaults;
        assigned |= overrides;
        check_case_value("a |= b takes b's value", assigned.value("beta"), "3");

        return;
    }

    //--------------------------------------------------------------------------
    template<typename S>
    void check_typed_access()
    {
        constexpr bool has_value = requires(const S& s) { { s.template value<int>(string {}) } -> same_as<optional<int>>; };
        constexpr bool has_value_or = requires(const S& s) { { s.template value_or<int>(string {}, 0) } -> same_as<int>; };

        if constexpr (has_value)
        {
            const S key_value_store {{"count", "42"}, {"ratio", "0.25"}, {"flag", "true"}, {"word", "abc"}, {"partial", "12abc"}};

            const optional<int> count = key_value_store.template value<int>("count");
            CHECK_CASE("value<int>", count == 42, count ? format("got {}", *count) : "got no value");

            const optional<double> ratio = key_value_store.template value<double>("ratio");
            CHECK_CASE("value<double>", ratio == 0.25, ratio ? format("got {}", *ratio) : "got no value");

            const optional<bool> flag = key_value_store.template value<bool>("flag");
            CHECK_CASE("value<bool>", flag == true, flag ? format("got {}", *flag) : "got no value");

            CHECK_CASE("value<int> of an absent key is empty", !key_value_store.template value<int>("absent").has_value());
            CHECK_CASE("value<int> of a word throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value<int>("word")); }));
            CHECK_CASE("value<int> of a partial number throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value<int>("partial")); }));
        }
        else
        {
            CHECK_CASE("value<T>", false, "not implemented");
        }

        if constexpr (has_value_or)
        {
            const S key_value_store {{"count", "42"}, {"word", "abc"}};

            CHECK_CASE("value_or<int>", key_value_store.template value_or<int>("count", 7) == 42);
            CHECK_CASE("value_or<int> of an absent key", key_value_store.template value_or<int>("absent", 7) == 7);
            CHECK_CASE("value_or<int> of a word throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value_or<int>("word", 7)); }));
        }
        else
        {
            CHECK_CASE("value_or<T>", false, "not implemented");
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("typed access")
    {
        check_typed_access<KeyValueStore>();
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("files")
    {
        const KeyValueStore values(data_directory / "values.txt");
        check_case_entries("comments, blank lines and whitespace", values,
            {{"name", "beta"}, {"spaced", "value with spaces"}, {"empty", ""}, {"equation", "a=b"}});

        CHECK_CASE("a file that cannot be opened throws", throws_runtime_error([] { KeyValueStore(data_directory / "absent.txt"); }));
        CHECK_CASE("a line with no = throws", throws_runtime_error([] { KeyValueStore(data_directory / "malformed.txt"); }));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("text")
    {
        const Properties key_value_store {{"alpha", "1"}, {"beta", "two words"}, {"gamma", "x=y"}};
        const entries_type entries_expected = entries(key_value_store);

        stringstream ss;
        ss << key_value_store;

        Properties read_back;
        ss >> read_back;
        check_case_entries("<< read back by >>", read_back, entries_expected);

        filesystem::create_directories(out_directory);
        const filesystem::path filepath = out_directory / "round_trip.txt";
        {
            ofstream file(filepath, ios_base::binary);
            file << key_value_store;
        }

        const KeyValueStore read_from_file(filepath);
        check_case_entries("<< read back from a file", read_from_file, entries_expected);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("aliases")
    {
        CHECK_CASE("Properties is KeyValueStore", is_same_v<Properties, KeyValueStore>);
        CHECK_CASE("Config is KeyValueStore", is_same_v<Config, KeyValueStore>);

        return;
    }

} // namespace
