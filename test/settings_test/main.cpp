//------------------------------------------------------------------------------
// settings_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/SettingGroup.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
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

    // An enumeration with a setting, which has a std::formatter and a from_chars, the two functions an enumeration written as
    // text requires
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
                return {first + mode_name(candidate).size(), std::errc{}};
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

    // The scopes these tests use; the store has no scope of its own
    const std::string user_scope = "user";
    const std::string game_scope = "game";

    //--------------------------------------------------------------------------
    void insert_all(Settings& test_settings)
    {
        test_settings.register_type<Mode>("mode", {"restore", "reseed"});
        test_settings.insert("gameplay.mode", {.type_name = "mode", .default_value = "restore", .scope = user_scope, .modifiable = true});
        test_settings.insert("gameplay.difficulty", {.type_name = "text", .default_value = "adventure", .scope = game_scope,
            .modifiable = true});
        test_settings.insert("video.fullscreen", {.type_name = "boolean", .default_value = "false", .scope = user_scope, .restart_required = true,
            .modifiable = true});
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280", .scope = user_scope, .modifiable = true,
            .minimum = "640", .maximum = "7680"});
        test_settings.insert("audio.volume", {.type_name = "real", .default_value = "0.8", .scope = user_scope, .modifiable = true,
            .minimum = "0", .maximum = "1"});
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("text")
    {
        CHECK_CASE("a boolean is true or false", quetzal::to_string(true) == "true" && quetzal::try_type<bool>("false") == false
            && !quetzal::try_type<bool>("yes"));
        CHECK_CASE("an integer reads back, and text with anything after the number is none", quetzal::try_type<int>(quetzal::to_string(-42))
            == -42 && !quetzal::try_type<int>("42x") && !quetzal::try_type<int>(""));
        CHECK_CASE("a real is the shortest text that reads back as the same value", quetzal::to_string(0.1) == "0.1"
            && quetzal::try_type<double>("0.1") == 0.1 && !quetzal::try_type<double>("one"));
        CHECK_CASE("a string is its own text", quetzal::try_type<std::string>("adventure") == "adventure");
        CHECK_CASE("an enumeration's value is the text its formatter writes and its from_chars reads", quetzal::to_string(Mode::Reseed) == "reseed"
            && quetzal::try_type<Mode>("restore") == Mode::Restore && !quetzal::try_type<Mode>("Reseed") && !quetzal::try_type<Mode>("reseeded"));
        CHECK_CASE("the types a setting may have are those that format and parse", SettingValue<bool> && SettingValue<int> && SettingValue<double>
            && SettingValue<std::string> && SettingValue<Mode> && !SettingValue<std::type_index>);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("values")
    {
        Settings test_settings;
        insert_all(test_settings);

        CHECK_CASE("a setting has its default until set", test_settings.value<Mode>("gameplay.mode") == Mode::Restore
            && test_settings.value<int64_t>("video.width") == 1280 && test_settings.value<double>("audio.volume") == 0.8
            && !test_settings.value<bool>("video.fullscreen"));
        CHECK_CASE("and has its type name, default text, scope and whether a restart is required",
            test_settings.setting("video.fullscreen").type_name == "boolean" && test_settings.setting("video.fullscreen").default_value == "false"
            && test_settings.setting("video.fullscreen").scope == user_scope && test_settings.setting("video.fullscreen").restart_required);

        test_settings.set_value<Mode>("gameplay.mode", Mode::Reseed);
        test_settings.set_value<int64_t>("video.width", 1920);
        CHECK_CASE("a value set is the value read", test_settings.value<Mode>("gameplay.mode") == Mode::Reseed
            && test_settings.value<int64_t>("video.width") == 1920);
        CHECK_CASE("and is read by key as text", test_settings.text("gameplay.mode") == "reseed" && test_settings.text("video.width") == "1920"
            && test_settings.text("gameplay.difficulty") == "adventure");

        test_settings.set_text("audio.volume", "0.25");
        CHECK_CASE("a value set by key as text is read as its type", test_settings.value<double>("audio.volume") == 0.25);
        CHECK_CASE("a key that is not a setting's is an error", throws([&] { test_settings.text("video.height"); })
            && throws([&] { test_settings.set_text("video.height", "720"); }) && !test_settings.contains("video.height"));
        CHECK_CASE("and so is text that is no value of the setting's type", throws([&] { test_settings.set_text("video.width", "wide"); })
            && test_settings.value<int64_t>("video.width") == 1920);

        CHECK_CASE("a value out of range sets nothing, and returns false", !test_settings.set_values({{"video.width", "800"}, {"audio.volume", "1.5"}})
            && !test_settings.set_value<int64_t>("video.width", 100) && test_settings.value<int64_t>("video.width") == 1920
            && test_settings.value<double>("audio.volume") == 0.25);
        CHECK_CASE("and a value in range returns true", test_settings.set_value<int64_t>("video.width", 640) && test_settings.bounded_inclusive("audio.volume", "1")
            && !test_settings.bounded_inclusive("audio.volume", "-0.5") && test_settings.bounded_inclusive("gameplay.difficulty", "anything"));
        CHECK_CASE("a setting's range is checked when it is inserted", throws([&] { test_settings.insert("video.height", {.type_name = "integer",
            .default_value = "100", .minimum = "480"}); }) && throws([&] { test_settings.insert("video.title", {.type_name = "text",
            .default_value = "a", .maximum = "z"}); }) && throws([&] { test_settings.insert("video.height", {.type_name = "integer",
            .default_value = "720", .maximum = "tall"}); }));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("types")
    {
        Settings test_settings;
        CHECK_CASE("boolean, integer, real and text are the types the store registers, with their C++ types",
            test_settings.type("boolean").cpp_type == type_index(typeid(bool)) && test_settings.type("integer").cpp_type == type_index(typeid(int64_t))
            && test_settings.type("real").cpp_type == type_index(typeid(double)) && test_settings.type("text").cpp_type == type_index(typeid(std::string))
            && !test_settings.registered_type("mode"));
        CHECK_CASE("and each is its own base type, with no values", test_settings.type("boolean").base_type == SettingBaseType::Boolean
            && test_settings.type("integer").base_type == SettingBaseType::Integer && test_settings.type("real").base_type == SettingBaseType::Real
            && test_settings.type("text").base_type == SettingBaseType::Text && test_settings.type("integer").values.empty());

        test_settings.insert("torches.flicker", {.type_name = "boolean", .default_value = "true"});
        CHECK_CASE("a setting is read as the C++ type of its type", test_settings.contains("torches.flicker")
            && test_settings.value<bool>("torches.flicker") && test_settings.text("torches.flicker") == "true");

        test_settings.register_type<Mode>("mode", {"restore", "reseed"});
        test_settings.insert("torches.mode", {.type_name = "mode", .default_value = "reseed", .scope = game_scope, .modifiable = true});
        CHECK_CASE("a type a library registers, an enumeration among them, may be a setting's type",
            test_settings.type("mode").cpp_type == type_index(typeid(Mode)) && test_settings.value<Mode>("torches.mode") == Mode::Reseed
            && throws([&] { test_settings.set_text("torches.mode", "sometimes"); }));
        CHECK_CASE("and an enumeration in C++ has its values, and is based on text", test_settings.type("mode").values
            == vector<string>{"restore", "reseed"} && test_settings.type("mode").base_type == SettingBaseType::Text);

        test_settings.register_enumeration("torches.style", {"steady", "flicker", "gutter"});
        test_settings.insert("torches.style", {.type_name = "torches.style", .default_value = "flicker", .modifiable = true});
        test_settings.set_text("torches.style", "gutter");
        CHECK_CASE("an enumeration defined in data is text, limited to its values", test_settings.type("torches.style").cpp_type
            == type_index(typeid(std::string)) && test_settings.type("torches.style").base_type == SettingBaseType::Text
            && test_settings.value<std::string>("torches.style") == "gutter" && throws([&] { test_settings.set_text("torches.style", "dim"); }));
        CHECK_CASE("and one registered twice, or with no values, is an error", throws([&] { test_settings.register_enumeration("torches.style",
            {"steady"}); }) && throws([&] { test_settings.register_enumeration("torches.shade", {}); }));

        CHECK_CASE("a type name that is not registered is an error", throws([&] { test_settings.insert("torches.color", {.type_name = "color",
            .default_value = "red"}); }));
        CHECK_CASE("and so is a default that is no value of the type", throws([&] { test_settings.insert("torches.count", {.type_name = "integer",
            .default_value = "many"}); }));
        CHECK_CASE("and a key inserted twice, or empty", throws([&] { test_settings.insert("torches.flicker", {.type_name = "boolean",
            .default_value = "false"}); }) && throws([&] { test_settings.insert("", {.type_name = "boolean", .default_value = "false"}); }));

        test_settings.clear();
        CHECK_CASE("clearing the store keeps only the four types it starts with", test_settings.registered_type("boolean")
            && !test_settings.registered_type("mode") && !test_settings.contains("torches.flicker"));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("scopes")
    {
        Settings test_settings;
        insert_all(test_settings);

        test_settings.set_value<std::string>("gameplay.difficulty", "hardcore");
        test_settings.set_value<bool>("video.fullscreen", true);

        quetzal::KeyValueStore user_values = test_settings.values(user_scope);
        quetzal::KeyValueStore game_values = test_settings.values(game_scope);
        CHECK_CASE("the values of a scope are those set in it, as text", user_values.value("video.fullscreen") == "true"
            && !user_values.contains("gameplay.difficulty") && !user_values.contains("video.width") && game_values.value("gameplay.difficulty") == "hardcore");

        test_settings.reset(game_scope);
        quetzal::KeyValueStore game_values_reset = test_settings.values(game_scope);
        CHECK_CASE("resetting a scope returns its settings to their defaults, and leaves the other's",
            test_settings.value<std::string>("gameplay.difficulty") == "adventure" && test_settings.value<bool>("video.fullscreen")
            && game_values_reset.empty());

        test_settings.set_values(game_values);
        CHECK_CASE("and setting the values saved restores them", test_settings.value<std::string>("gameplay.difficulty") == "hardcore");

        quetzal::KeyValueStore partly_wrong{{"video.width", "1600"}, {"video.height", "900"}};
        CHECK_CASE("values with a key that is not a setting's are an error, and none of them is set",
            throws([&] { test_settings.set_values(partly_wrong); }) && test_settings.value<int64_t>("video.width") == 1280);

        test_settings.insert("torches.flicker", {.type_name = "boolean", .default_value = "true", .scope = "mod.torches", .modifiable = true});
        test_settings.set_text("torches.flicker", "false");
        CHECK_CASE("a scope is any name the client chooses, and a store has a scope for each name its settings have",
            test_settings.values("mod.torches").value("torches.flicker") == "false" && !test_settings.values(user_scope).contains("torches.flicker")
            && !test_settings.values(game_scope).contains("torches.flicker"));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("change notifications")
    {
        Settings test_settings;
        insert_all(test_settings);

        vector<vector<string>> notifications;
        test_settings.add_change_listener([&notifications](const vector<string>& keys_changed) { notifications.push_back(keys_changed); });

        test_settings.set_value<int64_t>("video.width", 1920);
        CHECK_CASE("a change calls each listener with the key changed", notifications == vector<vector<string>>{{"video.width"}});

        test_settings.set_value<int64_t>("video.width", 1920);
        CHECK_CASE("and setting the same value again does not", notifications.size() == 1);

        const vector<string> keys_set{"gameplay.mode", "video.fullscreen", "video.width"};
        test_settings.set_values({{"video.width", "800"}, {"video.fullscreen", "true"}, {"gameplay.mode", "reseed"}});
        CHECK_CASE("setting values calls each listener once, with every key changed", notifications.size() == 2
            && sorted(notifications.back()) == keys_set);

        test_settings.reset(user_scope);
        CHECK_CASE("and so does resetting a scope", notifications.size() == 3 && sorted(notifications.back()) == keys_set);

        test_settings.insert("flicker", {.type_name = "boolean", .default_value = "true", .modifiable = true});
        test_settings.set_text("flicker", "false");
        CHECK_CASE("a key is notified as a whole, whatever its segments", notifications.back() == vector<string>{"flicker"});

        // The listener reads the new value, as a subsystem applying its settings does
        int64_t width_applied = 0;
        test_settings.add_change_listener([&test_settings, &width_applied](const vector<string>&)
        {
            width_applied = test_settings.value<int64_t>("video.width");
        });
        test_settings.set_value<int64_t>("video.width", 2560);
        CHECK_CASE("a listener reads the value already changed", width_applied == 2560);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("listeners")
    {
        Settings test_settings;
        insert_all(test_settings);

        size_t removed_count = 0;
        Settings::listener_id_type listener_id = test_settings.add_change_listener([&removed_count](const vector<string>&) { ++removed_count; });
        test_settings.set_value<int64_t>("video.width", 1920);
        test_settings.remove_listener(listener_id);
        test_settings.set_value<int64_t>("video.width", 1600);
        CHECK_CASE("a listener removed is not called", removed_count == 1);

        size_t change_count = 0;
        test_settings.add_change_listener([&change_count](const vector<string>&) { ++change_count; });
        test_settings.clear();
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280", .modifiable = true});
        test_settings.set_value<int64_t>("video.width", 1920);
        CHECK_CASE("clearing the store keeps its listeners", change_count == 1);
        return;
    }

    //--------------------------------------------------------------------------
    using presets_type = vector<pair<string, Preset>>;

    //--------------------------------------------------------------------------
    void insert_difficulty(Settings& test_settings)
    {
        test_settings.insert("difficulty.count", {.type_name = "real", .default_value = "1", .scope = game_scope, .modifiable = true,
            .maximum = "4"});
        test_settings.insert("difficulty.speed", {.type_name = "real", .default_value = "1", .scope = game_scope, .modifiable = true});
        test_settings.insert_group("difficulty", game_scope, true, "normal", {"difficulty.count", "difficulty.speed"}, {
            {"easy", {"Easy", false, quetzal::KeyValueStore{{"difficulty.count", "0.5"}, {"difficulty.speed", "0.75"}}}},
            {"normal", {"Normal", false, quetzal::KeyValueStore{{"difficulty.count", "1.2"}, {"difficulty.speed", "1"}}}},
            {"hard", {"Hard", false, quetzal::KeyValueStore{{"difficulty.count", "1.5"}, {"difficulty.speed", "1.25"}}}},
            {"custom", {"Custom", true}}}, "Difficulty", "difficulty");
        return;
    }


    //--------------------------------------------------------------------------
    TEST_CASE("setting groups")
    {
        Settings test_settings;
        insert_difficulty(test_settings);
        const SettingType& difficulty_type = test_settings.type(test_settings.setting("difficulty").type_name);
        CHECK_CASE("a group is a setting of a group type whose values are its presets' names, with its display name and category",
            test_settings.contains_group("difficulty") && difficulty_type.group
            && difficulty_type.values == vector<string>{"easy", "normal", "hard", "custom"}
            && test_settings.setting("difficulty").display_name == "Difficulty" && test_settings.setting("difficulty").category == "difficulty"
            && test_settings.group("difficulty").member_keys == vector<string>{"difficulty.count", "difficulty.speed"}
            && test_settings.member_group_key("difficulty.count") == "difficulty" && !test_settings.member_group_key("difficulty").has_value());
        CHECK_CASE("and has its default preset selected, whose values its members have, while each keeps a default of its own",
            test_settings.text("difficulty") == "normal" && test_settings.value<double>("difficulty.count") == 1.2
            && test_settings.value<double>("difficulty.speed") == 1 && test_settings.setting("difficulty.count").default_value == "1");

        vector<vector<string>> notifications;
        test_settings.add_change_listener([&notifications](const vector<string>& keys_changed) { notifications.push_back(keys_changed); });

        test_settings.set_text("difficulty", "hard");
        CHECK_CASE("selecting a preset changes its members to its values, with one notification of the group and each member changed",
            test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 1.25
            && notifications.size() == 1 && sorted(notifications.back()) == vector<string>{"difficulty", "difficulty.count", "difficulty.speed"});

        SettingsSnapshot hard = SettingsSnapshot::capture(game_scope, test_settings);
        CHECK_CASE("a preset that is not modifiable is saved as the group's value alone", hard.settings.value("difficulty") == "hard"
            && hard.settings.value("difficulty.count") == nullopt && hard.presets.empty());

        test_settings.set_value<double>("difficulty.speed", 2);
        CHECK_CASE("setting a member selects the modifiable preset with the selected preset's values, and sets the member there",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5
            && test_settings.value<double>("difficulty.speed") == 2 && notifications.size() == 2
            && sorted(notifications.back()) == vector<string>{"difficulty", "difficulty.speed"});

        const Settings::preset_key_type custom_key{"difficulty", "custom"};
        SettingsSnapshot custom = SettingsSnapshot::capture(game_scope, test_settings);
        CHECK_CASE("a modifiable preset is saved with its values", custom.settings.value("difficulty") == "custom"
            && custom.presets.at(custom_key).value("difficulty.count") == "1.5" && custom.presets.at(custom_key).value("difficulty.speed") == "2");

        test_settings.set_text("difficulty", "easy");
        test_settings.set_text("difficulty", "custom");
        CHECK_CASE("selecting another preset and then the modifiable preset again restores its values",
            test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 2);

        test_settings.set_text("difficulty", "hard");
        test_settings.set_value<double>("difficulty.count", 3);
        CHECK_CASE("but setting a member under another preset replaces them with that preset's, then sets the member",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 3
            && test_settings.value<double>("difficulty.speed") == 1.25);

        test_settings.set_text("difficulty", "normal");
        SettingsSnapshot normal = SettingsSnapshot::capture(game_scope, test_settings);
        CHECK_CASE("the modifiable preset's values are kept and saved while another preset is selected",
            test_settings.value<double>("difficulty.count") == 1.2 && normal.settings.value("difficulty") == "normal"
            && normal.presets.at(custom_key).value("difficulty.count") == "3");

        test_settings.reset(game_scope);
        const quetzal::KeyValueStore game_values_reset = test_settings.values(game_scope);
        const bool reset_to_default = test_settings.text("difficulty") == "normal" && game_values_reset.empty()
            && test_settings.preset_values(game_scope).empty();
        test_settings.set_text("difficulty", "hard");
        test_settings.set_text("difficulty", "custom");
        CHECK_CASE("resetting a scope selects its groups' default presets, and leaves modifiable presets undefined until selected",
            reset_to_default && test_settings.value<double>("difficulty.count") == 1.5 && test_settings.value<double>("difficulty.speed") == 1.25);

        test_settings.reset(game_scope);
        custom.restore(game_scope, test_settings);
        CHECK_CASE("restoring the values saved restores the selection and the modifiable preset's values",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5
            && test_settings.value<double>("difficulty.speed") == 2);

        CHECK_CASE("restoring values saved in another scope is an error",
            throws_with([&] { custom.restore(user_scope, test_settings); }, "'difficulty' is not a setting group of the scope 'user'")
            && throws_with([&] { SettingsSnapshot{.settings = {{"difficulty", "hard"}}}.restore(user_scope, test_settings); },
            "'difficulty' is not a setting of the scope 'user'"));
        SettingsSnapshot out_of_range{.settings = {{"difficulty", "hard"}},
            .presets = {{custom_key, quetzal::KeyValueStore{{"difficulty.count", "5"}, {"difficulty.speed", "1"}}}}};
        CHECK_CASE("and a preset's value out of its range, which names the setting, and selects and sets nothing",
            throws_with([&] { out_of_range.restore(game_scope, test_settings); }, "'5' is out of the range of the setting 'difficulty.count'")
            && test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 1.5);

        SettingsSnapshot incomplete{.presets = {{custom_key, quetzal::KeyValueStore{{"difficulty.count", "2"}}}}};
        SettingsSnapshot not_modifiable{.presets = {{{"difficulty", "hard"}, quetzal::KeyValueStore{{"difficulty.count", "2"},
            {"difficulty.speed", "1"}}}}};
        CHECK_CASE("and a modifiable preset's values that leave out a member, or values of a preset that is not modifiable",
            throws_with([&] { incomplete.restore(game_scope, test_settings); },
            "the preset 'custom' of the setting group 'difficulty' does not set its member 'difficulty.speed'")
            && throws_with([&] { not_modifiable.restore(game_scope, test_settings); },
            "the preset 'hard' of the setting group 'difficulty' is not modifiable"));

        SettingsSnapshot member_value{.settings = {{"difficulty", "hard"}, {"difficulty.count", "2"}}};
        member_value.restore(game_scope, test_settings);
        CHECK_CASE("a member's value restored is set as setting the member sets it, after the group's selection",
            test_settings.text("difficulty") == "custom" && test_settings.value<double>("difficulty.count") == 2
            && test_settings.value<double>("difficulty.speed") == 1.25);

        CHECK_CASE("a preset that is not one of the group's is an error", throws_with([&] { test_settings.set_text("difficulty", "nightmare"); },
            "'nightmare' is not a valid value of the setting 'difficulty'"));

        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.insert("other.count", {.type_name = "real", .default_value = "2", .scope = game_scope, .modifiable = true,
            .maximum = "4"});
        const Preset other_a{"A", false, quetzal::KeyValueStore{{"other.count", "2"}}};
        auto insert_other = [&test_settings](const vector<string>& member_keys, const presets_type& presets, string_view default_preset_name = "a")
        {
            test_settings.insert_group("other", game_scope, true, default_preset_name, member_keys, presets);
        };

        CHECK_CASE("a group whose key is a setting's is an error", throws([&] { test_settings.insert_group("difficulty", game_scope, true, "a",
            {"other.count"}, {{"a", other_a}}); }));
        CHECK_CASE("and so is a group with no members or a member twice, or a default preset that is missing or modifiable",
            throws([&] { insert_other({}, {{"a", other_a}}); })
            && throws([&] { insert_other({"other.count", "other.count"}, {{"a", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}}, "b"); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", true}}}); }));
        CHECK_CASE("and a member that is not a setting, is another scope's, is another group's member, or is a group",
            throws([&] { insert_other({"other.size"}, {{"a", {"A", false, quetzal::KeyValueStore{{"other.size", "1"}}}}}); })
            && throws([&] { insert_other({"video.width"}, {{"a", {"A", false, quetzal::KeyValueStore{{"video.width", "800"}}}}}); })
            && throws([&] { insert_other({"difficulty.count"}, {{"a", {"A", false, quetzal::KeyValueStore{{"difficulty.count", "1"}}}}}); })
            && throws_with([&] { insert_other({"difficulty"}, {{"a", {"A", false, quetzal::KeyValueStore{{"difficulty", "hard"}}}}}); },
            "the member 'difficulty' of the setting group 'other' is a setting group"));
        CHECK_CASE("and a preset that leaves out a member, sets a key that is not one, or sets no value of its type or one out of its range",
            throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", false, quetzal::KeyValueStore{}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore{{"other.count", "2"}, {"video.width", "800"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore{{"other.count", "fast"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", {"A", false, quetzal::KeyValueStore{{"other.count", "5"}}}}}); }));
        CHECK_CASE("and a preset named twice or with no name, a second modifiable preset, a modifiable preset with values, or one that is not and has none",
            throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"a", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"", other_a}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", true}}, {"c", {"C", true}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B", true, quetzal::KeyValueStore{{"other.count", "2"}}}}}); })
            && throws([&] { insert_other({"other.count"}, {{"a", other_a}, {"b", {"B"}}}); }));
        CHECK_CASE("and none of them inserts anything", !test_settings.contains("other") && !test_settings.contains_group("other")
            && !test_settings.registered_type("other") && !test_settings.member_group_key("other.count").has_value());

        insert_other({"other.count"}, {{"a", other_a}});
        CHECK_CASE("setting a member of a group with no modifiable preset is an error", throws([&] { test_settings.set_value<double>("other.count", 3); })
            && test_settings.value<double>("other.count") == 2);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("modifiable settings")
    {
        Settings test_settings;
        test_settings.insert("world.seed", {.type_name = "integer", .default_value = "1", .scope = game_scope});
        test_settings.insert("video.width", {.type_name = "integer", .default_value = "1280"});
        test_settings.insert("video.height", {.type_name = "integer", .default_value = "720", .modifiable = true});

        CHECK_CASE("a setting of either scope that is not modifiable cannot be set, and keeps its value",
            throws_with([&] { test_settings.set_text("world.seed", "2"); }, "the setting 'world.seed' is not modifiable")
            && throws([&] { test_settings.set_text("video.width", "1920"); }) && test_settings.text("world.seed") == "1"
            && test_settings.text("video.width") == "1280");
        CHECK_CASE("while a modifiable one can", test_settings.set_text("video.height", "1080") && test_settings.text("video.height") == "1080");
        CHECK_CASE("initializing values sets a setting whether or not it is modifiable",
            test_settings.initialize_values({{"world.seed", "2"}, {"video.width", "1920"}}, {}) && test_settings.text("world.seed") == "2"
            && test_settings.text("video.width") == "1920");

        test_settings.insert("rules.count", {.type_name = "real", .default_value = "1", .scope = game_scope, .modifiable = true});
        test_settings.insert("rules.speed", {.type_name = "real", .default_value = "1", .scope = game_scope});
        test_settings.insert_group("rules", game_scope, false, "normal", {"rules.count", "rules.speed"}, {
            {"normal", {"Normal", false, quetzal::KeyValueStore{{"rules.count", "1"}, {"rules.speed", "1"}}}},
            {"hard", {"Hard", false, quetzal::KeyValueStore{{"rules.count", "2"}, {"rules.speed", "2"}}}},
            {"custom", {"Custom", true}}});
        const Settings::preset_key_type custom_key{"rules", "custom"};
        CHECK_CASE("a group that is not modifiable has its selection and its modifiable preset's values fixed",
            throws_with([&] { test_settings.set_text("rules", "hard"); }, "the setting 'rules' is not modifiable")
            && throws_with([&] { test_settings.set_values({}, {{custom_key, quetzal::KeyValueStore{{"rules.count", "3"}, {"rules.speed", "1"}}}}); },
            "the setting group 'rules' is not modifiable"));
        CHECK_CASE("and its members fixed, a modifiable one included", throws_with([&] { test_settings.set_text("rules.count", "3"); },
            "the setting 'rules.count' is not modifiable since its setting group 'rules' is not modifiable") && test_settings.text("rules") == "normal");

        test_settings.insert("modes.count", {.type_name = "real", .default_value = "1", .scope = game_scope, .modifiable = true});
        test_settings.insert("modes.speed", {.type_name = "real", .default_value = "1", .scope = game_scope});
        test_settings.insert_group("modes", game_scope, true, "normal", {"modes.count", "modes.speed"}, {
            {"normal", {"Normal", false, quetzal::KeyValueStore{{"modes.count", "1"}, {"modes.speed", "1"}}}},
            {"custom", {"Custom", true}}});
        test_settings.set_text("modes.count", "3");
        CHECK_CASE("in a modifiable group with a modifiable preset, a modifiable member can be set, and one that is not cannot",
            test_settings.text("modes") == "custom" && test_settings.text("modes.count") == "3"
            && throws_with([&] { test_settings.set_text("modes.speed", "3"); }, "the setting 'modes.speed' is not modifiable")
            && test_settings.text("modes.speed") == "1");
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("the process-wide store")
    {
        CHECK_CASE("is one store", &store() == &store());
        return;
    }

} // namespace
