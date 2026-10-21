# Log --- Plan

Date: 2026-10-05

A reusable log library in `library/quetzal/log/`, the target `quetzal_log` and the namespace `quetzal::log`, for every
project in the repository: a `Log` class, and one process-wide instance of it that any code writes to without being
passed it. The program installs the sinks the entries are written to. The library is thread-safe. The open items are in
`work_plan.md` (`### Log`).

## Where it is now

- **The library is built and in use,** in `library/quetzal/log/`, with `log_test`, as `## The game's use` describes.
- **catagen has no warnings,** and `stratum_sim` reports a failure by throwing or through its event queue.

## What other projects do

- **spdlog** has a registry of named loggers and a default logger, which free functions such as `spdlog::info` and
  `spdlog::warn` write to with `fmt` formatting. Its levels, from least to most severe, are trace, debug, info, warn,
  error and critical, and `off` disables a logger. A logger writes an entry only if its level is at least the logger's
  level, and checks that before formatting the message. It writes each entry to its sinks: files, rotating and daily
  files, the console, a callback, and others, each sink with a level of its own. Each sink type has a `_mt` form, which
  locks a mutex around each write, and a `_st` form for one thread. An optional asynchronous mode puts entries in a
  queue that a worker thread writes, so the thread that logs only formats and enqueues. `flush_on(level)` flushes on
  every entry at or above a level. Logging at critical does not end the program.
- **glog** has stream macros, `LOG(INFO) << ...`, with the severities INFO, WARNING, ERROR and FATAL. FATAL writes the
  entry and a stack trace, then aborts. Each severity is written to its own file in a log directory, named with the
  program, the host, the date and the process id. `VLOG(n)` writes verbose entries, enabled for each source file with
  `--vmodule`. The macros are thread-safe.
- **Unreal Engine** writes with `UE_LOG(Category, Verbosity, format, ...)`. A category, such as `LogTemp` or a game's
  own, is declared with a default verbosity and a compile-time maximum, so verbose calls compile out of shipping
  builds. The verbosities are Fatal, Error, Warning, Display, Log, Verbose and VeryVerbose, and Fatal crashes the
  program. Every entry goes through the global `GLog`, which passes it to output devices: the log file in
  `Saved/Logs`, the console, and the debugger. Entries from other threads are buffered and passed on from the game
  thread. A category's verbosity can be changed while the game runs, with the `log` console command.
- **Godot** has `print`, `printerr`, `push_warning` and `push_error` for scripts, and macros such as `WARN_PRINT` and
  `ERR_FAIL_COND` in its C++, which write an entry and, for the `ERR_FAIL` macros, return from the function. Entries go
  to the console and the editor's output panel, and, when file logging is enabled, to `user://logs/godot.log`, with the
  previous runs' files kept up to a number that a project setting sets.

Each has one log that any code reaches without passing it, and lets the program decide where entries are written.
spdlog and Unreal separate the log from its destinations, as sinks or output devices; Unreal adds categories; spdlog
checks the level before formatting, locks around each write, and leaves ending the program to the caller.

## The design

The design follows spdlog: a default log reached through a function, sinks that the program installs, spdlog's levels,
an optional level for each sink, the level checked before formatting, and a mutex around each write. It adds Unreal's
category to each entry, with an optional level for each category.

```cpp
namespace quetzal::log
{

    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        Off
    };

    // The level's name in lowercase, and the level a name is of, for settings and command-line options
    std::string_view to_string(LogLevel level);
    std::optional<LogLevel> to_log_level(std::string_view name);

    struct LogEntry
    {
        std::chrono::system_clock::time_point time;
        LogLevel level;
        std::string category; // The library or part of the program that wrote the entry: "dungen", "stratum_sim"
        std::string message;
        std::thread::id thread_id;
    };

    using log_sink_type = std::move_only_function<void(const LogEntry& entry)>;

    class Log
    {
    public:

        // A sink with no level writes every entry that its category's level passes
        void add_sink(log_sink_type sink, std::optional<LogLevel> level = std::nullopt);
        void clear_sinks();

        LogLevel level() const;
        void set_level(LogLevel level);

        // A category with no level has the log's level
        std::optional<LogLevel> category_level(std::string_view category) const;
        void set_category_level(std::string_view category, std::optional<LogLevel> level);

        // Returns true if at least one sink writes an entry of this level in this category
        bool writes(LogLevel level, std::string_view category) const;

        void write(LogLevel level, std::string_view category, std::string_view message);

        template<typename... Args>
        void warning(std::string_view category, std::format_string<Args...> format, Args&&... args);
        // trace, debug, info, error and critical likewise

    private:

        struct InstalledSink
        {
            log_sink_type sink;
            std::optional<LogLevel> level;
        };

        mutable std::mutex m_mutex;
        LogLevel m_level;
        std::map<std::string, LogLevel, std::less<>> m_category_levels; // <category, level>
        std::vector<InstalledSink> m_sinks;
        LogLevel m_lowest_sink_level; // Off with no sinks, and Trace with a sink that has no level
    };

    // The process-wide log, constructed on first use
    Log& logger();

    // Sinks: a file appended to, and a stream such as std::clog
    class FileLogSink;
    class StreamLogSink;

    // An entry as one line of text: its ISO 8601 time, its level, its category, and its message
    std::string format_entry(const LogEntry& entry);

} // namespace quetzal::log
```

- **`Log` is reusable.** A program can make one of its own, as `log_test` does to test the class. The process-wide
  instance is a `Log` like any other.
- **`logger()` returns the process-wide log,** a function-local static, which C++ constructs once, on first use,
  safely across threads, as Rust's `log::logger()` returns its global one. No code passes a log to another. Every
  call writes it out, `log::logger().warning(...)`, so a line that logs is plain to see. Free functions that forward to it,
  as spdlog's `spdlog::warn` does, can be added later without changing a call written this way.
- **The program installs the sinks.** The process-wide log has none until then, and an entry written before a sink is
  installed is dropped. A suite or a tool that needs the entries installs a sink first.
- **A sink is a function,** given each entry, so a file, a stream, Godot's output, or a suite's collection are all the
  same type. `FileLogSink` and `StreamLogSink` are function objects that own their stream; a file is opened in binary
  mode and each line ends with `\n`.
- **The log is thread-safe.** The message is formatted on the thread that logs, outside the lock. Writing the entry to
  every sink happens under the log's mutex, so lines from two threads never interleave, and a sink needs no lock of its
  own. Adding and clearing sinks, and reading and setting the levels, take the same mutex.
- **Each entry has a category,** the library or part of the program that wrote it, as an Unreal category does, so a
  log can be read or filtered by its source.
- **The log does not end the program.** A `Critical` entry is written like any other, as in spdlog; a failure is an
  assert or a thrown exception, which the code that fails raises after logging.
- **A `Log` starts at `Info`,** spdlog's default level, and `logger()` constructs the process-wide log at it.
- **The program sets the levels at startup,** where it installs the sinks. The game's levels, the log's and any
  category's or sink's, are process-wide settings, which the host applies when it starts and again whenever a setting
  changes, as Unreal's `log` console command changes a category's level while the game runs. The tools take a
  `--log-level` option, and a suite sets the levels its cases check against.

### Levels

- **The log's level is the default.** A category with no level of its own has the log's level.
- **A category's level, when set, replaces the log's level for that category.** It may be more or less severe than the
  log's: `dungen` at `Debug` in a log at `Info` has its debug entries written, and `stratum_sim` at `Error` in the same
  log has its warnings dropped.
- **A sink's level, when set, narrows what that sink writes.** A sink with no level writes every entry that its
  category's level passes.
- **An entry is written to a sink only if its level is at least the higher severity of two levels:** its category's
  level, or the log's level if the category has none, and the sink's level. With `dungen` at `Debug`, a file sink with
  no level, and a console sink at `Warning`, a `dungen` debug entry is written to the file and not to the console,
  since for the console the higher severity of `Debug` and `Warning` is `Warning`. With `stratum_sim` at `Error`, a
  `stratum_sim` warning is written to neither, since the higher severity is at least `Error` for both sinks.
- **A sink's level never widens what a category's level passes.** A sink at `Trace` writes nothing that its category's
  level drops.
- **`Off` writes nothing.** A category at `Off` has no entries written, and a sink at `Off` writes none.
- **The levels are checked before the message is formatted.** The message is formatted only if at least one sink will
  write the entry: its level is at least its category's level and at least the lowest sink level. The log caches the
  lowest sink level, the least severe of its sinks' levels, recomputing it when a sink is added or the sinks are
  cleared. A sink with no level counts as `Trace`, and a log with no sinks has `Off`, so an entry that no sink writes
  costs two comparisons and the category's lookup.

### Usage

The program installs its sinks and sets the levels once, at startup. A sink is any function the caller provides that
takes a `LogEntry`; `FileLogSink` and `StreamLogSink` are two the library provides for a file and a stream.

```cpp
#include "quetzal/log/FileLogSink.hpp"
#include "quetzal/log/Log.hpp"

using namespace quetzal;

// At startup, in the host
log::logger().set_level(log::LogLevel::Info);
log::logger().set_category_level("dungen", log::LogLevel::Debug);

// A sink of the caller's own, here Godot's output, writing only warnings and above
log::logger().add_sink([](const log::LogEntry& entry)
{
    godot::UtilityFunctions::print(log::format_entry(entry).c_str());
}, log::LogLevel::Warning);

// A sink the library provides, writing every entry its category's level passes
log::logger().add_sink(log::FileLogSink(logs_directory / "stratum_godot.log"));

// Anywhere in the program, with no log passed to it
log::logger().warning("dungen", "{} of {} staircases placed", placed_count, drawn_count);
```

A suite collects the entries that the code it tests writes to the process-wide log, and clears the sinks after:

```cpp
std::vector<log::LogEntry> entries;
log::logger().add_sink([&entries](const log::LogEntry& entry)
{
    entries.push_back(entry);
});

// ... run the generator, then check the warnings in entries

log::logger().clear_sinks();
```

## The game's use

- **The terminal installs a file sink** writing to a file in a `logs` directory in the same place as the saves and
  working directories, since anything written to the console would write over its frame.
- **Godot installs a sink that writes to Godot's output,** errors and critical entries through `push_error`, warnings
  through `push_warning` and the rest through `print`, and a file sink. It installs them when its extension initializes
  at the scene level, once for the process, and clears them when the extension uninitializes.
- **The tools install a `std::clog` sink** and take `--log-level`.
- **dungen writes its warnings to the process-wide log** under the category `dungen`, and takes no warning function.
- **`stratum_format::read_zone` writes its warnings to the process-wide log** under the category `stratum_format`,
  and takes no warning function.
- **A suite that checks warnings** adds a sink collecting them to the process-wide log, and clears the sinks after.
- **A host logs the gameplay seed** (`random_engine.md`, `## Ownership`).

## Open

The open items are tracked in `work_plan.md`, under `### Log`.
