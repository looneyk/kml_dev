add_library(quetzal_common STATIC
    Arguments.cpp
    Color.cpp
    ComException.cpp
    DateTime.cpp
    error_util.cpp
    Exception.cpp
    file_util.cpp
    FileValidator.cpp
    Flags.cpp
    KeyValueStore.cpp
    logger.cpp
    LogLevel.cpp
    Platform.cpp
    process.cpp
    Properties.cpp
    string_util.cpp
    system_path.cpp
    Timer.cpp
    Timestamp.cpp
    Arguments.hpp
    Color.hpp
    com_ptr.hpp
    ComException.hpp
    Config.hpp
    DateTime.hpp
    Elements.hpp
    ElementsConstIterator.hpp
    ElementsIterator.hpp
    error_util.hpp
    Exception.hpp
    Factory.hpp
    FactoryRegistering.hpp
    file_util.hpp
    FileValidator.hpp
    Flags.hpp
    id.hpp
    KeyValueStore.hpp
    Log.hpp
    logger.hpp
    LogLevel.hpp
    Platform.hpp
    process.hpp
    Properties.hpp
    Singleton.hpp
    string_util.hpp
    system_path.hpp
    Timer.hpp
    Timestamp.hpp
)

set_property(TARGET quetzal_common PROPERTY FOLDER "library")

target_include_directories(quetzal_common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
