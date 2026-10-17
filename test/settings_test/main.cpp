//------------------------------------------------------------------------------
// settings_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/Settings.hpp"
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
        test_settings.insert("gameplay.mode", "mode", "restore", SettingScope::User, false);
        test_settings.insert("gameplay.difficulty", "text", "adventure", SettingScope::Game, false);
        test_settings.insert("video.fullscreen", "boolean", "false", SettingScope::User, true);
        test_settings.insert("video.width", "integer", "1280", SettingScope::User, false);
        test_settings.insert("audio.volume", "real", "0.8", SettingScope::User, false);
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

        test_settings.insert("torches.flicker", "boolean", "true", SettingScope::User, false);
        report_case("a setting is read as the C++ type of its type", test_settings.contains("torches.flicker")
            && test_settings.value<bool>("torches.flicker") && test_settings.text("torches.flicker") == "true");

        test_settings.register_type<Mode>("mode", {"restore", "reseed"});
        test_settings.insert("torches.mode", "mode", "reseed", SettingScope::Game, false);
        report_case("a type a library registers, an enumeration among them, may be a setting's type",
            test_settings.type("mode").cpp_type == type_index(typeid(Mode)) && test_settings.value<Mode>("torches.mode") == Mode::Reseed
            && throws([&] { test_settings.set_text("torches.mode", "sometimes"); }));
        report_case("and an enumeration in C++ has its values, and is based on text", test_settings.type("mode").values
            == vector<string> {"restore", "reseed"} && test_settings.type("mode").base_type == SettingBaseType::Text);

        test_settings.register_enumeration("torches.style", {"steady", "flicker", "gutter"});
        test_settings.insert("torches.style", "torches.style", "flicker", SettingScope::User, false);
        test_settings.set_text("torches.style", "gutter");
        report_case("an enumeration defined in data is text, limited to its values", test_settings.type("torches.style").cpp_type
            == type_index(typeid(std::string)) && test_settings.type("torches.style").base_type == SettingBaseType::Text
            && test_settings.value<std::string>("torches.style") == "gutter" && throws([&] { test_settings.set_text("torches.style", "dim"); }));
        report_case("and one registered twice, or with no values, is an error", throws([&] { test_settings.register_enumeration("torches.style",
            {"steady"}); }) && throws([&] { test_settings.register_enumeration("torches.shade", {}); }));

        report_case("a type name that is not registered is an error", throws([&] { test_settings.insert("torches.colour", "colour", "red",
            SettingScope::User, false); }));
        report_case("and so is a default that is no value of the type", throws([&] { test_settings.insert("torches.count", "integer", "many",
            SettingScope::User, false); }));
        report_case("and a key inserted twice, or empty", throws([&] { test_settings.insert("torches.flicker", "boolean", "false",
            SettingScope::User, false); }) && throws([&] { test_settings.insert("", "boolean", "false", SettingScope::User, false); }));

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

        test_settings.insert("flicker", "boolean", "true", SettingScope::User, false);
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
        test_process_store();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "exception: " << e.what() << escape_reset_string << endl;
        return 1;
    }

    return report_results();
}
