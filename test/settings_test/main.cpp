//------------------------------------------------------------------------------
// settings_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsGroups.hpp"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

using namespace std;
using namespace quetzal::settings;

namespace
{

    // An enumeration with a setting, which has a std::formatter and a from_chars, as any enumeration written as text does
    enum class Mode
    {
        Restore,
        Reseed
    };

    //--------------------------------------------------------------------------
    std::string_view mode_name(Mode mode)
    {
        return mode == Mode::Restore ? "restore" : "reseed";
    }

    //--------------------------------------------------------------------------
    // Reads "restore" or "reseed", and stops after it, as the standard overloads stop after a number
    std::from_chars_result from_chars(const char* first, const char* last, Mode& mode)
    {
        const std::string_view text(first, last);
        for (Mode candidate : {Mode::Restore, Mode::Reseed})
        {
            if (text.starts_with(mode_name(candidate)))
            {
                mode = candidate;
                return {first + mode_name(candidate).size(), std::errc {}};
            }
        }

        return {first, std::errc::invalid_argument};
    }

} // namespace

//------------------------------------------------------------------------------
template<>
struct std::formatter<Mode> : std::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(Mode mode, FormatContext& context) const
    {
        return std::formatter<std::string_view>::format(mode_name(mode), context);
    }
};

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

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
    // Returns true if the function throws std::runtime_error
    template<typename F>
    bool throws(F function)
    {
        try
        {
            function();
        }
        catch (const runtime_error&)
        {
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    vector<string> sorted(vector<string> keys)
    {
        ranges::sort(keys);
        return keys;
    }

    //--------------------------------------------------------------------------
    void insert_all(Settings& test_settings)
    {
        test_settings.register_type<Mode>("mode", {"restore", "reseed"});
        test_settings.insert("gameplay.mode", {.type_name = "mode", .default_value = "restore"});
        test_settings.insert("gameplay.difficulty", {.type_name = "text", .default_value = "adventure", .scope = SettingScope::Game});
        test_settings.insert("video.fullscreen", {.type_name = "boolean", .default_value = "false", .restart_required = true});
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280", .minimum = "640", .maximum = "7680"});
        test_settings.insert("audio.volume", {.type_name = "real", .default_value = "0.8", .minimum = "0", .maximum = "1"});
        return;
    }

    //--------------------------------------------------------------------------
    void test_text()
    {
        report_group("text");

        report_case("a boolean is true or false", quetzal::to_string(true) == "true" && quetzal::try_type<bool>("false") == false
            && !quetzal::try_type<bool>("yes"));
        report_case("an integer reads back, and text with anything after the number is none", quetzal::try_type<int>(quetzal::to_string(-42))
            == -42 && !quetzal::try_type<int>("42x") && !quetzal::try_type<int>(""));
        report_case("a real is the shortest text that reads back as the same value", quetzal::to_string(0.1) == "0.1"
            && quetzal::try_type<double>("0.1") == 0.1 && !quetzal::try_type<double>("one"));
        report_case("a string is its own text", quetzal::try_type<std::string>("adventure") == "adventure");
        report_case("an enumeration's value is the text its formatter writes and its from_chars reads", quetzal::to_string(Mode::Reseed) == "reseed"
            && quetzal::try_type<Mode>("restore") == Mode::Restore && !quetzal::try_type<Mode>("Reseed") && !quetzal::try_type<Mode>("reseeded"));
        report_case("the types a setting may have are those that format and parse", SettingValue<bool> && SettingValue<int> && SettingValue<double>
            && SettingValue<std::string> && SettingValue<Mode> && !SettingValue<SettingScope>);
        return;
    }

    //--------------------------------------------------------------------------
    void test_values()
    {
        report_group("values");

        Settings test_settings;
        insert_all(test_settings);

        report_case("a setting has its default until set", test_settings.value<Mode>("gameplay.mode") == Mode::Restore
            && test_settings.value<int64_t>("video.width") == 1280 && test_settings.value<double>("audio.volume") == 0.8
            && !test_settings.value<bool>("video.fullscreen"));
        report_case("and has its type name, default text, scope and whether a restart is required",
            test_settings.setting("video.fullscreen").type_name == "boolean" && test_settings.setting("video.fullscreen").default_value == "false"
            && test_settings.setting("video.fullscreen").scope == SettingScope::User && test_settings.setting("video.fullscreen").restart_required);

        test_settings.set_value<Mode>("gameplay.mode", Mode::Reseed);
        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("a value set is the value read", test_settings.value<Mode>("gameplay.mode") == Mode::Reseed
            && test_settings.value<int64_t>("video.width") == 1920);
        report_case("and is read by key as text", test_settings.text("gameplay.mode") == "reseed" && test_settings.text("video.width") == "1920"
            && test_settings.text("gameplay.difficulty") == "adventure");

        test_settings.set_text("audio.volume", "0.25");
        report_case("a value set by key as text is read as its type", test_settings.value<double>("audio.volume") == 0.25);
        report_case("a key that is not a setting's is an error", throws([&] { test_settings.text("video.height"); })
            && throws([&] { test_settings.set_text("video.height", "720"); }) && !test_settings.contains("video.height"));
        report_case("and so is text that is no value of the setting's type", throws([&] { test_settings.set_text("video.width", "wide"); })
            && test_settings.value<int64_t>("video.width") == 1920);

        report_case("a value out of range sets nothing, and returns false", !test_settings.set_values({{"video.width", "800"}, {"audio.volume", "1.5"}})
            && !test_settings.set_value<int64_t>("video.width", 100) && test_settings.value<int64_t>("video.width") == 1920
            && test_settings.value<double>("audio.volume") == 0.25);
        report_case("and a value in range returns true", test_settings.set_value<int64_t>("video.width", 640) && test_settings.bounded_inclusive("audio.volume", "1")
            && !test_settings.bounded_inclusive("audio.volume", "-0.5") && test_settings.bounded_inclusive("gameplay.difficulty", "anything"));
        report_case("a setting's range is checked when it is inserted", throws([&] { test_settings.insert("video.height", {.type_name = "integer",
            .default_value = "100", .minimum = "480"}); }) && throws([&] { test_settings.insert("video.title", {.type_name = "text",
            .default_value = "a", .maximum = "z"}); }) && throws([&] { test_settings.insert("video.height", {.type_name = "integer",
            .default_value = "720", .maximum = "tall"}); }));
        return;
    }

    //--------------------------------------------------------------------------
    void test_types()
    {
        report_group("types");

        Settings test_settings;
        report_case("boolean, integer, real and text are the types the store registers, with their C++ types",
            test_settings.type("boolean").cpp_type == type_index(typeid(bool)) && test_settings.type("integer").cpp_type == type_index(typeid(int64_t))
            && test_settings.type("real").cpp_type == type_index(typeid(double)) && test_settings.type("text").cpp_type == type_index(typeid(std::string))
            && !test_settings.registered_type("mode"));
        report_case("and each is its own base type, with no values", test_settings.type("boolean").base_type == SettingBaseType::Boolean
            && test_settings.type("integer").base_type == SettingBaseType::Integer && test_settings.type("real").base_type == SettingBaseType::Real
            && test_settings.type("text").base_type == SettingBaseType::Text && test_settings.type("integer").values.empty());

        test_settings.insert("torches.flicker", {.type_name = "boolean", .default_value = "true"});
        report_case("a setting is read as the C++ type of its type", test_settings.contains("torches.flicker")
            && test_settings.value<bool>("torches.flicker") && test_settings.text("torches.flicker") == "true");

        test_settings.register_type<Mode>("mode", {"restore", "reseed"});
        test_settings.insert("torches.mode", {.type_name = "mode", .default_value = "reseed", .scope = SettingScope::Game});
        report_case("a type a library registers, an enumeration among them, may be a setting's type",
            test_settings.type("mode").cpp_type == type_index(typeid(Mode)) && test_settings.value<Mode>("torches.mode") == Mode::Reseed
            && throws([&] { test_settings.set_text("torches.mode", "sometimes"); }));
        report_case("and an enumeration in C++ has its values, and is based on text", test_settings.type("mode").values
            == vector<string> {"restore", "reseed"} && test_settings.type("mode").base_type == SettingBaseType::Text);

        test_settings.register_enumeration("torches.style", {"steady", "flicker", "gutter"});
        test_settings.insert("torches.style", {.type_name = "torches.style", .default_value = "flicker"});
        test_settings.set_text("torches.style", "gutter");
        report_case("an enumeration defined in data is text, limited to its values", test_settings.type("torches.style").cpp_type
            == type_index(typeid(std::string)) && test_settings.type("torches.style").base_type == SettingBaseType::Text
            && test_settings.value<std::string>("torches.style") == "gutter" && throws([&] { test_settings.set_text("torches.style", "dim"); }));
        report_case("and one registered twice, or with no values, is an error", throws([&] { test_settings.register_enumeration("torches.style",
            {"steady"}); }) && throws([&] { test_settings.register_enumeration("torches.shade", {}); }));

        report_case("a type name that is not registered is an error", throws([&] { test_settings.insert("torches.colour", {.type_name = "colour",
            .default_value = "red"}); }));
        report_case("and so is a default that is no value of the type", throws([&] { test_settings.insert("torches.count", {.type_name = "integer",
            .default_value = "many"}); }));
        report_case("and a key inserted twice, or empty", throws([&] { test_settings.insert("torches.flicker", {.type_name = "boolean",
            .default_value = "false"}); }) && throws([&] { test_settings.insert("", {.type_name = "boolean", .default_value = "false"}); }));

        test_settings.clear();
        report_case("clearing the store keeps the four types it starts with, and no other", test_settings.registered_type("boolean")
            && !test_settings.registered_type("mode") && !test_settings.contains("torches.flicker"));
        return;
    }

    //--------------------------------------------------------------------------
    void test_scopes()
    {
        report_group("scopes");

        Settings test_settings;
        insert_all(test_settings);

        test_settings.set_value<std::string>("gameplay.difficulty", "hardcore");
        test_settings.set_value<bool>("video.fullscreen", true);

        quetzal::KeyValueStore user_values = test_settings.values(SettingScope::User);
        quetzal::KeyValueStore game_values = test_settings.values(SettingScope::Game);
        report_case("the values of a scope are those set in it, as text", user_values.value("video.fullscreen") == "true"
            && !user_values.contains("gameplay.difficulty") && !user_values.contains("video.width") && game_values.value("gameplay.difficulty") == "hardcore");

        test_settings.reset(SettingScope::Game);
        quetzal::KeyValueStore game_values_reset = test_settings.values(SettingScope::Game);
        report_case("resetting a scope returns its settings to their defaults, and leaves the other's",
            test_settings.value<std::string>("gameplay.difficulty") == "adventure" && test_settings.value<bool>("video.fullscreen")
            && game_values_reset.begin() == game_values_reset.end());

        test_settings.set_values(game_values);
        report_case("and setting the values saved restores them", test_settings.value<std::string>("gameplay.difficulty") == "hardcore");

        quetzal::KeyValueStore partly_wrong {{"video.width", "1600"}, {"video.height", "900"}};
        report_case("values with a key that is not a setting's are an error, and none of them is set",
            throws([&] { test_settings.set_values(partly_wrong); }) && test_settings.value<int64_t>("video.width") == 1280);
        return;
    }

    //--------------------------------------------------------------------------
    void test_notifications()
    {
        report_group("change notifications");

        Settings test_settings;
        insert_all(test_settings);

        vector<vector<string>> notifications;
        test_settings.add_change_listener([&notifications](const vector<string>& keys_changed) { notifications.push_back(keys_changed); });

        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("a change calls each listener with the key changed", notifications == vector<vector<string>> {{"video.width"}});

        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("and setting the same value again does not", notifications.size() == 1);

        const vector<string> keys_set {"gameplay.mode", "video.fullscreen", "video.width"};
        test_settings.set_values({{"video.width", "800"}, {"video.fullscreen", "true"}, {"gameplay.mode", "reseed"}});
        report_case("setting values calls each listener once, with every key changed", notifications.size() == 2
            && sorted(notifications.back()) == keys_set);

        test_settings.reset(SettingScope::User);
        report_case("and so does resetting a scope", notifications.size() == 3 && sorted(notifications.back()) == keys_set);

        test_settings.insert("flicker", {.type_name = "boolean", .default_value = "true"});
        test_settings.set_text("flicker", "false");
        report_case("a key is notified as a whole, whatever its segments", notifications.back() == vector<string> {"flicker"});

        // The listener reads the new value, as a subsystem applying its settings does
        int64_t width_applied = 0;
        test_settings.add_change_listener([&test_settings, &width_applied](const vector<string>&)
        {
            width_applied = test_settings.value<int64_t>("video.width");
        });
        test_settings.set_value<int64_t>("video.width", 2560);
        report_case("a listener reads the value already changed", width_applied == 2560);
        return;
    }

    //--------------------------------------------------------------------------
    void test_pre_change_listeners()
    {
        report_group("pre-change listeners");

        Settings test_settings;
        insert_all(test_settings);

        int64_t width_before = 0;
        quetzal::KeyValueStore values_seen;
        Settings::listener_id_type listener_id = test_settings.add_pre_change_listener([&](const quetzal::KeyValueStore& values)
        {
            width_before = test_settings.value<int64_t>("video.width");
            values_seen = values;
        });

        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("a pre-change listener is called with the values before any is set", width_before == 1280
            && values_seen.value("video.width") == "1920" && test_settings.value<int64_t>("video.width") == 1920);

        test_settings.add_pre_change_listener([](const quetzal::KeyValueStore& values)
        {
            if (values.contains("video.fullscreen"))
            {
                throw runtime_error("fullscreen is locked");
            }
        });
        report_case("and one that throws rejects every value", throws([&] { test_settings.set_values({{"video.width", "800"},
            {"video.fullscreen", "true"}}); }) && test_settings.value<int64_t>("video.width") == 1920 && !test_settings.value<bool>("video.fullscreen"));

        values_seen = {};
        test_settings.reset(SettingScope::User);
        report_case("resetting a scope calls no pre-change listener", values_seen.begin() == values_seen.end()
            && test_settings.value<int64_t>("video.width") == 1280);

        test_settings.remove_listener(listener_id);
        test_settings.set_value<int64_t>("video.width", 1600);
        report_case("a listener removed is not called", values_seen.begin() == values_seen.end());

        size_t change_count = 0;
        test_settings.add_change_listener([&change_count](const vector<string>&) { ++change_count; });
        test_settings.clear();
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("clearing the store keeps its listeners", change_count == 1);
        return;
    }

    //--------------------------------------------------------------------------
    void insert_difficulty(Settings& test_settings, SettingsGroups& test_groups)
    {
        test_settings.insert("difficulty.count", {.type_name = "real", .default_value = "1", .scope = SettingScope::Game, .maximum = "4"});
        test_settings.insert("difficulty.speed", {.type_name = "real", .default_value = "1", .scope = SettingScope::Game});
        test_groups.insert(SettingsGroup("difficulty", SettingScope::Game, "normal", {
            {"easy", "Easy", false, {{"difficulty.count", "0.5"}, {"difficulty.speed", "0.75"}}},
            {"normal", "Normal", false, {{"difficulty.count", "1.2"}}},
            {"hard", "Hard", false, {{"difficulty.count", "1.5"}, {"difficulty.speed", "1.25"}}},
            {"custom", "Custom", true, {}}}, "Difficulty", "difficulty"));
        return;
    }

    //--------------------------------------------------------------------------
    // The setting values a game saves: those set that no preset sets
    quetzal::KeyValueStore saved_setting_values(const Settings& test_settings, const SettingsGroups& test_groups)
    {
        quetzal::KeyValueStore values;
        for (const auto& [key, value] : test_settings.values(SettingScope::Game))
        {
            if (!test_groups.has_value_from_preset(key))
            {
                values.insert(key, value);
            }
        }

        return values;
    }

    //--------------------------------------------------------------------------
    void test_groups()
    {
        report_group("settings groups");

        Settings test_settings;
        SettingsGroups test_groups(test_settings);
        insert_difficulty(test_settings, test_groups);
        report_case("a group has its display name, category and default preset, and its members the preset's values",
            test_groups.group("difficulty").display_name() == "Difficulty" && test_groups.group("difficulty").category() == "difficulty"
            && test_groups.group("difficulty").selected().name == "normal"
            && test_settings.value<double>("difficulty.count") == 1.2 && test_settings.value<double>("difficulty.speed") == 1);

        vector<vector<string>> notifications;
        vector<vector<string>> selections;
        test_settings.add_change_listener([&notifications](const vector<string>& keys_changed) { notifications.push_back(keys_changed); });
        test_groups.add_selection_listener([&selections](const vector<string>& group_keys) { selections.push_back(group_keys); });

        test_groups.select("difficulty", "hard");
        report_case("selecting a preset sets each member to its value, with one notification of the members and one of the selection",
            test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 1.25
            && notifications.size() == 1 && sorted(notifications.back()) == vector<string> {"difficulty.count", "difficulty.speed"}
            && selections == vector<vector<string>> {{"difficulty"}});

        quetzal::KeyValueStore hard_selections = test_groups.values(SettingScope::Game);
        quetzal::KeyValueStore hard_values = saved_setting_values(test_settings, test_groups);
        report_case("a preset that is not editable is saved as its name alone", hard_selections.value("difficulty") == "hard"
            && hard_values.begin() == hard_values.end());

        test_settings.set_value<double>("difficulty.speed", 2);
        report_case("setting a member selects the editable preset, and keeps the other members' values",
            test_groups.group("difficulty").selected().name == "custom" && test_settings.value<double>("difficulty.count") == 1.5
            && test_settings.value<double>("difficulty.speed") == 2 && notifications.back() == vector<string> {"difficulty.speed"}
            && selections.back() == vector<string> {"difficulty"});

        quetzal::KeyValueStore custom_selections = test_groups.values(SettingScope::Game);
        quetzal::KeyValueStore custom_values = saved_setting_values(test_settings, test_groups);
        report_case("an editable preset is saved with its members' values", custom_selections.value("difficulty") == "custom"
            && custom_values.value("difficulty.count") == "1.5" && custom_values.value("difficulty.speed") == "2");

        test_groups.select("difficulty", "normal");
        quetzal::KeyValueStore normal_selections = test_groups.values(SettingScope::Game);
        quetzal::KeyValueStore normal_values = saved_setting_values(test_settings, test_groups);
        report_case("a member a preset leaves out returns to its default", test_settings.value<double>("difficulty.count") == 1.2
            && test_settings.value<double>("difficulty.speed") == 1 && normal_selections.begin() == normal_selections.end()
            && normal_values.begin() == normal_values.end());

        test_groups.set_values(custom_selections);
        test_settings.set_values(custom_values);
        report_case("setting the values saved restores the preset and its members", test_groups.group("difficulty").selected().name == "custom"
            && test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 2);

        test_settings.reset(SettingScope::Game);
        test_groups.reset(SettingScope::Game);
        report_case("resetting a scope selects its groups' default presets", test_groups.group("difficulty").selected().name == "normal"
            && test_settings.value<double>("difficulty.count") == 1.2 && test_settings.value<double>("difficulty.speed") == 1);

        report_case("an unknown preset or group is an error", throws([&] { test_groups.select("difficulty", "nightmare"); })
            && throws([&] { test_groups.select("challenge", "hard"); }));

        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.insert("other.count", {.type_name = "real", .default_value = "1", .scope = SettingScope::Game});
        report_case("a group key that is a group's already is an error", throws([&] { test_groups.insert(SettingsGroup("difficulty",
            SettingScope::Game, "a", {{"a", "A"}})); }));
        report_case("and so is a group with no default preset", throws([&] { test_groups.insert(SettingsGroup("other", SettingScope::Game, "a",
            {{"b", "B", false, {{"other.count", "2"}}}})); }));
        report_case("and a preset that sets no setting, another scope's, another group's, no value of its type, or one out of its range",
            throws([&] { test_groups.insert_preset("difficulty", {"bad", "Bad", false, {{"difficulty.size", "1"}}}); })
            && throws([&] { test_groups.insert_preset("difficulty", {"bad", "Bad", false, {{"video.width", "800"}}}); })
            && throws([&] { test_groups.insert(SettingsGroup("other", SettingScope::Game, "a", {{"a", "A", false, {{"difficulty.count", "2"}}}})); })
            && throws([&] { test_groups.insert_preset("difficulty", {"bad", "Bad", false, {{"difficulty.count", "fast"}}}); })
            && throws([&] { test_groups.insert_preset("difficulty", {"bad", "Bad", false, {{"difficulty.count", "5"}}}); }));
        report_case("and a preset named twice, a second editable preset, or an editable preset with values",
            throws([&] { test_groups.insert_preset("difficulty", {"hard", "Hard"}); })
            && throws([&] { test_groups.insert_preset("difficulty", {"custom2", "Custom 2", true}); })
            && throws([&] { test_groups.insert(SettingsGroup("other", SettingScope::Game, "a", {{"a", "A", true, {{"other.count", "2"}}}})); }));

        test_groups.insert(SettingsGroup("other", SettingScope::Game, "a", {{"a", "A", false, {{"other.count", "2"}}}}));
        report_case("setting a member of a group with no editable preset is an error", throws([&] { test_settings.set_value<double>("other.count", 3); })
            && test_settings.value<double>("other.count") == 2);

        test_groups.insert_preset("difficulty", {"nightmare", "Nightmare", false, {{"difficulty.count", "3"}}});
        test_groups.select("difficulty", "nightmare");
        report_case("a preset inserted later may be selected", test_settings.value<double>("difficulty.count") == 3
            && test_settings.value<double>("difficulty.speed") == 1);
        return;
    }

    //--------------------------------------------------------------------------
    void test_process_store()
    {
        report_group("the process-wide store");

        report_case("is one store", &store() == &store());
        report_case("and has one set of groups", &groups() == &groups());
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_text();
        test_values();
        test_types();
        test_scopes();
        test_notifications();
        test_pre_change_listeners();
        test_groups();
        test_process_store();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "exception: " << e.what() << escape_reset_string << endl;
        return 1;
    }

    return report_results();
}
