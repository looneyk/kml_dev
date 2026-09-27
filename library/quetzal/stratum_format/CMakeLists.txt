add_library(quetzal_stratum_format STATIC
    Document.cpp
    grid.cpp
    read.cpp
    references.cpp
    RelocatorTarget.cpp
    write.cpp
    Document.hpp
    Notation.hpp
    grid.hpp
    read.hpp
    references.hpp
    RelocatorTarget.hpp
    types.hpp
    write.hpp
)

set_property(TARGET quetzal_stratum_format PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_format PUBLIC
    ${DEV_ROOT}/library
)

# Not nlohmann_json::nlohmann_json, as library3 is added after library/ and a namespaced target must already exist
target_link_libraries(quetzal_stratum_format PUBLIC quetzal_common nlohmann_json)
