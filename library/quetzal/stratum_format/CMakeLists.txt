add_library(quetzal_stratum_format STATIC
    Document.cpp
    grid.cpp
    read.cpp
    references.cpp
    write.cpp
    Document.hpp
    grid.hpp
    read.hpp
    references.hpp
    types.hpp
    write.hpp
)

set_property(TARGET quetzal_stratum_format PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_format PUBLIC
    ${DEV_ROOT}/library
)

# Plain name rather than nlohmann_json::nlohmann_json: library3 is added after library/ in the root
# CMakeLists, and a namespaced target must already exist when it is referenced
target_link_libraries(quetzal_stratum_format PUBLIC quetzal_common nlohmann_json)
