add_library(quetzal_stratum_core STATIC
    AccessPoint.cpp
    Directionality.cpp
    Notation.cpp
    RegionTree.cpp
    TransitEndpoint.cpp
    Worldspace.cpp
    ZoneExtendedName.cpp
    ZoneSpec.cpp
    AccessPoint.hpp
    Directionality.hpp
    Notation.hpp
    PersistentId.hpp
    Region.hpp
    RegionExtent.hpp
    RegionSpec.hpp
    RegionTree.hpp
    spec_keys.hpp
    TransitEndpoint.hpp
    types.hpp
    Worldspace.hpp
    Zone.hpp
    ZoneExtendedName.hpp
    ZoneSpec.hpp
)

set_property(TARGET quetzal_stratum_core PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_core PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_stratum_core PUBLIC quetzal_common quetzal_math)
target_link_libraries(quetzal_stratum_core PRIVATE quetzal_log)
