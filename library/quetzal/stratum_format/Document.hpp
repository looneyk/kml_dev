#if !defined(QUETZAL_STRATUM_FORMAT_DOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_DOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// Document.hpp
//------------------------------------------------------------------------------

#include "Notation.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace quetzal::stratum_format
{

    constexpr std::string_view format_name = "quetzal.discrete_layout";
    constexpr std::string_view format_version = "0.4";

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
        values_type components;
    };

    //--------------------------------------------------------------------------
    struct Wall
    {
        WallAddress address;
        values_type components;
    };

    //--------------------------------------------------------------------------
    struct FlatProperties
    {
        std::optional<value_type> height;
        values_type components;
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
    // The site file component: what the entity is in or on, and where on it. No address is its parent
    struct Site
    {
        Relation relation = Relation::In;
        std::optional<std::variant<WallAddress, FlatCell>> address;
        std::optional<surface_point_type> position;
        std::optional<discrete_vector_type> orientation;
    };

    //--------------------------------------------------------------------------
    // One of the zone's entities. The site and parent file components are decoded into fields, which the reader validates;
    // components contains the rest as written
    struct Entity
    {
        stored_entity_id_type id = 0;
        std::string type;
        std::optional<Site> site;
        std::optional<stored_entity_id_type> parent_id;
        values_type components;
    };

    //--------------------------------------------------------------------------
    // The data structure analog of a layout file
    struct Document
    {
        std::string version;
        values_type generator;
        values_type components; // The layout's own
        std::optional<stored_entity_id_type> zone_id; // Persistent id, empty in a zone template
        Cells cells;
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
