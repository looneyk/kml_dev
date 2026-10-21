//------------------------------------------------------------------------------
// settings_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/SettingGroup.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
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
#include <utility>
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
    // Returns true if the function throws std::runtime_error with the message
    template<typename F>
    bool throws_with(F function, string_view message)
    {
        try
        {
            function();
        }
        catch (const runtime_error& e)
        {
            return e.what() == message;
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
            && game_values_reset.empty());

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
    void test_listeners()
    {
        report_group("listeners");

        Settings test_settings;
        insert_all(test_settings);

        size_t removed_count = 0;
        Settings::listener_id_type listener_id = test_settings.add_change_listener([&removed_count](const vector<string>&) { ++removed_count; });
        test_settings.set_value<int64_t>("video.width", 1920);
        test_settings.remove_listener(listener_id);
        test_settings.set_value<int64_t>("video.width", 1600);
        report_case("a listener removed is not called", removed_count == 1);

        size_t change_count = 0;
        test_settings.add_change_listener([&change_count](const vector<string>&) { ++change_count; });
        test_settings.clear();
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.set_value<int64_t>("video.width", 1920);
        report_case("clearing the store keeps its listeners", change_count == 1);
        return;
    }

    //--------------------------------------------------------------------------
    using presets_type = vector<pair<string, Preset>>;

    //--------------------------------------------------------------------------
    void insert_difficulty(Settings& test_settings)
    {
        test_settings.insert("difficulty.count", {.type_name = "real", .default_value = "1", .scope = SettingScope::Game, .maximum = "4"});
        test_settings.insert("difficulty.speed", {.type_name = "real", .default_value = "1", .scope = SettingScope::Game});
        test_settings.insert_group("difficulty", SettingScope::Game, "normal", {"difficulty.count", "difficulty.speed"}, {
            {"easy", {"Easy", false, quetzal::KeyValueStore {{"difficulty.count", "0.5"}, {"difficulty.speed", "0.75"}}}},
            {"normal", {"Normal", false, quetzal::KeyValueStore {{"difficulty.count", "1.2"}, {"difficulty.speed", "1"}}}},
            {"hard", {"Hard", false, quetzal::KeyValueStore {{"difficulty.count", "1.5"}, {"difficulty.speed", "1.25"}}}},
            {"custom", {"Custom", true}}}, "Difficulty", "difficulty");
        return;
    }


    //--------------------------------------------------------------------------
    void test_groups()
    {
        report_group("setting groups");

        Settings test_settings;
        insert_difficulty(test_settings);
        const SettingType& difficulty_type = test_settings.type(test_settings.setting("difficulty").type_name);
        report_case("a group is a setting of a group type whose values are its presets' names, with its display name and category",
            test_settings.contains_group("difficulty") && difficulty_type.group
            && difficulty_type.values == vector<string> {"easy", "normal", "hard", "custom"}
            && test_settings.setting("difficulty").display_name == "Difficulty" && test_settings.setting("difficulty").category == "difficulty"
            && test_settings.group("difficulty").member_keys == vector<string> {"difficulty.count", "difficulty.speed"}
            && test_settings.member_group_key("difficulty.count") == "difficulty" && !test_settings.member_group_key("difficulty").has_value());
        report_case("and has its default preset selected, whose values its members have, while each keeps a default of its own",
            test_settings.text("difficulty") == "normal" && test_settings.value<double>("difficulty.count") == 1.2
            && test_settings.value<double>("difficulty.speed") == 1 && test_settings.setting("difficulty.count").default_value == "1");

        vector<vector<string>> notifications;
        test_settings.add_change_listener([&notifications](const vector<string>& keys_changed) { notifications.push_back(keys_changed); });

        test_settings.set_text("difficulty", "hard");
        report_case("selecting a preset changes its members to its values, with one notification of the group and each member changed",
            test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 1.25
            && notifications.size() == 1 && sorted(notifications.back()) == vector<string> {"difficulty", "difficulty.count", "difficulty.speed"});

        SettingsSnapshot hard = SettingsSnapshot::capture(SettingScope::Game, test_settings);
        report_case("a preset that is not editable is saved as the group's value alone", hard.settings.value("difficulty") == "hard"
            && hard.settings.value("difficulty.count") == nullopt && hard.presets.empty());

        test_settings.set_value<double>("difficulty.speed", 2);
        report_case("setting a member selects the editable preset with the selected preset's values, and sets the member there",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5
            && test_settings.value<double>("difficulty.speed") == 2 && notifications.size() == 2
            && sorted(notifications.back()) == vector<string> {"difficulty", "difficulty.speed"});

        const Settings::preset_key_type custom_key {"difficulty", "custom"};
        SettingsSnapshot custom = SettingsSnapshot::capture(SettingScope::Game, test_settings);
        report_case("an editable preset is saved with its values", custom.settings.value("difficulty") == "custom"
            && custom.presets.at(custom_key).value("difficulty.count") == "1.5" && custom.presets.at(custom_key).value("difficulty.speed") == "2");

        test_settings.set_text("difficulty", "easy");
        test_settings.set_text("difficulty", "custom");
        report_case("selecting another preset and then the editable preset again restores its values",
            test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 2);

        test_settings.set_text("difficulty", "hard");
        test_settings.set_value<double>("difficulty.count", 3);
        report_case("but setting a member under another preset replaces them with that preset's, then sets the member",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 3
            && test_settings.value<double>("difficulty.speed") == 1.25);

        test_settings.set_text("difficulty", "normal");
        SettingsSnapshot normal = SettingsSnapshot::capture(SettingScope::Game, test_settings);
        report_case("the editable preset's values are kept and saved while another preset is selected",
            test_settings.value<double>("difficulty.count") == 1.2 && normal.settings.value("difficulty") == "normal"
            && normal.presets.at(custom_key).value("difficulty.count") == "3");

        test_settings.reset(SettingScope::Game);
        const quetzal::KeyValueStore game_values_reset = test_settings.values(SettingScope::Game);
        const bool reset_to_default = test_settings.text("difficulty") == "normal" && game_values_reset.empty()
            && test_settings.preset_values(SettingScope::Game).empty();
        test_settings.set_text("difficulty", "hard");
        test_settings.set_text("difficulty", "custom");
        report_case("resetting a scope selects its groups' default presets, and leaves editable presets undefined until selected",
            reset_to_default && test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 1.25);

        test_settings.reset(SettingScope::Game);
        custom.restore(SettingScope::Game, test_settings);
        report_case("restoring the values saved restores the selection and the editable preset's values",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5
            && test_settings.value<double>("difficulty.speed") == 2);

        report_case("restoring values saved in another scope is an error",
            throws_with([&] { custom.restore(SettingScope::User, test_settings); }, "'difficulty' is not a user setting group")
            && throws_with([&] { SettingsSnapshot {.settings = {{"difficulty", "hard"}}}.restore(SettingScope::User, test_settings); },
            "'difficulty' is not a user setting"));
        SettingsSnapshot out_of_range {.settings = {{"difficulty", "hard"}},
            .presets = {{custom_key, quetzal::KeyValueStore {{"difficulty.count", "5"}, {"difficulty.speed", "1"}}}}};
        report_case("and a preset's value out of its range, which names the setting, and selects and sets nothing",
            throws_with([&] { out_of_range.restore(SettingScope::Game, test_settings); }, "'5' is out of the range of the setting 'difficulty.count'")
            && test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5);

        SettingsSnapshot incomplete {.presets = {{custom_key, quetzal::KeyValueStore {{"difficulty.count", "2"}}}}};
        SettingsSnapshot not_editable {.presets = {{{"difficulty", "hard"}, quetzal::KeyValueStore {{"difficulty.count", "2"},
            {"difficulty.speed", "1"}}}}};
        report_case("and an editable preset's values that leave out a member, or values of a preset that is not editable",
            throws_with([&] { incomplete.restore(SettingScope::Game, test_settings); },
            "the preset 'custom' of the setting group 'difficulty' does not set its member 'difficulty.speed'")
            && throws_with([&] { not_editable.restore(SettingScope::Game, test_settings); },
            "the preset 'hard' of the setting group 'difficulty' is not editable"));

        SettingsSnapshot member_value {.settings = {{"difficulty", "hard"}, {"difficulty.count", "2"}}};
        member_value.restore(SettingScope::Game, test_settings);
        report_case("a member's value restored is set as setting the member sets it, after the group's selection",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 2
            && test_settings.value<double>("difficulty.speed") == 1.25);

        report_case("a preset that is not one of the group's is an error", throws_with([&] { test_settings.set_text("difficulty", "nightmare"); },
            "'nightmare' is not a valid value of the setting 'difficulty'"));

        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.insert("other.count", {.type_name = "real", .default_value = "2", .scope = SettingScope::Game, .maximum = "4"});
        const Preset other_a {"A", false, quetzal::KeyValueStore {{"other.count", "2"}}};
        auto insert_other = [&test_settings](const vector<string>& member_keys, const presets_type& presets, string_view default_preset_name = "a")
        {
            test_settings.insert_group("other", SettingScope::Game, default_preset_name, member_keys, presets);
        };

        report_case("a group whose key is a setting's is an error", throws([&] { test_settings.insert_group("difficulty", SettingScope::Game, "a",
            {"other.count"}, {{"a", other_a}}); }));
        report_case("and so is a group with no members or a member twice, or a default preset that is missing or editable",
            throws([&] { insert_other({}, {{"a", other_a}}); })
            && throws([&] { insert_other({"other.count", "other.count"}, {{"a", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}}, "b"); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", true}}}); }));
        report_case("and a member that is not a setting, is another scope's, is another group's member, or is a group",
            throws([&] { insert_other({"other.size"}, {{"a", {"A", false, quetzal::KeyValueStore {{"other.size", "1"}}}}}); })
            && throws([&] { insert_other({"video.width"}, {{"a", {"A", false, quetzal::KeyValueStore {{"video.width", "800"}}}}}); })
            && throws([&] { insert_other({"difficulty.count"}, {{"a", {"A", false, quetzal::KeyValueStore {{"difficulty.count", "1"}}}}}); })
            && throws_with([&] { insert_other({"difficulty"}, {{"a", {"A", false, quetzal::KeyValueStore {{"difficulty", "hard"}}}}}); },
            "the member 'difficulty' of the setting group 'other' is a setting group"));
        report_case("and a preset that leaves out a member, sets a key that is not one, or sets no value of its type or one out of its range",
            throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", false, quetzal::KeyValueStore {}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore {{"other.count", "2"}, {"video.width", "800"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore {{"other.count", "fast"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore {{"other.count", "5"}}}}}); }));
        report_case("and a preset named twice or with no name, a second editable preset, an editable preset with values, or one that is not and has none",
            throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"a", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", true}}, {"c", {"C", true}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", true, quetzal::KeyValueStore {{"other.count", "2"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B"}}}); }));
        report_case("and none of them inserts anything", !test_settings.contains("other") && !test_settings.contains_group("other")
            && !test_settings.registered_type("other") && !test_settings.member_group_key("other.count").has_value());

        insert_other({"other.count"}, {{"a", other_a}});
        report_case("setting a member of a group with no editable preset is an error", throws([&] { test_settings.set_value<double>("other.count", 3); })
            && test_settings.value<double>("other.count") == 2);
        return;
    }

    //--------------------------------------------------------------------------
    void test_process_store()
    {
        report_group("the process-wide store");

        report_case("is one store", &store() == &store());
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
        test_listeners();
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
