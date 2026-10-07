//------------------------------------------------------------------------------
// settings_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/Settings.hpp"
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

    const Setting<Mode> mode_setting {"gameplay.mode", Mode::Restore, SettingScope::User};
    const Setting<std::string> difficulty_setting {"gameplay.difficulty", "adventure", SettingScope::Game};
    const Setting<bool> fullscreen_setting {"video.fullscreen", false, SettingScope::User, true};
    const Setting<int> width_setting {"video.width", 1280, SettingScope::User};
    const Setting<double> volume_setting {"audio.volume", 0.8, SettingScope::User};

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
    void declare_all(Settings& test_settings)
    {
        test_settings.declare(mode_setting);
        test_settings.declare(difficulty_setting);
        test_settings.declare(fullscreen_setting);
        test_settings.declare(width_setting);
        test_settings.declare(volume_setting);
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
        report_case("a key is in each leading run of its segments but the last", sections_of("video.window.width") == vector<string_view> {"video",
            "video.window"} && sections_of("video.width") == vector<string_view> {"video"} && sections_of("width").empty());
        report_case("a setting key is in a section and has no empty segment", is_setting_key("video.window.width") && !is_setting_key("width")
            && !is_setting_key(".width") && !is_setting_key("video.") && !is_setting_key("video..width"));
        return;
    }

    //--------------------------------------------------------------------------
    void test_values()
    {
        report_group("values");

        Settings test_settings;
        declare_all(test_settings);

        report_case("a declared setting has its default until set", test_settings.value(mode_setting) == Mode::Restore
            && test_settings.value(width_setting) == 1280 && test_settings.value(volume_setting) == 0.8 && !test_settings.value(fullscreen_setting));
        report_case("and its declaration has its C++ type, default text, scope and whether a restart is required",
            test_settings.declaration("video.fullscreen").type == std::type_index(typeid(bool)) && test_settings.declaration("video.fullscreen").default_text == "false"
            && test_settings.declaration("video.fullscreen").scope == SettingScope::User && test_settings.declaration("video.fullscreen").restart_required);

        test_settings.set_value(mode_setting, Mode::Reseed);
        test_settings.set_value(width_setting, 1920);
        report_case("a value set is the value read", test_settings.value(mode_setting) == Mode::Reseed && test_settings.value(width_setting) == 1920);
        report_case("and is read by key as text", test_settings.text("gameplay.mode") == "reseed" && test_settings.text("video.width") == "1920"
            && test_settings.text("gameplay.difficulty") == "adventure");

        test_settings.set_text("audio.volume", "0.25");
        report_case("a value set by key as text is read as its type", test_settings.value(volume_setting) == 0.25);
        report_case("a key that is not declared is an error", throws([&] { test_settings.text("video.height"); })
            && throws([&] { test_settings.set_text("video.height", "720"); }) && !test_settings.declares("video.height"));
        report_case("and so is text that is no value of the setting's type", throws([&] { test_settings.set_text("video.width", "wide"); })
            && test_settings.value(width_setting) == 1920);
        return;
    }

    //--------------------------------------------------------------------------
    void test_data_declarations()
    {
        report_group("declarations in data");

        Settings test_settings;
        report_case("bool, integer, real and text are the types a declaration in data may give", test_settings.registers_type("bool")
            && test_settings.registers_type("integer") && test_settings.registers_type("real") && test_settings.registers_type("text")
            && !test_settings.registers_type("mode"));

        test_settings.declare("torches.flicker", "bool", "true", SettingScope::User, false);
        report_case("a setting declared in data has the C++ type its type name registers", test_settings.declares("torches.flicker")
            && test_settings.declaration("torches.flicker").type == std::type_index(typeid(bool)) && test_settings.text("torches.flicker") == "true");
        report_case("and a C++ declaration of that key and type reads it", test_settings.value(Setting<bool> {"torches.flicker", true}));

        test_settings.register_type<Mode>("mode");
        test_settings.declare("torches.mode", "mode", "reseed", SettingScope::Game, false);
        report_case("a type a library registers, an enumeration's among them, may be given by a declaration in data",
            test_settings.declaration("torches.mode").type == std::type_index(typeid(Mode)) && test_settings.text("torches.mode") == "reseed"
            && throws([&] { test_settings.set_text("torches.mode", "sometimes"); }));

        report_case("a type name that is not registered is an error", throws([&] { test_settings.declare("torches.colour", "colour", "red",
            SettingScope::User, false); }));
        report_case("and so is a default that is no value of the type", throws([&] { test_settings.declare("torches.count", "integer", "many",
            SettingScope::User, false); }));
        report_case("and a key declared twice, or with no section", throws([&] { test_settings.declare("torches.flicker", "bool", "false",
            SettingScope::User, false); }) && throws([&] { test_settings.declare("flicker", "bool", "false", SettingScope::User, false); }));

        test_settings.clear();
        report_case("clearing the store keeps the four types it starts with, and no other", test_settings.registers_type("bool")
            && !test_settings.registers_type("mode") && !test_settings.declares("torches.flicker"));
        return;
    }

    //--------------------------------------------------------------------------
    void test_scopes()
    {
        report_group("scopes");

        Settings test_settings;
        declare_all(test_settings);

        test_settings.set_value(difficulty_setting, std::string("hardcore"));
        test_settings.set_value(fullscreen_setting, true);

        quetzal::KeyValueStore user_values = test_settings.values(SettingScope::User);
        quetzal::KeyValueStore game_values = test_settings.values(SettingScope::Game);
        report_case("the values of a scope are those set in it, as text", user_values.value("video.fullscreen") == "true"
            && !user_values.contains("gameplay.difficulty") && !user_values.contains("video.width") && game_values.value("gameplay.difficulty") == "hardcore");

        test_settings.reset(SettingScope::Game);
        quetzal::KeyValueStore game_values_reset = test_settings.values(SettingScope::Game);
        report_case("resetting a scope returns its settings to their defaults, and leaves the other's", test_settings.value(difficulty_setting) == "adventure"
            && test_settings.value(fullscreen_setting) && game_values_reset.begin() == game_values_reset.end());

        test_settings.set_values(game_values);
        report_case("and setting the values saved restores them", test_settings.value(difficulty_setting) == "hardcore");

        quetzal::KeyValueStore partly_wrong {{"video.width", "1600"}, {"video.height", "900"}};
        report_case("values with a key that is not declared are an error, and none of them is set", throws([&] { test_settings.set_values(partly_wrong); })
            && test_settings.value(width_setting) == 1280);
        return;
    }

    //--------------------------------------------------------------------------
    void test_notifications()
    {
        report_group("change notifications");

        Settings test_settings;
        declare_all(test_settings);

        size_t video_applied = 0;
        size_t gameplay_applied = 0;
        test_settings.add_section_listener("video", [&video_applied] { ++video_applied; });
        test_settings.add_section_listener("gameplay", [&gameplay_applied] { ++gameplay_applied; });

        test_settings.set_value(width_setting, 1920);
        report_case("a change notifies its section", video_applied == 1 && gameplay_applied == 0);

        test_settings.set_value(width_setting, 1920);
        report_case("and setting the same value again does not", video_applied == 1);

        test_settings.set_values({{"video.width", "800"}, {"video.fullscreen", "true"}, {"gameplay.mode", "reseed"}});
        report_case("setting values notifies each section changed once", video_applied == 2 && gameplay_applied == 1);

        test_settings.reset(SettingScope::User);
        report_case("and so does resetting a scope", video_applied == 3 && gameplay_applied == 2);

        // The listener reads the new value, as a subsystem applying its section does
        int width_applied = 0;
        test_settings.add_section_listener("video", [&test_settings, &width_applied] { width_applied = test_settings.value(width_setting); });
        test_settings.set_value(width_setting, 2560);
        report_case("a listener reads the value already changed", width_applied == 2560);
        return;
    }

    //--------------------------------------------------------------------------
    void test_nested_sections()
    {
        report_group("nested sections");

        const Setting<int> window_width_setting {"video.window.width", 1280};
        const Setting<int> window_height_setting {"video.window.height", 720};

        Settings test_settings;
        test_settings.declare(fullscreen_setting);
        test_settings.declare(window_width_setting);
        test_settings.declare(window_height_setting);

        size_t video_applied = 0;
        size_t window_applied = 0;
        test_settings.add_section_listener("video", [&video_applied] { ++video_applied; });
        test_settings.add_section_listener("video.window", [&window_applied] { ++window_applied; });

        test_settings.set_value(window_width_setting, 1920);
        report_case("a change notifies each section its key is in, the outer as well as the inner", video_applied == 1 && window_applied == 1);

        test_settings.set_value(fullscreen_setting, true);
        report_case("and not a section nested in one of them that its key is not in", video_applied == 2 && window_applied == 1);

        test_settings.set_values({{"video.window.width", "800"}, {"video.window.height", "600"}, {"video.fullscreen", "false"}});
        report_case("values set together notify each section once, however many of its keys changed", video_applied == 3 && window_applied == 2);
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
        test_data_declarations();
        test_scopes();
        test_notifications();
        test_nested_sections();
        test_process_store();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "exception: " << e.what() << escape_reset_string << endl;
        return 1;
    }

    return report_results();
}
