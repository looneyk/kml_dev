//------------------------------------------------------------------------------
// keyvaluestore_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/Config.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/common/Properties.hpp"
#include <concepts>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
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

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    static const filesystem::path data_directory = "data";
    static const filesystem::path out_directory = "out";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    using entries_type = map<string, string>;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_equal(const string& name, const string& value, const string& value_expected)
    {
        report_case(name, value == value_expected, format("expected \"{}\", got \"{}\"", value_expected, value));
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_value(const string& name, const optional<string>& value, const string& value_expected)
    {
        if (!value)
        {
            report_case(name, false, format("expected \"{}\", got no value", value_expected));
            return;
        }

        report_case_equal(name, *value, value_expected);
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
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
    void report_case_entries(const string& name, const KeyValueStore& key_value_store, const entries_type& entries_expected)
    {
        const entries_type entries_actual = entries(key_value_store);
        report_case(name, entries_actual == entries_expected, format("expected {}, got {}", to_text(entries_expected), to_text(entries_actual)));
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
    void test_construction()
    {
        report_group("construction");

        const KeyValueStore empty_store;
        report_case("default is empty", empty_store.begin() == empty_store.end());

        const KeyValueStore listed {{"alpha", "1"}, {"beta", "2"}};
        report_case_entries("initializer list", listed, {{"alpha", "1"}, {"beta", "2"}});

        const KeyValueStore listed_duplicate {{"alpha", "1"}, {"alpha", "2"}};
        report_case_entries("initializer list keeps the last duplicate", listed_duplicate, {{"alpha", "2"}});

        const vector<KeyValueStore::map_type::value_type> pairs {{"alpha", "1"}, {"beta", "2"}, {"alpha", "3"}};
        const KeyValueStore spanned(pairs);
        report_case_entries("span keeps the last duplicate", spanned, {{"alpha", "3"}, {"beta", "2"}});

        return;
    }

    //--------------------------------------------------------------------------
    void test_insert_and_remove()
    {
        report_group("insert and remove");

        KeyValueStore key_value_store;
        key_value_store.insert("alpha", "1");
        report_case_value("insert a key and value", key_value_store.value("alpha"), "1");

        key_value_store.insert("alpha", "2");
        report_case_value("insert a key and value overwrites", key_value_store.value("alpha"), "2");

        key_value_store.insert({"beta", "1"});
        report_case_value("insert a pair", key_value_store.value("beta"), "1");

        key_value_store.insert({"beta", "2"});
        report_case_value("insert a pair overwrites", key_value_store.value("beta"), "2");

        KeyValueStore removing {{"alpha", "1"}, {"beta", "2"}};
        removing.remove("alpha");
        report_case_entries("remove a key", removing, {{"beta", "2"}});

        removing.remove("absent");
        report_case_entries("remove an absent key changes nothing", removing, {{"beta", "2"}});

        return;
    }

    //--------------------------------------------------------------------------
    void test_lookup()
    {
        report_group("lookup");

        const KeyValueStore key_value_store {{"alpha", "1"}, {"empty", ""}};

        report_case("contains a key", key_value_store.contains("alpha"));
        report_case("does not contain an absent key", !key_value_store.contains("absent"));
        report_case_value("value", key_value_store.value("alpha"), "1");
        report_case_value("value that is empty", key_value_store.value("empty"), "");
        report_case("value of an absent key is empty", !key_value_store.value("absent").has_value());
        report_case_equal("value_or", key_value_store.value_or("alpha", "default"), "1");
        report_case_equal("value_or of an absent key", key_value_store.value_or("absent", "default"), "default");

        return;
    }

    //--------------------------------------------------------------------------
    void test_order()
    {
        report_group("order");

        KeyValueStore key_value_store;
        key_value_store.insert("gamma", "3");
        key_value_store.insert("alpha", "1");
        key_value_store.insert("beta", "2");

        string keys;
        for (const auto& [key, value] : key_value_store)
        {
            keys += (keys.empty() ? "" : ",") + key;
        }

        report_case_equal("iteration in key order", keys, "alpha,beta,gamma");
        return;
    }

    //--------------------------------------------------------------------------
    void test_union()
    {
        report_group("union");

        const Properties defaults {{"alpha", "1"}, {"beta", "2"}};
        const Properties overrides {{"beta", "3"}, {"gamma", "4"}};

        const Properties combined = defaults | overrides;
        report_case("a | b has the keys of both", combined.contains("alpha") && combined.contains("beta") && combined.contains("gamma"));
        report_case_value("a | b takes b's value", combined.value("beta"), "3");

        Properties assigned = defaults;
        assigned |= overrides;
        report_case_value("a |= b takes b's value", assigned.value("beta"), "3");

        return;
    }

    //--------------------------------------------------------------------------
    template<typename S>
    void test_typed_access()
    {
        report_group("typed access");

        constexpr bool has_value = requires(const S& s) { { s.template value<int>(string {}) } -> same_as<optional<int>>; };
        constexpr bool has_value_or = requires(const S& s) { { s.template value_or<int>(string {}, 0) } -> same_as<int>; };

        if constexpr (has_value)
        {
            const S key_value_store {{"count", "42"}, {"ratio", "0.25"}, {"flag", "true"}, {"word", "abc"}, {"partial", "12abc"}};

            const optional<int> count = key_value_store.template value<int>("count");
            report_case("value<int>", count == 42, count ? format("got {}", *count) : "got no value");

            const optional<double> ratio = key_value_store.template value<double>("ratio");
            report_case("value<double>", ratio == 0.25, ratio ? format("got {}", *ratio) : "got no value");

            const optional<bool> flag = key_value_store.template value<bool>("flag");
            report_case("value<bool>", flag == true, flag ? format("got {}", *flag) : "got no value");

            report_case("value<int> of an absent key is empty", !key_value_store.template value<int>("absent").has_value());
            report_case("value<int> of a word throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value<int>("word")); }));
            report_case("value<int> of a partial number throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value<int>("partial")); }));
        }
        else
        {
            report_case("value<T>", false, "not implemented");
        }

        if constexpr (has_value_or)
        {
            const S key_value_store {{"count", "42"}, {"word", "abc"}};

            report_case("value_or<int>", key_value_store.template value_or<int>("count", 7) == 42);
            report_case("value_or<int> of an absent key", key_value_store.template value_or<int>("absent", 7) == 7);
            report_case("value_or<int> of a word throws", throws_runtime_error([&] { static_cast<void>(key_value_store.template value_or<int>("word", 7)); }));
        }
        else
        {
            report_case("value_or<T>", false, "not implemented");
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename S>
    void test_unknown_keys()
    {
        report_group("unknown keys");

        constexpr bool has_find_unknown_keys = requires(const S& s, const vector<string_view>& known_keys)
        {
            { s.find_unknown_keys(known_keys) } -> convertible_to<vector<string>>;
        };

        if constexpr (has_find_unknown_keys)
        {
            const S key_value_store {{"alpha", "1"}, {"beta", "2"}, {"gamma", "3"}};

            const vector<string> none = key_value_store.find_unknown_keys(vector<string_view> {"alpha", "beta", "gamma", "delta"});
            report_case("all keys known", none.empty());

            const vector<string> unknown = key_value_store.find_unknown_keys(vector<string_view> {"alpha"});
            report_case("unknown keys found", unknown == vector<string> {"beta", "gamma"});
        }
        else
        {
            report_case("find_unknown_keys", false, "not implemented");
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_files()
    {
        report_group("files");

        const KeyValueStore values(data_directory / "values.txt");
        report_case_entries("comments, blank lines and whitespace", values,
            {{"name", "beta"}, {"spaced", "value with spaces"}, {"empty", ""}, {"equation", "a=b"}});

        report_case("a file that cannot be opened throws", throws_runtime_error([] { KeyValueStore(data_directory / "absent.txt"); }));
        report_case("a line with no = throws", throws_runtime_error([] { KeyValueStore(data_directory / "malformed.txt"); }));

        return;
    }

    //--------------------------------------------------------------------------
    void test_text()
    {
        report_group("text");

        const Properties key_value_store {{"alpha", "1"}, {"beta", "two words"}, {"gamma", "x=y"}};
        const entries_type entries_expected = entries(key_value_store);

        stringstream ss;
        ss << key_value_store;

        Properties read_back;
        ss >> read_back;
        report_case_entries("<< read back by >>", read_back, entries_expected);

        filesystem::create_directories(out_directory);
        const filesystem::path filepath = out_directory / "round_trip.txt";
        {
            ofstream file(filepath, ios_base::binary);
            file << key_value_store;
        }

        const KeyValueStore read_from_file(filepath);
        report_case_entries("<< read back from a file", read_from_file, entries_expected);

        return;
    }

    //--------------------------------------------------------------------------
    void test_aliases()
    {
        report_group("aliases");

        report_case("Properties is KeyValueStore", is_same_v<Properties, KeyValueStore>);
        report_case("Config is KeyValueStore", is_same_v<Config, KeyValueStore>);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_construction();
        test_insert_and_remove();
        test_lookup();
        test_order();
        test_union();
        test_typed_access<KeyValueStore>();
        test_unknown_keys<KeyValueStore>();
        test_files();
        test_text();
        test_aliases();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "exception: " << e.what() << escape_reset_string << endl;
        return 1;
    }

    return report_results();
}
