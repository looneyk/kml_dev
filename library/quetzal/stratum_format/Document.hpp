#if !defined(QUETZAL_STRATUM_FORMAT_DOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_DOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// Document.hpp
//------------------------------------------------------------------------------

#include "Notation.hpp"
#include "RelocatorTarget.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace quetzal::stratum_format
{

    constexpr std::string_view format_name = "quetzal.discrete_layout";
    constexpr std::string_view format_version = "0.6";

    //--------------------------------------------------------------------------
    // How the grid rows are written. Rows are stored expanded either way, so this affects only what
    // a writer produces
    enum class Encoding
    {
        Characters,
        Runs
    };

    //--------------------------------------------------------------------------
    enum class FlatRole
    {
        Floor,
        Ceiling
    };

    //--------------------------------------------------------------------------
    // The relative position of one entity to another
    enum class Relation
    {
        In,
        On
    };

    //--------------------------------------------------------------------------
    // The wall on a face of a cell. The normal is the vector pointing toward a viewer who is facing the wall
    struct WallAddress
    {
        discrete_point_type cell_position;
        discrete_vector_type normal;

        friend bool operator==(const WallAddress&, const WallAddress&) = default;
    };

    //--------------------------------------------------------------------------
    // A virtual block/wall used to constrain the fill when identifying room cells
    struct Barrier
    {
        discrete_point_type cell_position;
        std::optional<discrete_vector_type> normal;

        friend bool operator==(const Barrier&, const Barrier&) = default;
    };

    //--------------------------------------------------------------------------
    // The data structure for the cell grid
    struct Cells
    {
        Notation notation = Notation::Wall;
        Encoding encoding = Encoding::Characters;
        discrete_vector_type extent;
        point_type origin;
        value_type scale = value_type(1);

        // Grid rows, uncompressed, from highest to lowest.
        std::vector<std::string> rows;
    };

    //--------------------------------------------------------------------------
    // A room perimeter polygon, one of the two ways to describe room extent
    struct Perimeter
    {
        std::vector<discrete_point_type> junctions;
    };

    //--------------------------------------------------------------------------
    // An arbitrary cell in the room and barriers to indicate closing off any open egress,
    // supports flood fill to enumerate room cells, one of the two ways to describe room extent
    struct RoomLocator
    {
        discrete_point_type cell_position;
        std::vector<Barrier> barriers;
    };

    //--------------------------------------------------------------------------
    // A named area over the grid, identified one way or the other and never both
    struct Room
    {
        std::variant<Perimeter, RoomLocator> identification;
        raw_values_type components;
    };

    //--------------------------------------------------------------------------
    struct Wall
    {
        WallAddress address;
        raw_values_type components;
    };

    //--------------------------------------------------------------------------
    struct FlatProperties
    {
        std::optional<value_type> height;
        raw_values_type components;
    };

    //--------------------------------------------------------------------------
    struct Flat
    {
        FlatRole role = FlatRole::Floor;
        FlatProperties properties; // Flat-specific or default cell properties
        std::map<id_type, FlatProperties> rooms;
        std::map<discrete_point_type, FlatProperties> cells;
    };

    //--------------------------------------------------------------------------
    struct FlatCell
    {
        FlatRole role = FlatRole::Floor;
        discrete_point_type cell_position;
    };

    //--------------------------------------------------------------------------
    // A cell of the zone that an entity is in, such as an actor standing on it, rather than a surface it is on
    struct ZoneCell
    {
        discrete_point_type cell_position;
    };

    //--------------------------------------------------------------------------
    // The site file component: the object the entity is in or on, and where on it. With no address, the entity is sited on
    // its parent
    struct Site
    {
        Relation relation = Relation::In;
        std::optional<std::variant<WallAddress, FlatCell, ZoneCell>> address;
        std::optional<surface_point_type> position;
        std::optional<discrete_vector_type> orientation;
    };

    //--------------------------------------------------------------------------
    // The access_point file component. The target is unbound, or in a saved zone the id of the access point it is bound to.
    // An access point with no target is arrival only
    struct AccessPoint
    {
        using target_type = std::variant<RelocatorTarget, stored_entity_id_type>;

        std::string type;
        std::uint64_t order = 0;
        std::optional<target_type> target;
    };

    //--------------------------------------------------------------------------
    // One of the zone's entities. The site, parent and access_point file components are decoded into fields, which the
    // reader validates; components contains the rest as written
    struct Entity
    {
        stored_entity_id_type id = 0;
        std::string type;
        std::optional<Site> site;
        std::optional<stored_entity_id_type> parent_id;
        std::optional<AccessPoint> access_point;
        raw_values_type components;
    };

    //--------------------------------------------------------------------------
    // The generator that produced a zone, and the parameters of its run as the tokens its options take
    struct Generator
    {
        std::string name;
        KeyValueStore parameters;
    };

    //--------------------------------------------------------------------------
    // The data structure analog of a layout file
    struct Document
    {
        std::string version;
        std::optional<Generator> generator;
        std::vector<stored_entity_id_type> access_points;
        std::optional<zone_position_type> position;
        bool is_unpopulated = false;
        raw_values_type components; // Other components
        std::optional<stored_entity_id_type> zone_id; // Persistent id, empty in a zone template
        std::optional<KeyValueStore> zone_spec;
        std::optional<Cells> cells;
        std::vector<Barrier> barriers;
        std::vector<Room> rooms;
        std::vector<Wall> walls;
        std::vector<Flat> flats;
        std::vector<Entity> entities;
    };

    std::string_view name(Encoding encoding);
    std::string_view name(FlatRole role);
    std::string_view name(Relation relation);

    std::optional<Encoding> to_encoding(std::string_view name);
    std::optional<FlatRole> to_flat_role(std::string_view name);
    std::optional<Relation> to_relation(std::string_view name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_DOCUMENT_HPP
