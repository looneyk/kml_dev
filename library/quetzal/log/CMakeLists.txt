add_library(quetzal_log STATIC
    FileLogSink.cpp
    Log.cpp
    LogEntry.cpp
    LogLevel.cpp
    StreamLogSink.cpp
    FileLogSink.hpp
    Log.hpp
    LogEntry.hpp
    LogLevel.hpp
    StreamLogSink.hpp
)

set_property(TARGET quetzal_log PROPERTY FOLDER "library")

target_include_directories(quetzal_log PUBLIC
    ${DEV_ROOT}/library
)

find_package(Threads REQUIRED)
target_link_libraries(quetzal_log PUBLIC Threads::Threads)
