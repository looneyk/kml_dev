//------------------------------------------------------------------------------
// log_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/log/FileLogSink.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/log/LogEntry.hpp"
#include "quetzal/log/LogLevel.hpp"
#include "quetzal/log/StreamLogSink.hpp"
#include "quetzal/log/timestamped_files.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace quetzal::log;

namespace
{

    size_t g_format_count = 0;

    // An argument that counts how often it is formatted
    struct CountedArgument
    {
    };

} // namespace

//------------------------------------------------------------------------------
template<>
struct std::formatter<CountedArgument> : std::formatter<std::string_view>
{
    auto format(const CountedArgument&, std::format_context& context) const
    {
        ++g_format_count;
        return std::formatter<std::string_view>::format("counted", context);
    }
};

namespace
{

    static const filesystem::path out_directory = "out";

    //--------------------------------------------------------------------------
    log_sink_type collecting_sink(vector<LogEntry>& entries)
    {
        return [&entries](const LogEntry& entry)
        {
            entries.push_back(entry);
        };
    }

    //--------------------------------------------------------------------------
    // The messages of the entries in the category
    vector<string> messages(const vector<LogEntry>& entries, const string& category)
    {
        vector<string> result;
        for (const auto& entry : entries)
        {
            if (entry.category == category)
            {
                result.push_back(entry.message);
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    string file_text(const filesystem::path& filepath)
    {
        ifstream is(filepath, ios_base::binary);
        return string(istreambuf_iterator<char>(is), istreambuf_iterator<char>());
    }

    //--------------------------------------------------------------------------
    TEST_CASE("levels")
    {
        bool names_round_trip = true;
        for (LogLevel level : {LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warning, LogLevel::Error, LogLevel::Critical, LogLevel::Off})
        {
            names_round_trip = names_round_trip && to_log_level(to_string(level)) == level;
        }

        CHECK_CASE("each level's name reads back as the level", names_round_trip);
        CHECK_CASE_EQUAL("a level's name is lowercase", string(to_string(LogLevel::Warning)), "warning");
        CHECK_CASE("a name that is no level's reads as none", !to_log_level("Warning") && !to_log_level("fatal") && !to_log_level(""));
        CHECK_CASE("the levels run from least to most severe", LogLevel::Trace < LogLevel::Debug && LogLevel::Debug < LogLevel::Info
            && LogLevel::Info < LogLevel::Warning && LogLevel::Warning < LogLevel::Error && LogLevel::Error < LogLevel::Critical
            && LogLevel::Critical < LogLevel::Off);

        Log log;
        CHECK_CASE("a log starts at info", log.level() == LogLevel::Info);
        CHECK_CASE("and the process-wide log is one log, at info", &logger() == &logger() && logger().level() == LogLevel::Info);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("categories")
    {
        Log log;
        vector<LogEntry> entries;
        log.add_sink(collecting_sink(entries));

        log.debug("dungen", "dropped");
        log.info("dungen", "written");
        CHECK_CASE("a category with no level has the log's level", messages(entries, "dungen") == vector<string> {"written"}
            && !log.category_level("dungen"));

        entries.clear();
        log.set_category_level("dungen", LogLevel::Debug);
        log.debug("dungen", "written");
        log.debug("stratum_sim", "dropped");
        CHECK_CASE("a category's level may be less severe than the log's", messages(entries, "dungen") == vector<string> {"written"}
            && messages(entries, "stratum_sim").empty() && log.category_level("dungen") == LogLevel::Debug);

        entries.clear();
        log.set_category_level("stratum_sim", LogLevel::Error);
        log.warning("stratum_sim", "dropped");
        log.error("stratum_sim", "written");
        CHECK_CASE("or more severe", messages(entries, "stratum_sim") == vector<string> {"written"});

        entries.clear();
        log.set_category_level("dungen", nullopt);
        log.debug("dungen", "dropped");
        log.info("dungen", "written");
        CHECK_CASE("and removing a category's level returns it to the log's", messages(entries, "dungen") == vector<string> {"written"}
            && !log.category_level("dungen"));

        entries.clear();
        log.set_level(LogLevel::Warning);
        log.info("catagen", "dropped");
        log.warning("catagen", "written");
        CHECK_CASE("the log's level is the default for every category with none", messages(entries, "catagen") == vector<string> {"written"});

        entries.clear();
        log.set_category_level("catagen", LogLevel::Off);
        log.critical("catagen", "dropped");
        CHECK_CASE("a category at off has nothing written", entries.empty());
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("sinks")
    {
        {
            Log log;
            CHECK_CASE("a log with no sinks writes nothing", !log.writes(LogLevel::Critical, "dungen"));
        }

        {
            Log log;
            vector<LogEntry> file_entries;
            vector<LogEntry> console_entries;
            log.add_sink(collecting_sink(file_entries));
            log.add_sink(collecting_sink(console_entries), LogLevel::Warning);
            log.set_category_level("dungen", LogLevel::Debug);
            log.set_category_level("stratum_sim", LogLevel::Error);

            log.debug("dungen", "debug");
            log.warning("dungen", "warning");
            CHECK_CASE("a debug entry of a category at debug is written to a sink with no level, and not to one at warning",
                messages(file_entries, "dungen") == vector<string> {"debug", "warning"}
                && messages(console_entries, "dungen") == vector<string> {"warning"});

            log.warning("stratum_sim", "warning");
            CHECK_CASE("a warning of a category at error is written to neither", messages(file_entries, "stratum_sim").empty()
                && messages(console_entries, "stratum_sim").empty());

            log.clear_sinks();
            log.critical("dungen", "critical");
            CHECK_CASE("and once the sinks are cleared, nothing is written", file_entries.size() == 2 && console_entries.size() == 1
                && !log.writes(LogLevel::Critical, "dungen"));
        }

        {
            Log log;
            vector<LogEntry> entries;
            log.add_sink(collecting_sink(entries), LogLevel::Trace);
            log.debug("dungen", "dropped");
            CHECK_CASE("a sink's level never widens what a category's level passes", entries.empty());
        }

        {
            Log log;
            vector<LogEntry> entries;
            log.add_sink(collecting_sink(entries), LogLevel::Off);
            log.critical("dungen", "dropped");
            CHECK_CASE("a sink at off writes nothing", entries.empty() && !log.writes(LogLevel::Critical, "dungen"));
        }

        {
            Log log;
            vector<LogEntry> entries;
            log.add_sink(collecting_sink(entries), LogLevel::Error);
            log.set_category_level("dungen", LogLevel::Trace);

            g_format_count = 0;
            log.warning("dungen", "{}", CountedArgument {});
            CHECK_CASE("a message is not formatted when every sink's level is more severe than its entry",
                g_format_count == 0 && entries.empty());

            log.error("dungen", "{}", CountedArgument {});
            CHECK_CASE("and is formatted once when a sink writes it", g_format_count == 1 && messages(entries, "dungen") == vector<string> {"counted"});

            log.set_category_level("dungen", LogLevel::Critical);
            log.error("dungen", "{}", CountedArgument {});
            CHECK_CASE("nor when its category's level is more severe than its entry", g_format_count == 1);
        }
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("entries")
    {
        Log log;
        vector<LogEntry> entries;
        log.add_sink(collecting_sink(entries));

        auto before = chrono::system_clock::now();
        log.warning("dungen", "{} staircases placed of {}", 1, 2);
        auto after = chrono::system_clock::now();

        CHECK_CASE("an entry has its level, category, formatted message, time, and thread", entries.size() == 1
            && entries[0].level == LogLevel::Warning && entries[0].category == "dungen" && entries[0].message == "1 staircases placed of 2"
            && entries[0].time >= before && entries[0].time <= after && entries[0].thread_id == this_thread::get_id());

        LogEntry entry;
        entry.time = chrono::sys_days {chrono::year {2026} / 10 / 5} + chrono::hours {14} + chrono::minutes {3} + chrono::seconds {12}
            + chrono::milliseconds {345} + chrono::microseconds {678};
        entry.level = LogLevel::Warning;
        entry.category = "dungen";
        entry.message = "text";
        CHECK_CASE_EQUAL("an entry formats as its UTC time to the millisecond, its level padded to the longest, its category and its message",
            format_entry(entry), "2026-10-05T14:03:12.345Z warning  dungen: text");

        entry.level = LogLevel::Critical;
        CHECK_CASE_EQUAL("the longest level has one space after it", format_entry(entry), "2026-10-05T14:03:12.345Z critical dungen: text");
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("stream and file sinks")
    {
        LogEntry entry;
        entry.time = chrono::sys_days {chrono::year {2026} / 10 / 5};
        entry.level = LogLevel::Info;
        entry.category = "stratum_terminal";
        entry.message = "seed 20260928";

        ostringstream os;
        StreamLogSink stream_sink(os);
        stream_sink(entry);
        CHECK_CASE_EQUAL("a stream sink writes an entry as its formatted line", os.str(), format_entry(entry) + "\n");

        filesystem::path filepath = out_directory / "file_sink" / "log_test.log";
        filesystem::remove_all(out_directory / "file_sink");

        {
            FileLogSink file_sink(filepath);
            file_sink(entry);
        }

        {
            Log log;
            log.add_sink(FileLogSink(filepath));
            log.write(LogLevel::Info, "stratum_terminal", "seed 20260928");
        }

        string text = file_text(filepath);
        CHECK_CASE("a file sink creates its directory, and appends to a file that exists, each line ending in a line feed",
            text.starts_with(format_entry(entry) + "\n") && text.ends_with(" info     stratum_terminal: seed 20260928\n")
            && ranges::count(text, '\n') == 2 && ranges::count(text, '\r') == 0);

        bool thrown = false;
        try
        {
            FileLogSink directory_sink(out_directory);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        CHECK_CASE("a file sink throws std::runtime_error if its file cannot be opened", thrown);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("timestamped files")
    {
        const filesystem::path directory = out_directory / "timestamped";
        const filesystem::path base_filepath = directory / "stratum_terminal.log";
        filesystem::remove_all(directory);

        const chrono::system_clock::time_point time = chrono::sys_days {chrono::year {2026} / 10 / 22} + chrono::hours {22}
            + chrono::minutes {30} + chrono::seconds {5} + chrono::milliseconds {123};
        CHECK_CASE_EQUAL("a timestamped file has the time inserted before its extension, in UTC to the millisecond",
            timestamped_filepath(base_filepath, time).generic_string(), (directory / "stratum_terminal_20261022T223005.123Z.log").generic_string());

        filesystem::create_directories(directory);
        vector<filesystem::path> timestamped_filepaths;
        for (int i = 0; i < 5; ++i)
        {
            timestamped_filepaths.push_back(timestamped_filepath(base_filepath, time + chrono::hours {i}));
            ofstream(timestamped_filepaths.back(), ios::binary) << "file " << i << "\n";
        }

        const vector<filesystem::path> other_filepaths {timestamped_filepath(directory / "stratum_godot.log", time),
            timestamped_filepath(directory / "stratum_terminal.txt", time), base_filepath, directory / "stratum_terminal_notes.log"};
        for (const auto& filepath : other_filepaths)
        {
            ofstream(filepath, ios::binary) << "other\n";
        }

        remove_oldest_timestamped_files(base_filepath, 5);
        CHECK_CASE("no file is removed while the base path's timestamped files are within the maximum",
            ranges::all_of(timestamped_filepaths, [](const auto& filepath) { return filesystem::exists(filepath); }));

        remove_oldest_timestamped_files(base_filepath, 0);
        CHECK_CASE("a maximum of 0 removes nothing",
            ranges::all_of(timestamped_filepaths, [](const auto& filepath) { return filesystem::exists(filepath); }));

        const vector<UnremovedFile> unremoved_files = remove_oldest_timestamped_files(base_filepath, 3);
        CHECK_CASE("past the maximum, the oldest are removed and the newest kept", unremoved_files.empty()
            && !filesystem::exists(timestamped_filepaths[0]) && !filesystem::exists(timestamped_filepaths[1])
            && filesystem::exists(timestamped_filepaths[2]) && filesystem::exists(timestamped_filepaths[3]) && filesystem::exists(timestamped_filepaths[4]));
        CHECK_CASE("another base path's timestamped files and files of another form are left", ranges::all_of(other_filepaths, [](const auto& filepath)
        {
            return filesystem::exists(filepath);
        }));
        CHECK_CASE("a directory that does not exist has nothing to remove", remove_oldest_timestamped_files(directory / "absent" / "x.log", 1).empty());

        for (int i = 5; i < 8; ++i)
        {
            timestamped_filepaths.push_back(timestamped_filepath(base_filepath, time + chrono::hours {i}));
            ofstream(timestamped_filepaths.back(), ios::binary) << "file " << i << "\n";
        }

        vector<UnremovedFile> held_unremoved_files;
        {
            ofstream held_2(timestamped_filepaths[2], ios::binary | ios::app);
            ofstream held_3(timestamped_filepaths[3], ios::binary | ios::app);
            held_unremoved_files = remove_oldest_timestamped_files(base_filepath, 3);
        }

        CHECK_CASE("a file that cannot be removed, such as one held open, takes no newer file's place",
            filesystem::exists(timestamped_filepaths[5]) && filesystem::exists(timestamped_filepaths[6]) && filesystem::exists(timestamped_filepaths[7])
            && !filesystem::exists(timestamped_filepaths[4]));
        CHECK_CASE("and each file not removed is returned, and is still there", ranges::all_of(held_unremoved_files, [&](const UnremovedFile& file)
        {
            return (file.filepath == timestamped_filepaths[2] || file.filepath == timestamped_filepaths[3]) && filesystem::exists(file.filepath)
                && file.error;
        }), format("{} files not removed", held_unremoved_files.size()));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("threads")
    {
        constexpr size_t thread_count = 8;
        constexpr size_t entry_count = 1000;

        Log log;
        vector<LogEntry> entries;
        ostringstream os;
        log.add_sink(collecting_sink(entries));
        log.add_sink(StreamLogSink(os));

        {
            vector<jthread> threads;
            for (size_t t = 0; t < thread_count; ++t)
            {
                threads.emplace_back([&log, t]()
                {
                    for (size_t i = 0; i < entry_count; ++i)
                    {
                        log.info("thread", "{} {}", t, i);
                    }
                });
            }
        }

        // <thread, the next entry expected from it>
        map<size_t, size_t> next_entries;
        bool in_order = entries.size() == thread_count * entry_count;
        for (const auto& entry : entries)
        {
            size_t t = 0;
            size_t i = 0;
            istringstream(entry.message) >> t >> i;
            in_order = in_order && i == next_entries[t]++;
        }

        CHECK_CASE("entries written from several threads at once all reach a sink, each thread's in its order", in_order
            && next_entries.size() == thread_count);

        size_t whole_line_count = 0;
        istringstream lines(os.str());
        for (string line; getline(lines, line);)
        {
            size_t t = 0;
            size_t i = 0;
            char trailing = 0;
            size_t message_start = line.find("thread: ");
            istringstream message(message_start == string::npos ? string() : line.substr(message_start + 8));
            if (message >> t >> i && !(message >> trailing) && t < thread_count && i < entry_count)
            {
                ++whole_line_count;
            }
        }

        CHECK_CASE("and no two lines are interleaved", whole_line_count == thread_count * entry_count);
        return;
    }

} // namespace
