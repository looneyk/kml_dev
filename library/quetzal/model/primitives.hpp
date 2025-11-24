#if !defined(QUETZAL_MODEL_PRIMITIVES_HPP)
#define QUETZAL_MODEL_PRIMITIVES_HPP
//------------------------------------------------------------------------------
// model
// primitives.hpp
//------------------------------------------------------------------------------

#include "SurfaceName.hpp"
#include "geometry.hpp"
#include "mesh_attributes.hpp"
#include "primitives_specification.hpp"
#include "primitives_util.hpp"
#include "vertices_attributes.hpp"
#include "quetzal/brep/Submesh.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/brep/inversion.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/geometry/Path.hpp"
#include "quetzal/geometry/Polyline.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/PolygonWithHoles.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <format>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <cassert>

namespace quetzal::model
{

    // Primitives are created with planar, but not necessarily triangulated faces
    // Any normals and texture coordinates are continuous within each surface
    // All angles and intervals that represent angles are measured in turns, 0..1 representing one turn and/or full range
    // Cylindrical coordinates: radius, azimuth, z (r, phi, z) with azimuth interval 0..1 -> phi: 0..2Pi
    // Spherical coordinates: radius, azimuth, elevation (r, phi, theta) with azimuth interval 0..1 -> phi: 0..2Pi; elevation interval 0..1 -> theta: -Pi/2..Pi/2 ({0, 0, -1}..{0, 0, 1})
    // Each primitive given a unique submesh name produces a single submesh with that name
    // Primitives given a submesh name that already exists in the mesh will be merged into that submesh

    // The unqualified versions of these functions consider the polygon to be smooth with just a single surface
    // The edges versions of these functions consider the given polygon as the cross section of a prism, one surface per polygon edge
    // The sections versions of these functions allow for arbitrary surface division of the polygon edges
    // Each of the above check that the polygon sections fit the request
    // The auto functions look at the polygon sections and call the correct implementation

    // Note that cusps at the ends of cylinders or swept volumes are supported, but not internally (where a parametric funtion for radius or scale returns 0 at a point other than at the ends) ...
    // Note that the reference point for polygon scaling and rotation is the origin ...

    // Create a grid in the xy-plane centered at the origin with nx rows and ny columns
    // Surfaces: single "grid" or per face "grid_<i>_<j>"
    template<typename M>
    id_type create_grid(M& mesh, const std::string& name, size_t nx, size_t ny, data_value_type<M> dx, data_value_type<M> dy, bool bSmooth);

    // Create a hexagonal grid in the xy-plane centered at the origin in a roughly circular pattern with hexagons of radius rHexagon
    // Surfaces: "hex_<n>"
    template<typename M>
    id_type create_hexagonal_grid(M& mesh, const std::string& name, size_t nRadial, data_value_type<M> rHexagon);

    // Create a hexagonal grid in the xy-plane centered at the origin in a rectangular pattern with hexagons of radius rHexagon
    // Surfaces: "hex_<n>"
    // Not implemented ...
    // Needs 2 nx values or row difference and offset ...
    template<typename M>
    id_type create_hexagonal_grid(M& mesh, const std::string& name, size_t nx, size_t ny, data_value_type<M> rHexagon);

    // Create a hexagonal grid in the xy-plane centered at the origin with hexagons of radius rHexagon
    // The three n parameters are the number of hexagons along the three orthogonal edges
    // Surfaces: "hex_<n>"
    // Not implemented ...
    template<typename M>
    id_type create_hexagonal_grid(M& mesh, const std::string& name, size_t n0, size_t n1, size_t n2, data_value_type<M> rHexagon);

    // there's also a pattern with two edge lengths that alternate along the six edges ...
    // n0 n1 n0 n1 n0 n1 instead of n0 n1 n2 n0 n1 n2
    // should hex orientation be supported? ...

    // private use by create_hexagonal_grid ...
    template<typename M>
    id_type hex_partner_id(M& mesh, size_t i, size_t j, size_t n, id_type id, size_t wRow, size_t wRowPrev, bool bShiftRight);

    // Create a disk in the xy-plane centered at the origin with nAzimuth divisions
    // Surfaces: "disk"
    template<typename M>
    id_type create_disk(M& mesh, const std::string& name, size_t nAzimuth, size_t nRadial, data_value_type<M> radius, const azimuth_interval_type<M>& intervalAzimuth = math::Interval(data_value_type<M>(1)));

    // Create a box (rectangular cuboid) centered at the origin
    // d is edge length
    // Surfaces: "left", "right", "back", "front", "bottom", "top"
    template<typename M>
    id_type create_cube(M& mesh, const std::string& name, data_value_type<M> d);

    // Create a box (rectangular cuboid) centered at the origin
    // Surfaces: "bottom", "back", "right", "left", "front", "top"
    // Note that the positive axes define the directions right, forward, and up respectively
    template<typename M>
    id_type create_box(M& mesh, const std::string& name, data_value_type<M> dx, data_value_type<M> dy, data_value_type<M> dz);

    // Create a prism parallel to the z-axis centered at the origin
    // Surfaces: per side "body<i>", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_prism(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create an antiprism parallel to the z-axis centered at the origin
    // Surfaces: per side "body<i>", "end0", "end1"
    template<typename M>
    id_type create_antiprism(M& mesh, const std::string& name, size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, Properties properties = {});

    // create_cylinder_auto functions will call the smooth or edges variant based on the azimuth_smooth property

    template<typename M>
    id_type create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Create a cylinder parallel to the z-axis centered at the origin
    // The nAzimuth and nz parameters control the degree of tessellation
    // Creates a cylinder with a single smooth surface
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a cylinder parallel to the z-axis centered at the origin
    // The separate radii can be used to produce frustrum shapes
    // The nAzimuth and nz parameters control the degree of tessellation
    // Creates a cylinder with a single smooth surface
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a cylinder parallel to the z-axis centered at the origin
    // The separate radii can be used to produce frustrum shapes
    // The azimuth offset revs can be used to produce a twist, the lower and upper values applying an offset to angle calculations at the corresponding ends
    // The nAzimuth and nz parameters control the degree of tessellation
    // Creates a cylinder with a single smooth surface
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Create a cylinder parallel to the z-axis centered at the origin
    // Separate radii can be used to produce frustrum shapes
    // Azimuth offset revs can be used to produce a twist, the lower and upper values applying an offset to angle calculations at the corresponding ends
    // nAzimuth and nz parameters control the degree of tessellation
    // Creates a cylinder with a single smooth surface
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on axis and azimuth extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec);

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Each azimuth face is its own section
    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec);

    // create_cylinder_auto functions will call the smooth/edges/sections variant based on the azimuth_smooth property and/or polygon sections

    template<typename M>
    id_type create_cylinder_auto(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_auto(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec);

    // Create a cylinder given a cross section of bottom and top in the form of simple polygons
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec);

    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body<n>...", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    // Smooth not currently supported
    template<typename M>
    id_type create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec);

    template<typename M>
    id_type create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a polygon with holes
    // The nz parameter controls the degree of tessellation along with polygons
    // All polygon and hole vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", per hole "body<i>", "end0", "end1" (body<i> not yet ...)
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, const geometry::PolygonWithHoles<typename M::vector_traits>& polygons, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon with variable scaling and rotation at each z
    // fr - polygon radius function, scale factor
    // fa - polygon azimuthal rotation function
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon with variable scaling and rotation at each z
    // fr - polygon radius function, scale factor
    // fa - polygon azimuthal rotation function
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties = {});

    // Create a cylinder given a cross section in the form of a simple polygon with variable scaling and rotation at each z
    // fr - polygon radius function, scale factor
    // fa - polygon azimuthal rotation function
    // The nz parameter controls the degree of tessellation along with polygon
    // Polygon vertices should be in ccw order relative to normal vector of 2D projection
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties = {});

    // Create a cylinder given parametric functions for radius, z, and azimuth offset based on z
    // The nAzimuth and nz parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fz, std::function<data_value_type<M>(data_value_type<M>)> fa = [](){return data_value_type<M>(0);}, const Properties& properties = {});

    // Create a cylinder given parametric functions for radius, z, and azimuth offset based on phi and z
    // The nAzimuth and nz parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fz, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fa = [](){return data_value_type<M>(0);}, const Properties& properties = {});

    // Create a contoured cylinder given radii at a series of z positions
    // The nAzimuth and contour parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, const geometry::Polyline<typename M::vector_traits>& contour, const Properties& properties = {});

    // Create a contoured cylinder given radii at a series of z positions
    // The nAzimuth and contour parameters control the degree of tessellation
    // countour is a container of (z, r) pairs
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, std::span<const geometry::Point<math::CartesianTraits<data_value_type<M>, 2>>> contour, const Properties& properties = {});

    // Create an anticylinder parallel to the z-axis centered at the origin
    // The nAzimuth and nz parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1" depending on extents (body<i>? ...)
    template<typename M>
    id_type create_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create an anticylinder parallel to the z-axis centered at the origin
    // The separate radii can be used to produce anti-frustrum shapes
    // The nAzimuth and nz parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1" depending on extents (body<i>? ...)
    template<typename M>
    id_type create_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    template<typename M>
    id_type create_anticylinder(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec);

    template<typename M>
    id_type create_anticylinder_faces(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec);

    // Create a sphere centered at the origin with nAzimuth divisions in the xy-plane and nElevation divisions from the negative to the positive z-axis
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    // Sections are not supported
    // add a polygon version that does, scale follows sphere radius ...
    // maybe just do this in post-processing, there are several ways of sectioning, axis/azimuth, polygon sections or azimuth edge surfaces, ...
    template<typename M>
    id_type create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, data_value_type<M> radius, const Properties& properties = {});

    // Create a sphere centered at the origin given radii at a series of theta positions
    // The nAzimuth and contour parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_sphere(M& mesh, const std::string& name, size_t nAzimuth, const geometry::Polyline<typename M::vector_traits>& contour, const Properties& properties = {});

    // Create a sphere centered at the origin given a parametric function for radius based on v, the parameter in the elevation/theta direction
    // The nAzimuth and nElevation parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, std::function<data_value_type<M>(data_value_type<M>)> fr, const Properties& properties = {});

    // Create a sphere centered at the origin given a parametric function for radius based on u and v, the parameter in the azimuth/phi and elevation/theta directions respectively
    // The nAzimuth and nElevation parameters control the degree of tessellation
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fr, const Properties& properties = {});

    // Create a torus centered at the origin around the z-axis
    // Minor extent endpoints are measured in the circular cross section in the xz-plane centered at (rMajor, 0, 0) with normal (0, 1, 0)
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, data_value_type<M> rMajor, data_value_type<M> rMinor, const Properties& properties = {});

    // Create a torus centered at the origin around the z-axis
    // Minor extent endpoints are measured in the circular cross section in the xz-plane centered at (rMajor, 0, 0) with normal (0, 1, 0)
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, data_value_type<M> rMajor_lower, data_value_type<M> rMajor_upper, data_value_type<M> rMinor_lower, data_value_type<M> rMinor_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties = {});

    // Create a torus centered at the origin around the z-axis with parametric functions for major radius, minor radius, and z based on u and v, the parameters in the major and minor directions respectively
    // Minor extent endpoints are measured in the circular cross section in the xz-plane centered at (rMajor, 0, 0) with normal (0, 1, 0)
    // Surfaces: "body", and "end0", "end1", "side", "side0", "side1" depending on extents
    template<typename M>
    id_type create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> frMajor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> frMinor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fz, const Properties& properties = {});

    // Create an tetrahedron centered at the origin with the given circumsphere radius
    // Surfaces: per face "body<i>"
    template<typename M>
    id_type create_tetrahedron(M& mesh, const std::string& name, data_value_type<M> radius);

    // Create an octahedron centered at the origin with the given circumsphere radius
    // bVertex refers to vertical orientation, vertex or face
    // Surfaces: per face "body<i>"
    template<typename M>
    id_type create_octahedron(M& mesh, const std::string& name, data_value_type<M> radius, bool bVertex = true);

    // Create a dodecahedron centered at the origin with the given circumsphere radius
    // Surfaces: per face "body<i>"
    template<typename M>
    id_type create_dodecahedron(M& mesh, const std::string& name, data_value_type<M> radius);

    // Create an icosahedron centered at the origin with the given circumsphere radius
    // bVertex refers to vertical orientation, vertex or face
    // Surfaces: per face "body<i>"
    template<typename M>
    id_type create_icosahedron(M& mesh, const std::string& name, data_value_type<M> radius, bool bVertex = true, bool bSmooth = false);

    // Create a geodesic sphere centered at the origin with the given radius
    // bVertex refers to vertical orientation, vertex or face
    // nSubdivisions (nSegments? ...)
    // Surfaces: per face "body<i>"
    template<typename M>
    id_type create_geodesic_sphere(M& mesh, const std::string& name, data_value_type<M> radius, size_t nSubdivisions, bool bVertex = true, bool bSmooth = false);

    // single body surface instead of per side? same as below? ...

    // Create volume by extruding polygon by the displacement vector
    // Surfaces: per side "body<i>", "end0", "end1"
    template<typename M>
    id_type create_extrusion(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const normal_vector_type<M>& normal, const typename M::vector_type& displacement, const Properties& properties = {});

    // Create volume by extruding polygon by the displacement vector
    // Surfaces: "body0", per hole "body<i>", "end0", "end1"
    template<typename M>
    id_type create_extrusion(M& mesh, const std::string& name, const geometry::PolygonWithHoles<typename M::vector_traits>& polygons, const normal_vector_type<M>& normal, const typename M::vector_type& displacement, const Properties& properties = {});

    // Create volume by sweeping polygon along points of path
    // Generalized cylinder
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties = {});

    template<typename M>
    id_type create_swept_volume_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties = {});

    template<typename M>
    id_type create_swept_volume_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties = {});

    // Create volume by sweeping polygon along points of path with parametric function applied to polygon at each point along the path
    // Generalized cylinder
    // Surfaces: "body", "end0", "end1"
    // scaling  applied by f cannot be 0, we can't support cusps without accesss to a scale factor, but that is baked into f ...
    // we don't have a use case that needs more than a scale factor right now ...
    // the functions below this one can support cusps ...
    template<typename M>
    id_type create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<geometry::Polygon<typename M::vector_traits>(const geometry::Polygon<typename M::vector_traits>&, data_value_type<M>)> f, const Properties& properties = {});

    // Create volume by sweeping polygon along points of path with parametric functions for scale and rotation applied to polygon at each point along the path
    // Generalized cylinder
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties = {});

    // Create volume by sweeping polygon along points of path with parametric functions for scale and rotation applied to polygon at each point along the path
    // Generalized cylinder
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_swept_volume_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties = {});

    // Create volume by sweeping polygon along points of path with parametric functions for scale and rotation applied to polygon at each point along the path
    // Generalized cylinder
    // Surfaces: "body", "end0", "end1"
    template<typename M>
    id_type create_swept_volume_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties = {});

    // End polygon is parallel to the xy plane
    // Junction with body surface(s) is smooth
    template<typename M>
    void connect_end_round_regular(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo);

    // End polygon has no position or orientation constraints
    // Junction with body surface(s) is smooth
    template<typename M>
    void connect_end_round(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo);

    // Junction with body surface(s) is discontinuous
    template<typename M>
    void connect_end_peaked(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo, data_value_type<M> scale);

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_grid(M& mesh, const std::string& name, size_t nx, size_t ny, data_value_type<M> dx, data_value_type<M> dy, bool bSmooth)
{
    using T = M::value_type;

    T x0 = T(-0.5) * dx;
    T x1 = T(0.5) * dx;
    T y0 = T(-0.5) * dy;
    T y1 = T(0.5) * dy;

    std::vector<T> x(nx + 1);
    std::vector<T> u(nx + 1);
    for (size_t i = 0; i <= nx; ++i)
    {
        T t = T(i) / T(nx);
        x[i] = math::lerp(x0, x1, t);
        u[i] = t;
    }

    std::vector<T> y(ny + 1);
    std::vector<T> v(ny + 1);
    for (size_t i = 0; i <= ny; ++i)
    {
        T t = T(i) / T(ny);
        y[i] = math::lerp(y0, y1, t);
        v[i] = T(ny - i) / T(ny);
    }

    auto normal = normal_vector_type<M>::z_direction();
    typename M::face_attributes_type af = {normal};
    typename M::surface_attributes_type as = {normal};

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = bSmooth ? m.create_surface(submesh_id, "grid", as) : nullid;

    for (size_t j = 0; j < ny; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            size_t nh = m.halfedge_store_count();
            size_t nv = m.vertex_store_count();

            if (!bSmooth)
            {
                surface_id = m.create_surface(submesh_id, std::format("grid_{}_{}", i, j), as);
            }
            size_t face_id = m.create_face(surface_id, nh, af);

            m.create_halfedge((j == 0 ? nullid : nh - 4 * nx + 2), nh + 1, nh + 3, nv + 0, face_id);
            m.create_halfedge((i == nx - 1 ? nullid : nh + 7), nh + 2, nh + 0, nv + 1, face_id);
            m.create_halfedge((j == ny - 1 ? nullid : nh + 4 * nx), nh + 3, nh + 1, nv + 2, face_id);
            m.create_halfedge((i == 0 ? nullid : nh - 3), nh + 0, nh + 2, nv + 3, face_id);

            if (bSmooth)
            {
                m.create_vertex(nh + 0, {{x[i + 0], y[j + 0], T(0)}, normal, {u[i + 0], v[j + 0]}});
                m.create_vertex(nh + 1, {{x[i + 1], y[j + 0], T(0)}, normal, {u[i + 1], v[j + 0]}});
                m.create_vertex(nh + 2, {{x[i + 1], y[j + 1], T(0)}, normal, {u[i + 1], v[j + 1]}});
                m.create_vertex(nh + 3, {{x[i + 0], y[j + 1], T(0)}, normal, {u[i + 0], v[j + 1]}});
            }
            else
            {
                m.create_vertex(nh + 0, {{x[i + 0], y[j + 0], T(0)}, normal, {T(0), T(1)}});
                m.create_vertex(nh + 1, {{x[i + 1], y[j + 0], T(0)}, normal, {T(1), T(1)}});
                m.create_vertex(nh + 2, {{x[i + 1], y[j + 1], T(0)}, normal, {T(1), T(0)}});
                m.create_vertex(nh + 3, {{x[i + 0], y[j + 1], T(0)}, normal, {T(0), T(0)}});
            }
        }
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_hexagonal_grid(M& mesh, const std::string& name, size_t nRadial, data_value_type<M> rHexagon)
{
    using T = M::value_type;

    M m{};
    id_type submesh_id = m.create_submesh(name);

    //size_t nHexagons = nRadial * (nRadial + 1) * 3 + 1;

    size_t nhx = 2 * nRadial + 1;
    size_t nhy = 2 * nRadial + 1;

    T dx = rHexagon * sqrt(T(3)) / T(2);
    T rHexagon2 = rHexagon / T(2);

    std::array<T, 2 * nhx + 1> xs;
    for (size_t i = -nhx; i <= nhx; ++i)
    {
        xs[i] = dx * i;
    }

    size_t nhy1 = nhy + 1;
    std::array<T, 2 * nhy1> ys;
    T yMin = rHexagon * (T(1.5) * nRadial + T(1));
    for (size_t j = 0; j < nhy1; ++j)
    {
        size_t j2 = 2 * j;
        ys[j2] = yMin + T(1.5) * rHexagon * j;
        ys[j2 + 1] = ys[j2] + rHexagon2;
    }

    auto normal = normal_vector_type<M>::z_direction();
    typename M::surface_attributes_type as{normal};
    typename M::face_attributes_type af{normal};
    typename M::vertex_attributes_type av{{}, normal, {}};
    auto& position = av.position();
    auto& texcoord = av.texcoord();

    size_t ns = 0;
    size_t nh = 0;
    size_t wRowPrev = 0;

    for (size_t j = 0; j < nhy; ++j)
    {
        size_t iRow0 = j >= nRadial ? j - nRadial : nRadial - j;
        size_t wRow = nhx - iRow0;
        bool bShiftRight = j > nRadial;

        id_type idPartner = nullid;

        for (size_t i = 0; i < wRow; ++i)
        {
            id_type surface_id = m.create_surface(submesh_id, surface_name("hex", ns), as);
            id_type face_id = m.create_face(surface_id, nh, af);

            size_t i0 = iRow0 + 2 * i + 1;
            size_t j0 = 2 * j;

            position = {xs[i0], ys[j0], T(0)};
            texcoord = {T(0.5), T(0)};
            idPartner = hex_partner_id(m, i, j, 0, nh, wRow, wRowPrev, bShiftRight);
            m.create_halfedge_vertex(idPartner, nh + 1, nh + 5, face_id, av);

            position = {xs[i0 + 1], ys[j0 + 1], T(0)};
            texcoord = {T(1), T(0.25)};
            idPartner = nullid;
            m.create_halfedge_vertex(idPartner, nh + 2, nh + 0, face_id, av);

            position = {xs[i0 + 1], ys[j0 + 2], T(0)};
            texcoord = {T(1), T(0.75)};
            idPartner = nullid;
            m.create_halfedge_vertex(idPartner, nh + 3, nh + 1, face_id, av);

            position = {xs[i0], ys[j0 + 3], T(0)};
            texcoord = {T(0.5), T(1)};
            idPartner = nullid;
            m.create_halfedge_vertex(idPartner, nh + 4, nh + 2, face_id, av);

            position = {xs[i0 - 1], ys[j0 + 2], T(0)};
            texcoord = {T(0), T(0.75)};
            idPartner = hex_partner_id(m, i, j, 4, nh, wRow, wRowPrev, bShiftRight);
            m.create_halfedge_vertex(idPartner, nh + 5, nh + 3, face_id, av);

            position = {xs[i0 - 1], ys[j0 + 1], T(0)};
            texcoord = {T(0), T(0.25)};
            idPartner = hex_partner_id(m, i, j, 5, nh, wRow, wRowPrev, bShiftRight);
            m.create_halfedge_vertex(idPartner, nh + 0, nh + 4, face_id, av);

            ++ns;
            nh += 6;
        }

        wRowPrev = wRow;
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_hexagonal_grid(M& mesh, const std::string& name, size_t nx, size_t ny, data_value_type<M> rHexagon)
{
    // ...
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_hexagonal_grid(M& mesh, const std::string& name, size_t n0, size_t n1, size_t n2, data_value_type<M> rHexagon)
{
    // ...
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::hex_partner_id(M& mesh, size_t i, size_t j, size_t n, id_type id, size_t wRow, size_t wRowPrev, bool bShiftRight)
{
    id_type idPartner = nullid;

    if (n == 4) // left edge
    {
        if (i > 0)
        {
            idPartner = id - 9;
        }
    }
    else if (j > 0)
    {
        if (n == 0) // lower right edge
        {
            if (wRow >= wRowPrev)
            {
                if (i < wRow - 1 || !bShiftRight)
                {
                    idPartner = id - 6 * wRow + 9;
                }
            }
            else
            {
                idPartner = id - 6 * wRow + 3;
            }
        }
        else if (n == 5) // lower left edge
        {
            if (wRow > wRowPrev)
            {
                if (i > 0 || bShiftRight)
                {
                    idPartner = id - 6 * wRow + 2;
                }
            }
            else
            {
                idPartner = id - 6 * wRow - 4;
            }
        }
    }

    if (idPartner != nullid)
    {
        mesh.halfedge(idPartner).set_partner_id(id);
    }

    return idPartner;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_disk(M& mesh, const std::string& name, size_t nAzimuth, size_t nRadial, data_value_type<M> radius, const azimuth_interval_type<M>& intervalAzimuth)
{
    using T = M::value_type;

    assert(nRadial > 0);
    assert(nAzimuth > 2);
    assert(radius > T(0));
    assert(intervalAzimuth.proper());

    M m{};
    create_cylinder(m, name, nAzimuth, nRadial, radius, T(0), T(0), T(0), {{"axis_lower_style", "none"}});

    T radius2 = radius * T(2);

    for (auto& vertex : m.vertices())
    {
        vertex.attributes().set_texcoord({(radius + vertex.attributes().position().x()) / radius2, (radius - vertex.attributes().position().y()) / radius2});
    }

    m.rename_surface(m.surface_id(m.submesh_id(name), SurfaceName::Body), "disk");

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cube(M& mesh, const std::string& name, data_value_type<M> d)
{
    return create_box(mesh, name, d, d, d);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_box(M& mesh, const std::string& name, data_value_type<M> dx, data_value_type<M> dy, data_value_type<M> dz)
{
    using T = M::value_type;

    M m{};
    id_type submesh_id = m.create_submesh(name);

    const std::vector<std::string> names =
    {
        "bottom",
        "back",
        "right",
        "front",
        "left",
        "top"
    };

    T x1 = T(0.5) * dx;
    T y1 = T(0.5) * dy;
    T z1 = T(0.5) * dz;
    T x0 = -x1;
    T y0 = -y1;
    T z0 = -z1;

    auto normal_x = normal_vector_type<M>::x_direction();
    auto normal_y = normal_vector_type<M>::y_direction();
    auto normal_z = normal_vector_type<M>::z_direction();

    vertices_attributes_type<M> avs =
    {
        // Bottom
        {{x1, y0, z0}, -normal_z, {T(0), T(1)}},
        {{x0, y0, z0}, -normal_z, {T(1), T(1)}},
        {{x0, y1, z0}, -normal_z, {T(1), T(0)}},
        {{x1, y1, z0}, -normal_z, {T(0), T(0)}},

        // Back
        {{x0, y0, z0}, -normal_y, {T(0), T(1)}},
        {{x1, y0, z0}, -normal_y, {T(1), T(1)}},
        {{x1, y0, z1}, -normal_y, {T(1), T(0)}},
        {{x0, y0, z1}, -normal_y, {T(0), T(0)}},

        // Right
        {{x1, y0, z0}, normal_x, {T(0), T(1)}},
        {{x1, y1, z0}, normal_x, {T(1), T(1)}},
        {{x1, y1, z1}, normal_x, {T(1), T(0)}},
        {{x1, y0, z1}, normal_x, {T(0), T(0)}},

        // Front
        {{x1, y1, z0}, normal_y, {T(0), T(1)}},
        {{x0, y1, z0}, normal_y, {T(1), T(1)}},
        {{x0, y1, z1}, normal_y, {T(1), T(0)}},
        {{x1, y1, z1}, normal_y, {T(0), T(0)}},

        // Left
        {{x0, y1, z0}, -normal_x, {T(0), T(1)}},
        {{x0, y0, z0}, -normal_x, {T(1), T(1)}},
        {{x0, y0, z1}, -normal_x, {T(1), T(0)}},
        {{x0, y1, z1}, -normal_x, {T(0), T(0)}},

        // Top
        {{x0, y0, z1}, normal_z, {T(0), T(1)}},
        {{x1, y0, z1}, normal_z, {T(1), T(1)}},
        {{x1, y1, z1}, normal_z, {T(1), T(0)}},
        {{x0, y1, z1}, normal_z, {T(0), T(0)}},
    };

    for (size_t i = 0; i < 6; ++i)
    {
        typename M::face_attributes_type af {avs[i * 4].normal()};
        m.create_surface(submesh_id, names[i], af);
        m.create_face(i, i * 4, af);
    }

    for (size_t i = 0; i < 24; ++i)
    {
        m.create_vertex(i, avs[i]);
    }

    m.create_halfedge( 4,  1,  3,  0,  0);
    m.create_halfedge(16,  2,  0,  1,  0);
    m.create_halfedge(12,  3,  1,  2,  0);
    m.create_halfedge( 8,  0,  2,  3,  0);
    m.create_halfedge( 0,  5,  7,  4,  1);
    m.create_halfedge(11,  6,  4,  5,  1);
    m.create_halfedge(20,  7,  5,  6,  1);
    m.create_halfedge(17,  4,  6,  7,  1);
    m.create_halfedge( 3,  9, 11,  8,  2);
    m.create_halfedge(15, 10,  8,  9,  2);
    m.create_halfedge(21, 11,  9, 10,  2);
    m.create_halfedge( 5,  8, 10, 11,  2);
    m.create_halfedge( 2, 13, 15, 12,  3);
    m.create_halfedge(19, 14, 12, 13,  3);
    m.create_halfedge(22, 15, 13, 14,  3);
    m.create_halfedge( 9, 12, 14, 15,  3);
    m.create_halfedge( 1, 17, 19, 16,  4);
    m.create_halfedge( 7, 18, 16, 17,  4);
    m.create_halfedge(23, 19, 17, 18,  4);
    m.create_halfedge(13, 16, 18, 19,  4);
    m.create_halfedge( 6, 21, 23, 20,  5);
    m.create_halfedge(10, 22, 20, 21,  5);
    m.create_halfedge(14, 23, 21, 22,  5);
    m.create_halfedge(18, 20, 22, 23,  5);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_prism(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_prism" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "false"}} | properties);
    assert(!spec.azimuth_smooth);
    return create_cylinder_edges(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_antiprism(M& mesh, const std::string& name, size_t nAzimuth, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, Properties properties)
{
    return create_anticylinder(mesh, name, nAzimuth, 1, radius, z_lower, z_upper, Properties{{"azimuth_smooth", "false"}} | properties);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder r" << std::endl;
    return create_cylinder_auto(mesh, name, nAzimuth, nz, radius, radius, z_lower, z_upper, M::val(0), M::val(0), properties);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder rr" << std::endl;
    return create_cylinder_auto(mesh, name, nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, M::val(0), M::val(0), properties);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_auto(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder r revs" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, revs_lower, revs_upper, properties);

    if (spec.azimuth_smooth)
    {
        return create_cylinder(mesh, name, spec);
    }

    return create_cylinder_edges(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder r" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius, radius, z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "true"}} | properties);
    return create_cylinder(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder rr" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "true"}} | properties);
    return create_cylinder(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder r revs" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, revs_lower, revs_upper, Properties{{"azimuth_smooth", "true"}} | properties);
    return create_cylinder(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec)
{
//std::cout << "create_cylinder spec" << std::endl;
    using T = M::value_type;

    assert(spec.azimuth_smooth);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    auto nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = math::float_eq0(spec.axis_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.axis_radius_interval.upper());
    assert(!cusped_lower || !cusped_upper);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T r0 = spec.axis_radius_interval.lower();
    T r1 = spec.axis_radius_interval.lerp(t);

    T rev0 = spec.axis_azimuth_offset_interval.lower();
    T rev1 = spec.axis_azimuth_offset_interval.lerp(t);

    auto azimuth0 = shift(spec.azimuth_interval, rev0);
    auto azimuth1 = shift(spec.azimuth_interval, rev1);

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    auto dz = spec.axis_z_interval.length();
    auto dr = spec.axis_radius_interval.length();

    auto vertex_normal = [dr, dz](T phi)
    {
        auto cos_phi = math::denoise011(cos(phi));
        auto sin_phi = math::denoise011(sin(phi));
        normal_vector_type<M> direction = {dz * cos_phi, dz * sin_phi, -dr};
        return normalize(direction);
    };

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, face_count, r1, z0, z1, azimuth1, ts1, surface_id, vertex_normal);
    }
    else if (nz == 1 && cusped_upper)
    {
        create_apex_cusp(m, face_count, r0, z0, z1, azimuth0, ts0, surface_id, vertex_normal);
    }
    else
    {
        create_band(m, face_count, r0, r1, z0, z1, azimuth0, azimuth1, ts0, ts1, surface_id, vertex_normal);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        r1 = spec.axis_radius_interval.lerp(t);
        rev1 = spec.axis_azimuth_offset_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        azimuth0 = azimuth1;
        azimuth1 = shift(spec.azimuth_interval, rev1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            connect_apex_cusp(m, face_count, z1, azimuth1, surface_id, vertex_normal);
        }
        else
        {
            connect_band(m, face_count, r1, z1, azimuth1, ts1, surface_id, vertex_normal);
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder_edges r" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius, radius, z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "false"}} | properties);
    return create_cylinder_edges(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder_edges rr" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "false"}} | properties);
    return create_cylinder_edges(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder_edges r revs" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, revs_lower, revs_upper, Properties{{"azimuth_smooth", "false"}} | properties);
    return create_cylinder_edges(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec)
{
//std::cout << "create_cylinder_edges spec" << std::endl;
    using T = M::value_type;

    assert(!spec.azimuth_smooth);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    size_t section_count = face_count;
    size_t section_face_count = 1;

    auto nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = math::float_eq0(spec.axis_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.axis_radius_interval.upper());
    assert(!cusped_lower || !cusped_upper);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T r0 = spec.axis_radius_interval.lower();
    T r1 = spec.axis_radius_interval.lerp(t);

    T rev0 = spec.axis_azimuth_offset_interval.lower();
    T rev1 = spec.axis_azimuth_offset_interval.lerp(t);

    auto azimuth0 = shift(spec.azimuth_interval, rev0);
    auto azimuth1 = shift(spec.azimuth_interval, rev1);

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = m.surface_store_count();
    for (size_t i = 0; i < face_count; ++i)
    {
        m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, i));
    }

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp_sections(m, face_count, r1, z0, z1, azimuth1, ts1, surface_id0);
    }
    else if (nz == 1 && cusped_upper)
    {
        create_apex_cusp_sections(m, face_count, r0, z0, z1, azimuth0, ts0, surface_id0);
    }
    else
    {
        create_band_sections(m, face_count, r0, r1, z0, z1, azimuth0, azimuth1, ts0, ts1, surface_id0);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        r1 = spec.axis_radius_interval.lerp(t);
        rev1 = spec.axis_azimuth_offset_interval.lerp(t);
        azimuth1 = shift(spec.azimuth_interval, rev1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            connect_apex_cusp_sections(m, face_count, z1, azimuth1, surface_id0);
        }
        else
        {
            ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);
            connect_band_sections(m, face_count, r1, z1, azimuth1, ts1, surface_id0);
        }
    }

    // Calculate normals for the upper vertices
    size_t n = section_face_count;
    
    if (cusped_upper)
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_apex_cusp_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else if (nz == 1 && cusped_lower)
    {
        id_type id = j0_orig + face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_auto(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder polygon z" << std::endl;
    return create_cylinder(mesh, name, polygon, nz, z_lower, z_upper, M::val(1), M::val(1), M::val(0), M::val(0), properties);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_auto(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder polygon z revs" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, scale_lower, scale_upper, z_lower, z_upper, revs_lower, revs_upper, properties);

    if (spec.azimuth_smooth)
    {
        return create_cylinder(mesh, name, polygon, spec);
    }

    if (polygon.sections() && polygon.sections().value().empty())
    {
        return create_cylinder_edges(mesh, name, polygon, spec);
    }

    return create_cylinder_sections(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder polygon z" << std::endl;
    polygon.sections().reset();
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, M::val(1), M::val(1), z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "true"}} | properties);
    return create_cylinder(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder polygon z revs" << std::endl;
    polygon.sections().reset();
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, scale_lower, scale_upper, z_lower, z_upper, revs_lower, revs_upper, Properties{{"azimuth_smooth", "true"}} | properties);
    return create_cylinder(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec)
{
    using T = M::value_type;
//std::cout << "create_cylinder polygon spec" << std::endl;

    assert(spec.azimuth_smooth);
    assert(!polygon.sections());

    size_t face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T scale0 = spec.axis_radius_interval.lower();
    T scale1 = spec.axis_radius_interval.lerp(t);

    T revs0 = spec.axis_azimuth_offset_interval.lower();
    T revs1 = spec.axis_azimuth_offset_interval.lerp(t);

    bool cusped_lower = spec.axis_radius_interval.lower() == T(0);
    bool cusped_upper = spec.axis_radius_interval.upper() == T(0);
    assert(!cusped_lower || !cusped_upper);
    bool closed = true;

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);
 
    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        auto avs0 = cusp_vertices_attributes<M>(polygon, z0, ts0);
        auto avs1 = vertices_attributes<M>(polygon, z1, scale1, revs1, ts1);
        create_base_cusp(m, avs0, avs1, surface_id, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        auto avs0 = vertices_attributes<M>(polygon, z0, scale0, revs0, ts0);
        auto avs1 = cusp_vertices_attributes<M>(polygon, z1, ts1);
        create_apex_cusp(m, avs0, avs1, surface_id, closed);
    }
    else
    {
        auto avs0 = vertices_attributes<M>(polygon, z0, scale0, revs0, ts0);
        auto avs1 = vertices_attributes<M>(polygon, z1, scale1, revs1, ts1);
        create_band(m, avs0, avs1, surface_id, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        scale1 = spec.axis_radius_interval.lerp(t);
        revs1 = spec.axis_azimuth_offset_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        auto avs1 = vertices_attributes<M>(polygon, z1, scale1, revs1, ts1);
        connect_band(m, avs1, surface_id, closed);
    }

    calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties)
 {
    assert(polygon_lower.vertex_count() == polygon_upper.vertex_count());
    assert(polygon_lower.sections() == polygon_upper.sections());
    assert(!polygon_lower.sections());

    using T = M::value_type;
//std::cout << "create_cylinder polygon 2" << std::endl;

    CylinderSpecification<T> spec(polygon_lower.edge_count(), nz, T(0), T(0), T(0), T(0), properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    assert(nz > 0);

    T t = T(1) / T(nz);

    auto ts0 = texture_span<T>(0, nz, false, false);
    auto ts1 = texture_span<T>(1, nz, false, false);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    id_type j0_orig = m.halfedge_store_count();

    auto avs0 = vertices_attributes<M>(polygon_lower, std::nullopt, ts0);
    auto avs1 = vertices_attributes<M>(polygon_lower, polygon_upper, t, ts1);
    create_band(m, avs0, avs1, surface_id, true, true);

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        ts1 = texture_span<T>(i, nz, false, false);

        j0_orig = m.halfedge_store_count();

        avs1 = vertices_attributes<M>(polygon_lower, polygon_upper, t, ts1);
        connect_band(m, avs1, surface_id, true, true);
    }

    // Calculate normals for the upper vertices
    calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);

    seal_cylinder(m, submesh_id, spec, false, false);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder_edges polygon z" << std::endl;
    polygon.sections() = std::vector<id_type>();
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, M::val(1), M::val(1), z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", "false"}} | properties);
    return create_cylinder_edges(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, geometry::Polygon<typename M::vector_traits> polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder_edges polygon z scale revs" << std::endl;
    polygon.sections() = std::vector<id_type>();
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, scale_lower, scale_upper, z_lower, z_upper, revs_lower, revs_upper, Properties{{"azimuth_smooth", "false"}} | properties);
    return create_cylinder_edges(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec)
{
//std::cout << "create_cylinder_edges polygon spec" << std::endl;
    using T = M::value_type;

    assert(!spec.azimuth_smooth);
    assert(polygon.sections() && polygon.sections().value().empty());

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    size_t section_count = face_count;
    size_t section_face_count = 1;

    auto nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = math::float_eq0(spec.axis_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.axis_radius_interval.upper());
    assert(!cusped_lower || !cusped_upper);
    bool closed = true;

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T scale0 = spec.axis_radius_interval.lower();
    T scale1 = spec.axis_radius_interval.lerp(t);

    T revs0 = spec.axis_azimuth_offset_interval.lower();
    T revs1 = spec.axis_azimuth_offset_interval.lerp(t);

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    vertices_attributes_sections_type<M> avss0;
    vertices_attributes_sections_type<M> avss1;

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        avss0 = cusp_vertices_attributes_edges<M>(polygon, z0, ts0);
        avss1 = vertices_attributes_edges<M>(polygon, z1, scale1, revs1, ts1);
        create_base_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        avss0 = vertices_attributes_edges<M>(polygon, z0, scale0, revs0, ts0);
        avss1 = cusp_vertices_attributes_edges<M>(polygon, z1, ts1);
        create_apex_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else
    {
        avss0 = vertices_attributes_edges<M>(polygon, z0, scale0, revs0, ts0);
        avss1 = vertices_attributes_edges<M>(polygon, z1, scale1, revs1, ts1);
        create_band(m, avss0, avss1, surface_id0, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        scale1 = spec.axis_radius_interval.lerp(t);
        revs1 = spec.axis_azimuth_offset_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            avss1 = cusp_vertices_attributes_edges<M>(polygon, z1, ts1);
            connect_apex_cusp(m, avss1, surface_id0, closed);
        }
        else
        {
            avss1 = vertices_attributes_edges<M>(polygon, z1, scale1, revs1, ts1);
            connect_band(m, avss1, surface_id0, closed);
        }
    }

    // Calculate normals for the upper vertices
    size_t n = section_face_count;
    
    if (cusped_upper)
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_apex_cusp_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else if (nz == 1 && cusped_lower)
    {
        id_type id = j0_orig + face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties)
{
    assert(polygon_lower.vertex_count() == polygon_upper.vertex_count());
    assert(polygon_lower.sections() == polygon_upper.sections());
    assert(polygon_lower.sections() && polygon_lower.sections().value().empty());

    using T = M::value_type;
//std::cout << "create_cylinder edges polygon 2" << std::endl;

    CylinderSpecification<T> spec(polygon_lower.edge_count(), nz, T(0), T(0), T(0), T(0), properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    size_t section_count = face_count;

    assert(nz > 0);

    T t = T(1) / T(nz);

    auto ts0 = texture_span<T>(0, nz, false, false);
    auto ts1 = texture_span<T>(1, nz, false, false);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    id_type j0_orig = m.halfedge_store_count();

    auto avss0 = vertices_attributes_edges<M>(polygon_lower, std::nullopt, ts0);
    auto avss1 = vertices_attributes_edges<M>(lerp(polygon_lower, polygon_upper, t), std::nullopt, ts1);
    create_band(m, avss0, avss1, surface_id0, true);

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        ts1 = texture_span<T>(i, nz, false, false);

        j0_orig = m.halfedge_store_count();

        avss1 = vertices_attributes_edges<M>(lerp(polygon_lower, polygon_upper, t), std::nullopt, ts1);
        connect_band(m, avss1, surface_id0, true);
    }

    // Calculate normals for the upper vertices
    id_type id = j0_orig + 2 * face_count;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss1[i].size() - 1;
        calculate_upper_surface_vertex_normals_sequential(m, id, n);
        id += n;
    }

    seal_cylinder(m, submesh_id, spec, false, false);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
//std::cout << "create_cylinder_sections polygon z" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, M::val(1), M::val(1), z_lower, z_upper, M::val(0), M::val(0), Properties{{"azimuth_smooth", to_string(!polygon.sections())}} | properties);
    return create_cylinder_sections(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, data_value_type<M> scale_lower, data_value_type<M> scale_upper, data_value_type<M> revs_lower, data_value_type<M> revs_upper, const Properties& properties)
{
//std::cout << "create_cylinder_sections polygon z scale revs" << std::endl;
    CylinderSpecification<data_value_type<M>> spec(polygon.edge_count(), nz, scale_lower, scale_upper, z_lower, z_upper, revs_lower, revs_upper, Properties{{"azimuth_smooth", to_string(!polygon.sections())}} | properties);
    return create_cylinder_sections(mesh, name, polygon, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const CylinderSpecification<data_value_type<M>>& spec)
{
    using T = M::value_type;
//std::cout << "create_cylinder_sections polygon spec" << std::endl;

    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();

    assert(section_count == 1 && spec.azimuth_smooth || !spec.azimuth_smooth); 
    assert(section_count > 0);

    auto face_count = polygon.edge_count();
    assert(face_count > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = spec.axis_radius_interval.lower() == T(0);
    bool cusped_upper = spec.axis_radius_interval.upper() == T(0);
    assert(!cusped_lower || !cusped_upper);
    bool closed = true;

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T scale0 = spec.axis_radius_interval.lower();
    T scale1 = spec.axis_radius_interval.lerp(t);

    T revs0 = spec.axis_azimuth_offset_interval.lower();
    T revs1 = spec.axis_azimuth_offset_interval.lerp(t);

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    vertices_attributes_sections_type<M> avss0;
    vertices_attributes_sections_type<M> avss1;

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        avss0 = cusp_vertices_attributes_sections<M>(polygon, z0, ts0);
        avss1 = vertices_attributes_sections<M>(polygon, z1, scale1, revs1, ts1);
        create_base_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        avss0 = vertices_attributes_sections<M>(polygon, z0, scale0, revs0, ts0);
        avss1 = cusp_vertices_attributes_sections<M>(polygon, z1, ts1);
        create_apex_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else
    {
        avss0 = vertices_attributes_sections<M>(polygon, z0, scale0, revs0, ts0);
        avss1 = vertices_attributes_sections<M>(polygon, z1, scale1, revs1, ts1);
        create_band(m, avss0, avss1, surface_id0, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        scale1 = spec.axis_radius_interval.lerp(t);
        revs1 = spec.axis_azimuth_offset_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            avss1 = cusp_vertices_attributes_sections<M>(polygon, z1, ts1);
            connect_apex_cusp(m, avss1, surface_id0, closed);
        }
        else
        {
            avss1 = vertices_attributes_sections<M>(polygon, z1, scale1, revs1, ts1);
            connect_band(m, avss1, surface_id0, closed);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size();
            calculate_apex_cusp_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else if (nz == 1 && cusped_lower)
    {
        id_type id = j0_orig + face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size();
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size() - 1;
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon_lower, const geometry::Polygon<typename M::vector_traits>& polygon_upper, size_t nz, const Properties& properties)
{
    assert(polygon_lower.vertex_count() == polygon_upper.vertex_count());
    assert(polygon_lower.sections() == polygon_upper.sections());
    assert(polygon_lower.sections() && !polygon_lower.sections().value().empty());

    using T = M::value_type;
//std::cout << "create_cylinder sections polygon 2" << std::endl;

    CylinderSpecification<T> spec(polygon_lower.edge_count(), nz, T(0), T(0), T(0), T(0), properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    assert(polygon_lower.sections() && !polygon_lower.sections().value().empty());
    size_t section_count = polygon_lower.sections().value().size();
    if (section_count == 0)
    {
        section_count = face_count;
    }

    assert(nz > 0);

    T t = T(1) / T(nz);

    auto ts0 = texture_span<T>(0, nz, false, false);
    auto ts1 = texture_span<T>(1, nz, false, false);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    id_type j0_orig = m.halfedge_store_count();

    auto avss0 = vertices_attributes_sections<M>(polygon_lower, std::nullopt, ts0);
    auto avss1 = vertices_attributes_sections<M>(lerp(polygon_lower, polygon_upper, t), std::nullopt, ts1); // just do lerp here and get rid of vass ...
    create_band(m, avss0, avss1, surface_id0, true);

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        ts1 = texture_span<T>(i, nz, false, false);

        j0_orig = m.halfedge_store_count();

        avss1 = vertices_attributes_sections<M>(lerp(polygon_lower, polygon_upper, t), std::nullopt, ts1); // lerp does not 
        connect_band(m, avss1, surface_id0, true);
    }

    // Calculate normals for the upper vertices
    id_type id = j0_orig + 2 * face_count;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss1[i].size() - 1;
        calculate_upper_surface_vertex_normals_sequential(m, id, n);
        id += n;
    }

    seal_cylinder(m, submesh_id, spec, false, false);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties)
{
    using T = M::value_type;
//std::cout << "create_cylinder polygon ff" << std::endl;

    CylinderSpecification<T> spec(polygon.edge_count(), nz, T(0), T(0), z_lower, z_upper, properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T r0 = fr(T(0));
    T r1 = fr(t);

    T a0 = fa(T(0));
    T a1 = fa(t);

    auto matrix0 = geometry::scaling(r0) * geometry::rotation_z(math::PiTwo<T> * a0);
    auto matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

    bool cusped_lower = math::float_eq0(fr(T(0)));
    bool cusped_upper = math::float_eq0(fr(T(1)));
    assert(nz > 1 || !cusped_lower || !cusped_upper);

    bool closed = true;

    texture_span_type<M> ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    texture_span_type<M> ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = m.create_surface(submesh_id, SurfaceName::Body);

    vertices_attributes_type<M> avs0;
    vertices_attributes_type<M> avs1;

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        avs0 = cusp_vertices_attributes<M>(polygon, z0, ts0);
        avs1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
        create_base_cusp(m, avs0, avs1, surface_id0, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        avs0 = vertices_attributes<M>(transform(polygon, matrix0), z0, ts0);
        avs1 = cusp_vertices_attributes<M>(polygon, z1, ts1);
        create_apex_cusp(m, avs0, avs1, surface_id0, closed);
    }
    else
    {
        avs0 = vertices_attributes<M>(transform(polygon, matrix0), z0, ts0);
        avs1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
        create_band(m, avs0, avs1, surface_id0, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        r1 = fr(t);
        a1 = fa(t);
        matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            avs1 = cusp_vertices_attributes<M>(polygon, z1, ts1);
            connect_apex_cusp(m, avs1, surface_id0, closed);
        }
        else
        {
            avs1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
            connect_band(m, avs1, surface_id0, closed);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nz == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties)
{
    using T = M::value_type;
//std::cout << "create_cylinder_edges polygon ff" << std::endl;

    assert(polygon.sections() && polygon.sections().value().empty());

    CylinderSpecification<T> spec(polygon.edge_count(), nz, T(0), T(0), z_lower, z_upper, properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    nz = spec.axis_divisions;
    assert(nz > 0);

    size_t section_count = face_count;
    size_t section_face_count = 1;

    assert(!spec.azimuth_smooth);
    assert(spec.azimuth_interval.unit());

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T r0 = fr(T(0));
    T r1 = fr(t);

    T a0 = fa(T(0));
    T a1 = fa(t);

    auto matrix0 = geometry::scaling(r0) * geometry::rotation_z(math::PiTwo<T> * a0);
    auto matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

    bool cusped_lower = math::float_eq0(fr(T(0)));
    bool cusped_upper = math::float_eq0(fr(T(1)));
    assert(nz > 1 || !cusped_lower || !cusped_upper);

    bool closed = true;

    texture_span_type<M> ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    texture_span_type<M> ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    vertices_attributes_type<M> avss0;
    vertices_attributes_type<M> avss1;

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        avss0 = cusp_vertices_attributes<M>(polygon, z0, ts0);
        avss1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
        create_base_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        avss0 = vertices_attributes<M>(transform(polygon, matrix0), z0, ts0);
        avss1 = cusp_vertices_attributes<M>(polygon, z1, ts1);
        create_apex_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else
    {
        avss0 = vertices_attributes<M>(transform(polygon, matrix0), z0, ts0);
        avss1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
        create_band(m, avss0, avss1, surface_id0, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        r1 = fr(t);
        a1 = fa(t);
        matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            avss1 = cusp_vertices_attributes<M>(polygon, z1, ts1);
            connect_apex_cusp(m, avss1, surface_id0, closed);
        }
        else
        {
            avss1 = vertices_attributes<M>(transform(polygon, matrix1), z1, ts1);
            connect_band(m, avss1, surface_id0, closed);
        }
    }

    // Calculate normals for the upper vertices
    size_t n = section_face_count;
    
    if (cusped_upper)
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_apex_cusp_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else if (nz == 1 && cusped_lower)
    {
        id_type id = j0_orig + face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties)
{
    using T = M::value_type;
//std::cout << "create_cylinder_sections polygon ff" << std::endl;

    CylinderSpecification<T> spec(polygon.edge_count(), nz, T(0), T(0), z_lower, z_upper, properties);

    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T r0 = fr(T(0));
    T r1 = fr(t);

    T a0 = fa(T(0));
    T a1 = fa(t);

    auto matrix0 = geometry::scaling(r0) * geometry::rotation_z(math::PiTwo<T> * a0);
    auto matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

    bool cusped_lower = math::float_eq0(fr(T(0)));
    bool cusped_upper = math::float_eq0(fr(T(1)));
    assert(nz > 1 || !cusped_lower || !cusped_upper);

    bool closed = true;

    texture_span_type<M> ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    texture_span_type<M> ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    vertices_attributes_sections_type<M> avss0;
    vertices_attributes_sections_type<M> avss1;

    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        avss0 = cusp_vertices_attributes_sections<M>(polygon, z0, ts0);
        avss1 = vertices_attributes_sections<M>(transform(polygon, matrix1), z1, ts1);
        create_base_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else if (nz == 1 && cusped_upper)
    {
        avss0 = vertices_attributes_sections<M>(transform(polygon, matrix0), z0, ts0);
        avss1 = cusp_vertices_attributes_sections<M>(polygon, z1, ts1);
        create_apex_cusp(m, avss0, avss1, surface_id0, closed);
    }
    else
    {
        avss0 = vertices_attributes_sections<M>(transform(polygon, matrix0), z0, ts0);
        avss1 = vertices_attributes_sections<M>(transform(polygon, matrix1), z1, ts1);
        create_band(m, avss0, avss1, surface_id0, closed);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        z1 = spec.axis_z_interval.lerp(t);
        //ty1 = ty0 - t;
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        r1 = fr(t);
        a1 = fa(t);
        matrix1 = geometry::scaling(r1) * geometry::rotation_z(math::PiTwo<T> * a1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            avss1 = cusp_vertices_attributes_sections<M>(polygon, z1, ts1);
            connect_apex_cusp(m, avss1, surface_id0, closed);
        }
        else
        {
            avss1 = vertices_attributes_sections<M>(transform(polygon, matrix1), z1, ts1);
            connect_band(m, avss1, surface_id0, closed);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size();
            calculate_apex_cusp_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else if (nz == 1 && cusped_lower)
    {
        id_type id = j0_orig + face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size();
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }
    else
    {
        id_type id = j0_orig + 2 * face_count;
        for (size_t i = 0; i < section_count; ++i)
        {
            size_t n = avss1[i].size() - 1;
            calculate_upper_surface_vertex_normals_sequential(m, id, n);
            id += n;
        }
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, const geometry::PolygonWithHoles<typename M::vector_traits>& polygon_holes, size_t nz, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
    using T = M::value_type;
//std::cout << "create_cylinder polygon h" << std::endl;

    CylinderSpecification<T> spec(polygon_holes.polygon().edge_count(), nz, T(0), T(0), z_lower, z_upper, properties);

    nz = spec.axis_divisions;
    assert(nz > 0);

    z_lower = spec.z_interval.lower();
    z_upper = spec.z_interval.upper();

    M m{};
    id_type submesh_id = create_cylinder(m, name, polygon_holes.polygon(), nz, z_lower, z_upper, properties);

    id_type idHalfedge_lower = 0;
    assert(m.halfedge(idHalfedge_lower).border());
    id_type idHalfedge_upper = m.halfedge_store_count() - 2 * polygon_holes.polygon().edge_count();
    assert(m.halfedge(idHalfedge_upper).border());

    std::vector<id_type> idHalfedgeHoles_lower(polygon_holes.hole_count());
    std::vector<id_type> idHalfedgeHoles_upper(polygon_holes.hole_count());
    for (size_t i = 0; i < polygon_holes.hole_count(); ++i)
    {
        const auto& polygon = polygon_holes.hole(i);

        idHalfedgeHoles_lower[i] = m.halfedge_store_count();

        std::string hole_name = surface_name("hole", i);

        M mm{};
        create_cylinder(mm, hole_name, polygon, nz, z_lower, z_upper);
        // now transfer hole submesh name to hole surface name(s), directly hole_0 or concatenated hole_0_body ...
        m.insert(mm);

        assert(m.halfedge(idHalfedgeHoles_lower[i]).border());
        idHalfedgeHoles_upper[i] = m.halfedge_store_count() - 2 * polygon.edge_count();
        assert(m.halfedge(idHalfedgeHoles_upper[i]).border());
    }

    // make sure that the orientation of holes is correct due the the reversed winding order ...

// seal_cylinder here needs to use create_border_with_holes_face ...
//    seal_cylinder_with_holes(m, submesh_id, false, false, -normal, normal);

    if (spec.axis_termination_lower.style == TerminationStyle::Butt)
    {
        typename M::vector_type normal = {T(0), T(0), T(-1)};
        id_type surface_id_lower = m.create_surface(submesh_id, spec.axis_termination_lower.name, {normal});

        id_type face_id = create_border_with_holes_face(m, idHalfedge_lower, idHalfedgeHoles_lower, normal, surface_id_lower);
        calculate_face_texcoords(m, face_id);
    }

    if (spec.axis_termination_upper.style == TerminationStyle::Butt)
    {
        auto normal = normal_vector_type<M>::z_direction();
        id_type surface_id_upper = m.create_surface(submesh_id, spec.axis_termination_lower.name, {normal});

        id_type face_id = create_border_with_holes_face(m, idHalfedge_upper, idHalfedgeHoles_upper, normal, surface_id_upper);
        calculate_face_texcoords(m, face_id);
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, std::function<data_value_type<M>(data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>)> fz, std::function<data_value_type<M>(data_value_type<M>)> fa, const Properties& properties)
{
    using T = M::value_type;
//std::cout << "create_cylinder polygon fff" << std::endl;

    CylinderSpecification<T> spec(nAzimuth, nz, T(0), T(0), T(0), T(0), properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T r0 = fr(T(0));
    T r1 = fr(t);

    T z0 = fz(T(0));
    T z1 = fz(t);

    T revs0 = fa(T(0));
    T revs1 = fa(t);

    auto azimuth0 = shift(spec.azimuth_interval, revs0);
    auto azimuth1 = shift(spec.azimuth_interval, revs1);

    bool cusped_lower = math::float_eq0(r0);
    bool cusped_upper = math::float_eq0(fr(T(1)));
    assert(spec.azimuth_smooth);

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

     id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, face_count, r1, z0, z1, azimuth1, ts1, surface_id);
    }
    else if (nz == 1 && cusped_upper)
    {
        create_apex_cusp(m, face_count, r0, z0, z1, azimuth0, ts0, surface_id);
    }
    else
    {
        create_band(m, face_count, r0, r1, z0, z1, azimuth0, azimuth1, ts0, ts1, surface_id);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        r1 = fr(t);
        z1 = fz(t);
        revs1 = fa(t);
        azimuth1 = shift(spec.azimuth_interval, revs1);

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            connect_apex_cusp(m, face_count, z1, azimuth1, {r1, T(0), z1}, surface_id);
        }
        else
        {
            ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);
            connect_band(m, face_count, r1, z1, azimuth1, ts1, surface_id);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nz == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fr, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fz, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fa, const Properties& properties)
{
//std::cout << "create_cylinder polygon fff 2 params" << std::endl;
    // create_cylinder fr, fz, fa ...
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, const geometry::Polyline<typename M::vector_traits>& contour, const Properties& properties)
{
//std::cout << "create_cylinder polyline" << std::endl;
    using T = M::value_type;

    CylinderSpecification<T> spec(nAzimuth, contour.edge_count(), contour.vertices().front().y(), contour.vertices().back().y(), contour.vertices().front().x(), contour.vertices().back().x(), properties);

    nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = math::float_eq0(spec.axis_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.axis_radius_interval.upper());

    T r0 = contour.vertex(0).y();
    T r1 = contour.vertex(1).y();

    T z0 = contour.vertex(0).x();
    T z1 = contour.vertex(1).x();

    T dd = contour.length();
    T d1 = contour.length(0, 1);

    auto ts0 = texture_span<T>(T(0), nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(T(1), nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    auto face_count = spec.azimuth_divisions;
    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, nAzimuth, r1, z0, z1, spec.azimuth_interval, ts1, surface_id, true);
    }
    else if (nz == 1 && cusped_upper)
    {
        create_apex_cusp(m, nAzimuth, r0, z0, z1, spec.azimuth_interval, ts0, surface_id);
    }
    else
    {
        create_band(m, nAzimuth, r0, r1, z0, z1, spec.azimuth_interval, spec.azimuth_interval, ts0, ts1, surface_id, true);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        const auto& point = contour.vertex(i);
        z1 = point.x();
        r1 = point.y();
        d1 += contour.length(i - 1, i);
        if (i == nz)
        {
            d1 = dd;
        }

        // texcoord should use d0, d1 for v ...
        // same below ...

        j0_orig = m.halfedge_store_count();

        if (i == nz && cusped_upper)
        {
            connect_apex_cusp(m, nAzimuth, z1, {r1, z1}, spec.azimuth_interval, surface_id, true, true);
        }
        else
        {
            ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);
            connect_band(m, nAzimuth, r1, z1, spec.azimuth_interval, ts1, surface_id, true, true);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nz == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_cylinder(M& mesh, const std::string& name, size_t nAzimuth, std::span<const geometry::Point<math::CartesianTraits<data_value_type<M>, 2>>> contour, const Properties& properties)
{
//std::cout << "create_cylinder contour" << std::endl;
    using T = M::value_type;

    CylinderSpecification<T> spec(nAzimuth, contour.size() - 1, contour.front().y(), contour.back().y(), contour.front().x(), contour.back().x(), properties);

    auto face_count = spec.azimuth_divisions;
    assert(face_count > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    bool cusped_lower = math::float_eq0(spec.axis_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.axis_radius_interval.upper());
    bool closed = true;

    auto [z0, r0] = contour[0].rep();
    auto [z1, r1] = contour[1].rep();

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = nullid;

    bool bSmooth = spec.azimuth_smooth;
    assert(bSmooth); // otherwise not supported ...
    if (bSmooth)
    {
        surface_id0 = m.create_surface(submesh_id, SurfaceName::Body);
    }
    else
    {
        size_t section_count = face_count;
        surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });
    }

    auto face_count = spec.azimuth_divisions;
    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, nAzimuth, r1, z0, z1, spec.azimuth_interval, ts1, surface_id0, closed, true);
    }
    else if (nz == 1 && cusped_upper)
    {
        create_apex_cusp(m, nAzimuth, r0, z0, z1, spec.azimuth_interval, surface_id0, closed, true);
    }
    else
    {
        create_band(m, nAzimuth, r0, r1, z0, z1, spec.azimuth_interval, spec.azimuth_interval, ts0, ts1, surface_id0, closed, true);
    }

    for (size_t i = 2; i <= nz; ++i)
    {
        j0_orig = m.halfedge_store_count();

        auto [z1, r1] = contour[i].rep();

        if (i == nz && cusped_upper)
        {
            connect_apex_cusp(m, nAzimuth, z1, spec.azimuth_interval, surface_id0, closed, true);
        }
        else
        {
            ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);
            connect_band(m, nAzimuth, r1, z1, spec.azimuth_interval, ts1, surface_id0, closed, true);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nz == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
    return create_anticylinder(mesh, name, nAzimuth, nz, radius, radius, z_lower, z_upper, properties);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_anticylinder(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> radius_lower, data_value_type<M> radius_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
    CylinderSpecification<data_value_type<M>> spec(nAzimuth, nz, radius_lower, radius_upper, z_lower, z_upper, properties);

    if (spec.azimuth_smooth)
    {
        return create_anticylinder(mesh, name, spec);
    }

    return create_anticylinder_faces(mesh, name, spec);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_anticylinder(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec)
{
    assert(spec.azimuth_smooth);

    using T = M::value_type;

    size_t nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T dz = spec.axis_z_interval.length();
    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T dr = spec.axis_radius_interval.length();
    T r0 = spec.axis_radius_interval.lower();
    T r1 = spec.axis_radius_interval.lerp(t);

    // should handle possibility of cusp here with dr != 0 ...

    auto ts0 = texture_span<T>(0, nz, false, false);
    auto ts1 = texture_span<T>(1, nz, false, false);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    math::Interval<T> azimuth_interval(T(1));
    T dAzimuth = T(0.5) / T(nAzimuth);

    // smooth cylindrical/conical normal vector
    auto vertex_normal = [dr, dz](T phi) -> normal_vector_type<M>
    {
        auto cos_phi = math::denoise011(cos(phi));
        auto sin_phi = math::denoise011(sin(phi));
        return normalize(normal_vector_type<M>{dz * cos_phi, dz * sin_phi, -dr});
    };

    // Only reason not too just use the else clause for all is a little more calculation (for the radius) in the shift call ...
    if (dr == T(0))
    {
        auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth_interval, ts0, vertex_normal);
        auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, shift(azimuth_interval, dAzimuth), ts1, vertex_normal);

        create_antiband(m, avs0, avs1, surface_id);

        for (size_t i = 2; i <= nz; ++i)
        {
            t = T(i) / T(nz);
            z1 = spec.axis_z_interval.lerp(t);
            ts1 = texture_span<T>(i, nz, false, false);

            if (math::even(i))
            {
                shift(avs0, z1, ts1);
                connect_antiband(m, avs0, surface_id);
            }
            else
            {
                shift(avs1, z1, ts1);
                connect_antiband(m, avs1, surface_id);
            }
        }
    }
    else
    {
        auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth_interval, ts0, vertex_normal);
        auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, shift(azimuth_interval, dAzimuth), ts1, vertex_normal);

        create_antiband(m, avs0, avs1, surface_id);

        for (size_t i = 2; i <= nz; ++i)
        {
            t = T(i) / T(nz);
            r1 = spec.axis_radius_interval.lerp(t);
            z1 = spec.axis_z_interval.lerp(t);
            ts1 = texture_span<T>(i, nz, false, false);

            if (math::even(i))
            {
                shift(avs0, r1, z1, ts1);
                connect_antiband(m, avs0, surface_id);
            }
            else
            {
                shift(avs1, r1, z1, ts1);
                connect_antiband(m, avs1, surface_id);
            }
        }
    }

    seal_cylinder(m, submesh_id, spec, false, false);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_anticylinder_faces(M& mesh, const std::string& name, const CylinderSpecification<data_value_type<M>>& spec)
{
    assert(!spec.azimuth_smooth);

    using T = M::value_type;

    size_t nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    size_t nz = spec.axis_divisions;
    assert(nz > 0);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T dr = spec.axis_radius_interval.length();
    T r0 = spec.axis_radius_interval.lower();
    T r1 = spec.axis_radius_interval.lerp(t);

    // should handle possibility of cusp here with dr != 0 ...

    M m{};
    id_type submesh_id = m.create_submesh(name);

    math::Interval<T> azimuth_interval(T(1));
    T dAzimuth = T(0.5) / T(nAzimuth);

    // should be able to just omit ts here too ...
    auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth_interval, {});
    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, shift(azimuth_interval, dAzimuth), {});

    if (dr == T(0))
    {
        create_antiband_faces(m, avs0, avs1, submesh_id);

        for (size_t i = 2; i <= nz; ++i)
        {
            t = T(i) / T(nz);
            z1 = spec.axis_z_interval.lerp(t);

            if (math::even(i))
            {
                shift(avs0, z1);
                connect_antiband_faces(m, avs0, submesh_id);
            }
            else
            {
                shift(avs1, z1);
                connect_antiband_faces(m, avs1, submesh_id);
            }
        }
    }
    else
    {
        create_antiband_faces(m, avs0, avs1, submesh_id);

        for (size_t i = 2; i <= nz; ++i)
        {
            t = T(i) / T(nz);
            r1 = spec.axis_radius_interval.lerp(t);
            z1 = spec.axis_z_interval.lerp(t);

            if (math::even(i))
            {
                shift(avs0, r1, z1);
                connect_antiband_faces(m, avs0, submesh_id);
            }
            else
            {
                shift(avs1, r1, z1);
                connect_antiband_faces(m, avs1, submesh_id);
            }
        }
    }

    seal_cylinder(m, submesh_id, spec, false, false);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, data_value_type<M> radius, const Properties& properties)
{
    using T = M::value_type;

    SphereSpecification<T> spec(nAzimuth, nElevation, radius, Properties{{"azimuth_smooth", "true"}} | properties);

    nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    nElevation = spec.elevation_divisions;
    assert(nElevation > 0);

    radius = spec.radius;
    assert(radius > T(0));

    bool cusped_lower = math::float_eq0(spec.elevation_interval.lower());
    bool cusped_upper = math::float_eq(spec.elevation_interval.upper(), T(1));

    T t = T(1) / T(nElevation);
    T t0 = spec.elevation_interval.lower();
    T t1 = spec.elevation_interval.lerp(t);

    T theta0 = math::Pi<T> * (t0 - T(0.5));
    T theta1 = math::Pi<T> * (t1 - T(0.5));
    assert(theta0 >= -math::PiHalf<T>);

    T z0 = radius * math::denoise011(sin(theta0));
    T z1 = radius * math::denoise011(sin(theta1));

    T r0 = cusped_lower ? T(0) : radius * math::denoise011(cos(theta0));
    T r1 = radius * math::denoise011(cos(theta1));

    auto ts0 = texture_span<T>(0, nElevation, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nElevation, cusped_lower, cusped_upper);

    auto vertex_normal0 = [r0, z0](T phi) -> normal_vector_type<M>
    {
        auto cos_phi = math::denoise011(cos(phi));
        auto sin_phi = math::denoise011(sin(phi));
        normal_vector_type<M> direction = {r0 * cos_phi, r0 * sin_phi, z0};
        return normalize(direction);
    };

    auto vertex_normal1 = [&r1, &z1](T phi) -> normal_vector_type<M>
    {
        auto cos_phi = math::denoise011(cos(phi));
        auto sin_phi = math::denoise011(sin(phi));
        normal_vector_type<M> direction = {r1 * cos_phi, r1 * sin_phi, z1};
        return normalize(direction);
    };

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    if (cusped_lower)
    {
        create_base_cusp(m, nAzimuth, r1, -radius, z1, spec.azimuth_interval, ts1, surface_id, vertex_normal0, vertex_normal1);
    }
    else if (nElevation == 1 && cusped_upper)
    {
        create_apex_cusp(m, nAzimuth, r0, z0, z1, spec.azimuth_interval, ts0, surface_id, vertex_normal0, vertex_normal1);
    }
    else
    {
        create_band(m, nAzimuth, r0, r1, z0, z1, spec.azimuth_interval, spec.azimuth_interval, ts0, ts1, surface_id, vertex_normal0, vertex_normal1);
    }

    for (size_t i = 2; i <= nElevation; ++i)
    {
        t = T(i) / T(nElevation);
        t1 = spec.elevation_interval.lerp(t);
        theta1 = math::Pi<T> * (t1 - T(0.5));
        r1 = i == nElevation && cusped_upper ? T(0) : radius * math::denoise011(cos(theta1));
        z1 = radius * math::denoise0(sin(theta1));
        ts1 = texture_span<T>(i, nElevation, cusped_lower, cusped_upper);

        if (i == nElevation && cusped_upper)
        {
            connect_apex_cusp(m, nAzimuth, radius, spec.azimuth_interval, surface_id, vertex_normal1);
        }
        else
        {
            connect_band(m, nAzimuth, r1, z1, spec.azimuth_interval, ts1, surface_id, vertex_normal1);
        }
    }

    seal_sphere(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_sphere(M& mesh, const std::string& name, size_t nAzimuth, const geometry::Polyline<typename M::vector_traits>& contour, const Properties& properties)
{
    using T = M::value_type;

    SphereSpecification<T> spec(nAzimuth, contour.edge_count(), T(0), properties);

    size_t nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    size_t nElevation = contour.edge_count();
    assert(nElevation > 0);

    // check that contour at ends is 0 too ...
    bool cusped_lower = math::float_eq0(spec.elevation_interval.lower());
    bool cusped_upper = math::float_eq(spec.elevation_interval.upper(), T(1));
    assert(spec.azimuth_smooth); // otherwise not supported ...

    T t0 = contour.vertex(0).x();
    T t1 = contour.vertex(1).x();

    T theta0 = math::Pi<T> * (t0 - T(0.5));
    T theta1 = math::Pi<T> * (t1 - T(0.5));

    T radius0 = contour.vertex(0).y();
    T radius1 = contour.vertex(1).y();

    T z0 = radius0 * math::denoise011(sin(theta0));
    T z1 = radius1 * math::denoise011(sin(theta1));

    T r0 = radius0 * math::denoise011(cos(theta0));
    T r1 = radius1 * math::denoise011(cos(theta1));

    T dd = contour.length();
    T d1 = contour.length(0, 1);

    auto ts0 = texture_span<T>(T(0), cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(d1 / dd, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    auto face_count = spec.azimuth_divisions;
    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, nAzimuth, r1, z0, z1, spec.azimuth_interval, ts1, surface_id);
    }
    else if (nElevation == 1 && cusped_upper)
    {
        create_apex_cusp(m, nAzimuth, r0, z0, z1, spec.azimuth_interval, surface_id);
    }
    else
    {
        create_band(m, nAzimuth, r0, r1, z0, z1, spec.azimuth_interval, spec.azimuth_interval, ts0, ts1, surface_id);
    }

    for (size_t i = 2; i <= nElevation; ++i)
    {
        const auto& point = contour.vertex(i);
        T t1 = point.x();
        T theta1 = math::Pi<T> * (t1 - T(0.5));
        T radius1 = point.y();
        r1 = radius1 * math::denoise011(cos(theta1));
        z1 = radius1 * math::denoise011(sin(theta1));
        d1 += contour.length(i - 1, i);
        if (i == nElevation)
        {
            d1 = dd;
        }
        ts1 = texture_span<T>(d1 / dd, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        if (i == nElevation && cusped_upper)
        {
            T radius0 = contour.vertex(i - 1).y();
            assert(float_eq0(radius0));
            connect_apex_cusp(m, nAzimuth, radius0, spec.azimuth_interval, surface_id);
        }
        else
        {
            connect_band(m, nAzimuth, r1, z1, spec.azimuth_interval, ts1, surface_id);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nElevation == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_sphere(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, std::function<data_value_type<M>(data_value_type<M>)> fr, const Properties& properties)
{
    using T = M::value_type;

    SphereSpecification<T> spec(nAzimuth, nElevation, T(0), properties);

    size_t nAzimuth = spec.azimuth_divisions;
    assert(nAzimuth > 2);

    nElevation = spec.elevation_divisions;
    assert(nElevation > 0);

    // check that fr at ends is 0 too ...
    bool cusped_lower = math::float_eq0(spec.elevation_interval.lower());
    bool cusped_upper = math::float_eq(spec.elevation_interval.upper(), T(1));
    assert(spec.azimuth_smooth); // otherwise not supported ...

    T t = T(1) / T(nElevation);
    T t0 = spec.elevation_interval.lower();
    T t1 = spec.elevation_interval.lerp(t);

    T theta0 = math::Pi<T> * (t0 - T(0.5));
    T theta1 = math::Pi<T> * (t1 - T(0.5));

    T radius0 = fr(t0);
    T radius1 = fr(t1);

    T z0 = radius0 * math::denoise011(sin(theta0));
    T z1 = radius1 * math::denoise011(sin(theta1));

    T r0 = radius0 * math::denoise011(cos(theta0));
    T r1 = radius1 * math::denoise011(cos(theta1));

    auto ts0 = texture_span<T>(0, nElevation, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nElevation, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    auto face_count = spec.azimuth_divisions;
    id_type j0_orig = m.halfedge_store_count();

    if (cusped_lower)
    {
        create_base_cusp(m, nAzimuth, r1, z0, z1, spec.azimuth_interval, ts1, surface_id);
    }
    else if (nElevation == 1 && cusped_upper)
    {
        create_apex_cusp(m, nAzimuth, r0, z0, z1, spec.azimuth_interval, surface_id);
    }
    else
    {
        create_band(m, nAzimuth, r0, r1, z0, z1, spec.azimuth_interval, spec.azimuth_interval, ts0, ts1, surface_id);
    }

    for (size_t i = 2; i <= nElevation; ++i)
    {
        t = T(i) / T(nElevation);
        t1 = spec.elevation_interval.lerp(t);
        theta1 = math::Pi<T> * (t1 - T(0.5));
        radius1 = fr(t1);
        r1 = radius1 * math::denoise011(cos(theta1));
        z1 = radius1 * math::denoise011(sin(theta1));
        ts1 = texture_span<T>(i, nElevation, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        if (i == nElevation && cusped_upper)
        {
            assert(float_eq0(radius1));
            connect_apex_cusp(m, nAzimuth, radius1, spec.azimuth_interval, surface_id);
        }
        else
        {
            connect_band(m, nAzimuth, r1, z1, spec.azimuth_interval, ts1, surface_id);
        }
    }

    // Calculate normals for the upper vertices
    if (cusped_upper)
    {
        calculate_apex_cusp_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }
    else if (nElevation == 1 && cusped_lower)
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + face_count, face_count);
    }
    else
    {
        calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);
    }

    seal_sphere(m, submesh_id, spec, cusped_lower, cusped_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_sphere(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fr, const Properties& properties)
{
    // create_sphere ...
    assert(false);
    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, data_value_type<M> rMajor, data_value_type<M> rMinor, const Properties& properties)
{
    using T = M::value_type;

    TorusSpecification<T> spec(nMajor, nMinor, rMajor, rMajor, rMinor, rMinor, properties);

    nMajor = spec.major_divisions;
    assert(nMajor > 2);

    nMinor = spec.minor_divisions;
    assert(nMinor > 2);

    assert(spec.major_radius_interval.length() == T(0));
    rMajor = spec.major_radius_interval.lower();
    assert(rMajor > T(0));

    assert(spec.minor_radius_interval.length() == T(0));
    rMinor = spec.minor_radius_interval.lower();
    assert(rMinor > T(0));

    // theta/major and phi/minor follow cylinder band orientation
    // phi measured from innermost points and with rotation reversed

    bool closed_major = spec.major_azimuth_interval.unit();
    bool closed_minor = spec.minor_azimuth_interval.unit();

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    vertices_attributes_type<M> avs0(nMinor + 1);
    vertices_attributes_type<M> avs1(nMinor + 1);

    for (size_t i = 0; i <= nMajor; ++i)
    {
        T t = T(i) / T(nMajor);
//        T ty = T(1) - t;
        T ty = T(nMajor - i) / T(nMajor);
        T theta = math::PiTwo<T> * spec.major_azimuth_interval.lerp(t);

        if (i == nMajor && closed_major)
        {
            avs1 = avs0;
            for (size_t j = 0; j <= nMinor; ++j)
            {
                avs1[j].texcoord().set_y(T(0));
            }
        }
        else
        {
            T sxMajor = math::denoise011(cos(theta));
            T syMajor = math::denoise011(sin(theta));

            typename M::point_type center = {rMajor * sxMajor, rMajor * syMajor, T(0)};

            for (size_t j = 0; j <= nMinor; ++j)
            {
                T s = T(j) / T(nMinor);
T phi =  math::PiTwo<T> * (math::lerp(spec.minor_azimuth_interval.upper(), spec.minor_azimuth_interval.lower(), s) - T(0.5));
//                T phi =  math::PiTwo<T> * (spec.minor_azimuth_interval.lerp(T(1) - s) - T(0.5)); // 1 - s? ...

                if (j == nMinor && closed_minor)
                {
                    avs1[j] = avs1[0];
                    avs1[j].texcoord().set_x(T(1));
                }
                else
                {
                    T sxMinor = math::denoise011(cos(phi));
                    T syMinor = math::denoise011(sin(phi));

                    avs1[j].set_position({(rMajor + rMinor * sxMinor) * sxMajor, (rMajor + rMinor * sxMinor) * syMajor, rMinor * syMinor});
                    avs1[j].set_normal(normalize(avs1[j].position() - center));
                    avs1[j].set_texcoord({s, ty});
                }
            }
        }

        if (i == 0)
        {
            avs0 = avs1;
        }
        else if (i == 1)
        {
            create_band(m, avs0, avs1, surface_id, closed_minor, false);
        }
        else
        {
            connect_band(m, avs1, surface_id, closed_minor, false);
        }
    }

    seal_torus(m, submesh_id, spec, false, false, rMajor);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, data_value_type<M> rMajor_lower, data_value_type<M> rMajor_upper, data_value_type<M> rMinor_lower, data_value_type<M> rMinor_upper, data_value_type<M> z_lower, data_value_type<M> z_upper, const Properties& properties)
{
    using T = M::value_type;

    TorusSpecification<T> spec(nMajor, nMinor, rMajor_lower, rMajor_upper, rMinor_lower, rMinor_upper, properties); // z ...

    nMajor = spec.azimuth_divisions;
    assert(nMajor > 2);

    nMinor = spec.elevation_divisions;
    assert(nMinor > 2);

    // theta/major and phi/minor follow cylinder band orientation
    // phi measured from innermost points and with rotation reversed (? ...)

    bool bOpenMajor = !spec.major_azimuth_interval.unit() || spec.major_radius_interval.length != T(0) || spec.major_z_interval.length != T(0);
    bool closed_minor = spec.minor_azimuth_interval.unit();
    bool cusped_lower = math::float_eq0(spec.minor_radius_interval.lower());
    bool cusped_upper = math::float_eq0(spec.minor_radius_interval.upper());

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    vertices_attributes_type<M> avs0(nMinor + 1);
    vertices_attributes_type<M> avs1(nMinor + 1);

    for (size_t i = 0; i <= nMajor; ++i)
    {
        T t = T(i) / T(nMajor);
//        T ty = T(1) - t;
        T ty = T(nMajor - i) / T(nMajor);
        T theta = math::PiTwo<T> * spec.major_azimuth_interval.lerp(t);
        T rMajor = spec.major_radius_interval.lerp(t);
        T rMinor = spec.minor_radius_interval.lerp(t);
        T zCenter = spec.major_z_interval.lerp(t);

        if (i == nMajor && !bOpenMajor)
        {
            avs1 = avs0;
            for (size_t j = 0; j <= nMinor; ++j)
            {
                avs1[j].texcoord().set_y(T(0));
            }
        }
        else
        {
            T sxMajor = math::denoise011(cos(theta));
            T syMajor = math::denoise011(sin(theta));

            typename M::point_type center = {rMajor * sxMajor, rMajor * syMajor, zCenter};

            for (size_t j = 0; j <= nMinor; ++j)
            {
                T s = T(j) / T(nMinor);
T phi =  math::PiTwo<T> * (math::lerp(spec.minor_azimuth_interval.upper(), spec.minor_azimuth_interval.lower(), s) - T(0.5));
//                T phi =  math::PiTwo<T> * (spec.minor_azimuth_interval.lerp(T(1) - s) - T(0.5));

                if (j == nMinor && closed_minor)
                {
                    avs1[j] = avs1[0];
                    avs1[j].texcoord().set_x(T(1));
                }
                else
                {
                    T sxMinor = math::denoise011(cos(phi));
                    T syMinor = math::denoise011(sin(phi));

                    avs1[j].set_position({center.x() + rMinor * sxMinor * sxMajor, center.y() + rMinor * sxMinor * syMajor, center.z() + rMinor * syMinor});
                    avs1[j].set_normal(normalize(avs1[j].position() - center));
                    avs1[j].set_texcoord({s, ty});
                }
            }
        }

        if (i == 0)
        {
            avs0 = avs1;
        }
        else if (i == 1)
        {
            create_band(m, avs0, avs1, closed_minor, surface_id);
        }
        else
        {
            connect_band(m, avs1, closed_minor, surface_id);
        }
    }

    // close avs since we don't have enough info here to close off side center ...
    // not a big deal except for normal calculations of internal faces ...
//    seal_torus(m, nMinor, nMajor, (rMajor_lower + rMajor_upper) / T(2), cusped_lower, cusped_upper, bOpenMinor, bOpenMajor, extentMinor, extentMajor, submesh_id); // ...
    seal_torus(m, submesh_id, spec, cusped_lower, cusped_upper, (rMajor_lower + rMajor_upper) / T(2)); // average not correct ...

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_torus(M& mesh, const std::string& name, size_t nMajor, size_t nMinor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> frMajor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> frMinor, std::function<data_value_type<M>(data_value_type<M>, data_value_type<M>)> fz, const Properties& properties)
{
    using T = M::value_type;

    TorusSpecification<T> spec(nMajor, nMinor, frMajor(T(0)), frMajor(T(1)), frMinor(T(0)), frMinor(T(1)), properties); // z ...

    nMajor = spec.azimuth_divisions;
    assert(nMajor > 2);

    nMinor = spec.elevation_divisions;
    assert(nMinor > 2);

    // theta/major and phi/minor follow cylinder band orientation
    // phi measured from innermost points and with rotation reversed (? ...)

//    bool bOpenMajor = !spec.major_azimuth_interval.unit() || spec.major_radius_interval.length() != T(0) || spec.major_z_interval.length() != T(0);
    bool closed_major = !spec.major_azimuth_interval.unit(); // ...
    bool closed_minor = spec.minor_azimuth_interval.unit();
    bool cusped_lower = math::float_eq0(frMinor(T(0)));
    bool cusped_upper = math::float_eq0(frMinor(T(1)));

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

    vertices_attributes_type<M> avs0(nMinor + 1);
    vertices_attributes_type<M> avs1(nMinor + 1);

    for (size_t i = 0; i <= nMajor; ++i)
    {
        T t = T(i) / T(nMajor);
//        T ty = T(1) - t;
        T ty = T(nMajor - i) / T(nMajor);
        T theta = math::PiTwo<T> * spec.major_azimuth_interval.lerp(t);

        if (i == nMajor && closed_major)
        {
            avs1 = avs0;
            for (size_t j = 0; j <= nMinor; ++j)
            {
                avs1[j].texcoord().set_y(T(0));
            }
        }
        else
        {
            T sxMajor = math::denoise011(cos(theta));
            T syMajor = math::denoise011(sin(theta));

            for (size_t j = 0; j <= nMinor; ++j)
            {
                T s = T(j) / T(nMinor);
T phi =  math::PiTwo<T> * (math::lerp(spec.minor_azimuth_interval.upper(), spec.minor_azimuth_interval.lower(), s) - T(0.5));
//                T phi =  math::PiTwo<T> * (spec.minor_azimuth_interval.lerp(s) - T(0.5));

                T rMajor = frMajor(t, s);
                T rMinor = frMinor(t, s);
                T zCenter = fz(t, s);

                if (j == nMinor && closed_minor)
                {
                    avs1[j] = avs1[0];
                    avs1[j].texcoord().set_x(T(1));
                }
                else
                {
                    T sxMinor = math::denoise011(cos(phi));
                    T syMinor = math::denoise011(sin(phi));

                    typename M::point_type center = {rMajor * sxMajor, rMajor * syMajor, zCenter};

                    avs1[j].set_position({center.x() + rMinor * sxMinor * sxMajor, center.y() + rMinor * sxMinor * syMajor, center.z() + rMinor * syMinor});
                    avs1[j].set_normal(normalize(avs1[j].position() - center));
                    avs1[j].set_texcoord({s, ty});
                }
            }
        }

        if (i == 0)
        {
            avs0 = avs1;
        }
        else if (i == 1)
        {
            create_band(m, avs0, avs1, closed_minor, surface_id);
        }
        else
        {
            connect_band(m, avs1, closed_minor, surface_id);
        }
    }

    // close avs since we don't have enough info here to close off side center ...
    // not a big deal except for normal calculations of internal faces ...
    seal_torus(m, submesh_id, cusped_lower, cusped_upper, fr(T(0.5), T(0.5))); // ...

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_tetrahedron(M& mesh, const std::string& name, data_value_type<M> radius)
{
    using T = M::value_type;

    assert(radius > T(0));

    // a = 4 / sqrt(6); edge length of unit tetrahedron
    T x1 = radius * sqrt(T(2)) / T(3); // radius * a * sqrt(3) / 6;
    T y1 = radius * sqrt(T(6)) / T(3); // radius * a / 2;
    T z1 = radius / T(3);

    typename M::point_type position0 = {T(2) * x1, T(0), -z1};
    typename M::point_type position1 = {-x1, y1, -z1};
    typename M::point_type position2 = {-x1, -y1, -z1};
    typename M::point_type position3 = {T(0), T(0), radius};

    typename M::vector_type normal0 = normalize(cross(position2 - position0, position1 - position0));
    typename M::vector_type normal1 = normalize(cross(position3 - position1, position0 - position1));
    typename M::vector_type normal2 = normalize(cross(position3 - position2, position1 - position2));
    typename M::vector_type normal3 = normalize(cross(position2 - position3, position0 - position3));

    T yt = T(1) - sqrt(T(3)) / T(2);

    vertices_attributes_type<M> avs =
    {
        {position2, normal0, {T(0), T(1)}},
        {position1, normal0, {T(1), T(1)}},
        {position0, normal0, {T(0.5), yt}},

        {position0, normal1, {T(0), T(1)}},
        {position1, normal1, {T(1), T(1)}},
        {position3, normal1, {T(0.5), yt}},

        {position1, normal2, {T(0), T(1)}},
        {position2, normal2, {T(1), T(1)}},
        {position3, normal2, {T(0.5), yt}},

        {position2, normal3, {T(0), T(1)}},
        {position0, normal3, {T(1), T(1)}},
        {position3, normal3, {T(0.5), yt}},
    };

    M m{};
    id_type submesh_id = m.create_submesh(name);

    m.create_halfedge(6, 1, 2, 0, 0);
    m.create_halfedge(3, 2, 0, 1, 0);
    m.create_halfedge(9, 0, 1, 2, 0);

    m.create_halfedge(1, 4, 5, 3, 1);
    m.create_halfedge(8, 5, 3, 4, 1);
    m.create_halfedge(10, 3, 4, 5, 1);

    m.create_halfedge(0, 7, 8, 6, 2);
    m.create_halfedge(11, 8, 6, 7, 2);
    m.create_halfedge(4, 6, 7, 8, 2);

    m.create_halfedge(2, 10, 11, 9, 3);
    m.create_halfedge(5, 11, 9, 10, 3);
    m.create_halfedge(7, 9, 10, 11, 3);

    for (size_t i = 0; i < m.halfedge_count(); ++i) // One vertex per halfedge
    {
        m.create_vertex(i, avs[i]);
    }

    for (size_t i = 0; i < 4; ++i)
    {
        typename M::vector_type normal = avs[3 * i].normal();
        id_type surface_id = m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, i), {normal});
        m.create_face(surface_id, 3 * i, {normal});
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_octahedron(M& mesh, const std::string& name, data_value_type<M> radius, bool bVertex)
{
    using T = M::value_type;

    assert(radius > T(0));

    vertices_attributes_type<M> avs;

    if (bVertex)
    {
        T n0 = T(1) / sqrt(T(3));

        avs =
        {
            {{T(0), T(0), -radius}, {n0, n0, -n0}, {T(0.5), T(0)}},
            {{T(0), radius, T(0)}, {n0, n0, -n0}, {T(0), T(1)}},
            {{radius, T(0), T(0)}, {n0, n0, -n0}, {T(1), T(1)}},

            {{T(0), T(0), -radius}, {-n0, n0, -n0}, {T(0.5), T(0)}},
            {{-radius, T(0), T(0)}, {-n0, n0, -n0}, {T(0), T(1)}},
            {{T(0), radius, T(0)}, {-n0, n0, -n0}, {T(1), T(1)}},

            {{T(0), T(0), -radius}, {-n0, -n0, -n0}, {T(0.5), T(0)}},
            {{T(0), -radius, T(0)}, {-n0, -n0, -n0}, {T(0), T(1)}},
            {{-radius, T(0), T(0)}, {-n0, -n0, -n0}, {T(1), T(1)}},

            {{T(0), T(0), -radius}, {n0, -n0, -n0}, {T(0.5), T(0)}},
            {{radius, T(0), T(0)}, {n0, -n0, -n0}, {T(0), T(1)}},
            {{T(0), -radius, T(0)}, {n0, -n0, -n0}, {T(1), T(1)}},

            {{T(0), T(0), radius}, {n0, n0, n0}, {T(0.5), T(0)}},
            {{radius, T(0), T(0)}, {n0, n0, n0}, {T(0), T(1)}},
            {{T(0), radius, T(0)}, {n0, n0, n0}, {T(1), T(1)}},

            {{T(0), T(0), radius}, {-n0, n0, n0}, {T(0.5), T(0)}},
            {{T(0), radius, T(0)}, {-n0, n0, n0}, {T(0), T(1)}},
            {{-radius, T(0), T(0)}, {-n0, n0, n0}, {T(1), T(1)}},

            {{T(0), T(0), radius}, {-n0, -n0, n0}, {T(0.5), T(0)}},
            {{-radius, T(0), T(0)}, {-n0, -n0, n0}, {T(0), T(1)}},
            {{T(0), -radius, T(0)}, {-n0, -n0, n0}, {T(1), T(1)}},

            {{T(0), T(0), radius}, {n0, -n0, n0}, {T(0.5), T(0)}},
            {{T(0), -radius, T(0)}, {n0, -n0, n0}, {T(0), T(1)}},
            {{radius, T(0), T(0)}, {n0, -n0, n0}, {T(1), T(1)}},
        };
    }
    else
    {
        // a = sqrt(2); edge length of unit octahedron
        T x1 = radius / sqrt(T(6)); // radius * a * sqrt(3) / 6;
        T x2 = radius * sqrt(T(6)) / T(3); // radius * a * sqrt(3) / 3;
        T y1 = radius / sqrt(T(2)); // radius * a / 2;
        T z1 = radius / sqrt(T(3)); // radius * a / sqrt(6);

        T xn1 = sqrt(T(2)) / T(3);
        T xn2 = xn1 * T(2);
        T yn = sqrt(T(2) / T(3));
        T zn = T(1) / T(3);

        auto normal_z = normal_vector_type<M>::z_direction();

        avs =
        {
            // Bottom face
            {{x2, T(0), -z1}, -normal_z, {T(0.5), T(0)}},
            {{-x1, -y1, -z1}, -normal_z, {T(0), T(1)}},
            {{-x1, y1, -z1}, -normal_z, {T(1), T(1)}},

            {{x2, T(0), -z1}, {xn1, -yn, -zn}, {T(1), T(1)}},
            {{x1, -y1, z1}, {xn1, -yn, -zn}, {T(0.5), T(0)}},
            {{-x1, -y1, -z1}, {xn1, -yn, -zn}, {T(0), T(1)}},

            {{x2, T(0), -z1}, {xn2, T(0), zn}, {T(0.5), T(0)}},
            {{x1, y1, z1}, {xn2, T(0), zn}, {T(0), T(1)}},
            {{x1, -y1, z1}, {xn2, T(0), zn}, {T(1), T(1)}},

            {{x2, T(0), -z1}, {xn1, yn, -zn}, {T(0), T(1)}},
            {{-x1, y1, -z1}, {xn1, yn, -zn}, {T(1), T(1)}},
            {{x1, y1, z1}, {xn1, yn, -zn}, {T(0.5), T(0)}},

            {{-x2, T(0), z1}, {-xn2, T(0), -zn}, {T(0.5), T(0)}},
            {{-x1, y1, -z1}, {-xn2, T(0), -zn}, {T(0), T(1)}},
            {{-x1, -y1, -z1}, {-xn2, T(0), -zn}, {T(1), T(1)}},

            {{-x2, T(0), z1}, {-xn1, -yn, zn}, {T(1), T(1)}},
            {{-x1, -y1, -z1}, {-xn1, -yn, zn}, {T(0.5), T(0)}},
            {{x1, -y1, z1}, {-xn1, -yn, zn}, {T(0), T(1)}},

            // Top face                            
            {{-x2, T(0), z1}, normal_z, {T(0.5), T(0)}}, 
            {{x1, -y1, z1}, normal_z, {T(0), T(1)}}, 
            {{x1, y1, z1}, normal_z, {T(1), T(1)}},
                                                   
            {{-x2, T(0), z1}, {-xn1, yn, zn}, {T(0), T(1)}}, 
            {{x1, y1, z1}, {-xn1, yn, zn}, {T(1), T(1)}}, 
            {{-x1, y1, -z1}, {-xn1, yn, zn}, {T(0.5), T(0)}},
        };
    }

    M m{};
    id_type submesh_id = m.create_submesh(name);

    m.create_halfedge(5, 1, 2, 0, 0);
    m.create_halfedge(13, 2, 0, 1, 0);
    m.create_halfedge(9, 0, 1, 2, 0);

    m.create_halfedge(8, 4, 5, 3, 1);
    m.create_halfedge(16, 5, 3, 4, 1);
    m.create_halfedge(0, 3, 4, 5, 1);

    m.create_halfedge(11, 7, 8, 6, 2);
    m.create_halfedge(19, 8, 6, 7, 2);
    m.create_halfedge(3, 6, 7, 8, 2);

    m.create_halfedge(2, 10, 11, 9, 3);
    m.create_halfedge(22, 11, 9, 10, 3);
    m.create_halfedge(6, 9, 10, 11, 3);

    m.create_halfedge(23, 13, 14, 12, 4);
    m.create_halfedge(1, 14, 12, 13, 4);
    m.create_halfedge(15, 12, 13, 14, 4);

    m.create_halfedge(14, 16, 17, 15, 5);
    m.create_halfedge(4, 17, 15, 16, 5);
    m.create_halfedge(18, 15, 16, 17, 5);

    m.create_halfedge(17, 19, 20, 18, 6);
    m.create_halfedge(7, 20, 18, 19, 6);
    m.create_halfedge(21, 18, 19, 20, 6);

    m.create_halfedge(20, 22, 23, 21, 7);
    m.create_halfedge(10, 23, 21, 22, 7);
    m.create_halfedge(12, 21, 22, 23, 7);

    for (size_t i = 0; i < m.halfedge_count(); ++i) // One vertex per halfedge
    {
        m.create_vertex(i, avs[i]);
    }

    for (size_t i = 0; i < 8; ++i)
    {
        typename M::vector_type normal = avs[3 * i].normal();
        id_type surface_id = m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, i), {normal});
        m.create_face(surface_id, 3 * i, {normal});
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_dodecahedron(M& mesh, const std::string& name, data_value_type<M> radius)
{
    using T = M::value_type;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    assert(radius > T(0));

    // Pentagonal face parameters
    T gr = math::GoldenRatio<T>;
    T a = radius * T(2) / (sqrt(T(3)) * gr);
    T d = radius * T(2) / sqrt(T(3));
    T ri = a / (T(2) * sqrt(T(5) - sqrt(T(20)))); // inscribed radius, a * .68819
    T rc = a * sqrt(T(50) + T(10) * sqrt(T(5))) / T(10); // circumscribed radius, a * .85065

    // Dodecahedron face parameters
    T x1 = radius * T(0.6070620); // rc
    T x2 = radius * T(0.1875925);
    T x3 = radius * T(0.4911235); // ri
    T x4 = radius * T(0.9822469); // 2 * ri
    T x5 = radius * T(0.3035310); // rc / 2
    T x6 = radius * T(0.7946545);
    T y1 = radius * T(0.3568221); // a / 2
    T y2 = radius * T(0.5773503); // d / 2
    T y3 = radius * T(0.9341724);
    T z1 = radius * T(0.1875925);
    T z2 = radius * T(0.7946545);

    const std::vector<typename M::point_type> positions =
    {
        {x1, T(0), -z2},
        {x2, y2, -z2},
        {-x3, y1, -z2},
        {-x3, -y1, -z2},
        {x2, -y2, -z2},
        {x4, T(0), -z1},
        {x5, y3, -z1},
        {-x6, y2, -z1},
        {-x6, -y2, -z1},
        {x5, -y3, -z1},
        {x6, y2, z1},
        {-x5, y3, z1},
        {-x4, T(0), z1},
        {-x5, -y3, z1},
        {x6, -y2, z1},
        {x3, y1, z2},
        {-x2, y2, z2},
        {-x1, T(0), z2},
        {-x2, -y2, z2},
        {x3, -y1, z2}
    };

    std::vector<id_type> vertex_position_ids =
    {
        0, 4, 3, 2, 1,
        0, 1, 6, 10, 5,
        1, 2, 7, 11, 6,
        2, 3, 8, 12, 7,
        3, 4, 9, 13, 8,
        4, 0, 5, 14, 9,
        5, 10, 15, 19, 14,
        6, 11, 16, 15, 10,
        7, 12, 17, 16, 11,
        8, 13, 18, 17, 12,
        9, 14, 19, 18, 13,
        15, 16, 17, 18, 19
    };

    T xt2 = a / (T(2) * d);
    T yt1 = T(1) - (ri + rc) / d;
    T ah = a * sqrt(T(4) - gr * gr) / (T(2) * d);
    T yt2 = yt1 + ah;

    const std::vector<texcoord_type> texcoords =
    {
        {T(0.5), yt1},
        {T(0), yt2},
        {T(0.5) - xt2, T(1)},
        {T(0.5) + xt2, T(1)},
        {T(1), yt2}
    };

    M m{};
    id_type submesh_id = m.create_submesh(name);

    m.create_halfedge(25,  1,  4,  0, 0);
    m.create_halfedge(20,  2,  0,  1, 0);
    m.create_halfedge(15,  3,  1,  2, 0);
    m.create_halfedge(10,  4,  2,  3, 0);
    m.create_halfedge( 5,  0,  3,  4, 0);

    m.create_halfedge( 4,  6,  9,  5, 1);
    m.create_halfedge(14,  7,  5,  6, 1);
    m.create_halfedge(39,  8,  6,  7, 1);
    m.create_halfedge(30,  9,  7,  8, 1);
    m.create_halfedge(26,  5,  8,  9, 1);

    m.create_halfedge( 3, 11, 14, 10, 2);
    m.create_halfedge(19, 12, 10, 11, 2);
    m.create_halfedge(44, 13, 11, 12, 2);
    m.create_halfedge(35, 14, 12, 13, 2);
    m.create_halfedge( 6, 10, 13, 14, 2);

    m.create_halfedge( 2, 16, 19, 15, 3);
    m.create_halfedge(24, 17, 15, 16, 3);
    m.create_halfedge(49, 18, 16, 17, 3);
    m.create_halfedge(40, 19, 17, 18, 3);
    m.create_halfedge(11, 15, 18, 19, 3);

    m.create_halfedge( 1, 21, 24, 20, 4);
    m.create_halfedge(29, 22, 20, 21, 4);
    m.create_halfedge(54, 23, 21, 22, 4);
    m.create_halfedge(45, 24, 22, 23, 4);
    m.create_halfedge(16, 20, 23, 24, 4);

    m.create_halfedge( 0, 26, 29, 25, 5);
    m.create_halfedge( 9, 27, 25, 26, 5);
    m.create_halfedge(34, 28, 26, 27, 5);
    m.create_halfedge(50, 29, 27, 28, 5);
    m.create_halfedge(21, 25, 28, 29, 5);

    m.create_halfedge( 8, 31, 34, 30, 6);
    m.create_halfedge(38, 32, 30, 31, 6);
    m.create_halfedge(59, 33, 31, 32, 6);
    m.create_halfedge(51, 34, 32, 33, 6);
    m.create_halfedge(27, 30, 33, 34, 6);

    m.create_halfedge(13, 36, 39, 35, 7);
    m.create_halfedge(43, 37, 35, 36, 7);
    m.create_halfedge(55, 38, 36, 37, 7);
    m.create_halfedge(31, 39, 37, 38, 7);
    m.create_halfedge( 7, 35, 38, 39, 7);

    m.create_halfedge(18, 41, 44, 40, 8);
    m.create_halfedge(48, 42, 40, 41, 8);
    m.create_halfedge(56, 43, 41, 42, 8);
    m.create_halfedge(36, 44, 42, 43, 8);
    m.create_halfedge(12, 40, 43, 44, 8);

    m.create_halfedge(23, 46, 49, 45, 9);
    m.create_halfedge(53, 47, 45, 46, 9);
    m.create_halfedge(57, 48, 46, 47, 9);
    m.create_halfedge(41, 49, 47, 48, 9);
    m.create_halfedge(17, 45, 48, 49, 9);

    m.create_halfedge(28, 51, 54, 50, 10);
    m.create_halfedge(33, 52, 50, 51, 10);
    m.create_halfedge(58, 53, 51, 52, 10);
    m.create_halfedge(46, 54, 52, 53, 10);
    m.create_halfedge(22, 50, 53, 54, 10);

    m.create_halfedge(37, 56, 59, 55, 11);
    m.create_halfedge(42, 57, 55, 56, 11);
    m.create_halfedge(47, 58, 56, 57, 11);
    m.create_halfedge(52, 59, 57, 58, 11);
    m.create_halfedge(32, 55, 58, 59, 11);

    for (size_t i = 0; i < m.halfedge_count(); ++i)
    {
        typename M::vertex_attributes_type attributes {positions[vertex_position_ids[i]], {}, texcoords[i % 5]};
        m.create_vertex(i, attributes);
    }

    for (size_t i = 0; i < 12; ++i)
    {
        id_type surface_id = m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, i));
        id_type face_id = m.create_face(surface_id, 5 * i);
        typename M::face_type& face = m.face(face_id);
        set_facet_normal(face);
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_icosahedron(M& mesh, const std::string& name, data_value_type<M> radius, bool bVertex, bool bSmooth)
{
    using T = M::value_type;
    using point_type = M::point_type;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    assert(radius > T(0));

    M m{};
    id_type submesh_id = m.create_submesh(name);

    bool bFace = !bVertex;

    std::vector<point_type> vertex_positions;

    // a = 1.0514622242382672120513381696958
    // gr = 1.6180339887498948482045868343656

    if (bVertex)
    {
        // Vertices
        // a = 4 / sqrt(10 + 2 * sqrt(5)); edge length of unit icosahedron
        // gr = (1 + sqrt(5)) / 2
        T x0 = radius * T(0.89442719099991587856366946749247); // x0 + x2 = gr
        T x1 = radius * T(0.27639320225002103035908263312687); // x1 + x2 = 1
        T x2 = radius * T(0.72360679774997896964091736687313); // x1 = x0 - gr + 1
        T y1 = radius * T(0.85065080835203993218154049706299); // a * gr / 2
        T y2 = radius * T(0.5257311121191336060256690848479); // a / 2
        T z1 = radius * T(0.44721359549995793928183473374624); // x0 / 2
        T z2 = radius;

        vertex_positions =
        {
            {T(0), T(0), -z2},
            {x2, y2, -z1},
            {-x1, y1, -z1},
            {-x0, T(0), -z1},
            {-x1, -y1, -z1},
            {x2, -y2, -z1},
            {x0, T(0), z1},
            {x1, y1, z1},
            {-x2, y2, z1},
            {-x2, -y2, z1},
            {x1, -y1, z1},
            {T(0), T(0), z2},
        };
    }
    else if (bFace)
    {
        // Vertices
        // a = 4 / sqrt(10 + 2 * sqrt(5)); edge length of unit icosahedron
        T x1 = radius * T(0.30353099910334311154769579071001); // a * sqrt(3) / 6
        T x2 = radius * T(0.60706199820668622309539158142001); // a * sqrt(3) / 3
        T x3 = radius * T(0.49112347318842301140783513761761); // x1 * gr
        T x4 = radius * T(0.98224694637684602281567027523522); // x2 * gr
        T y1 = radius * T(0.85065080835203993218154049706299); // a * gr / 2
        T y2 = radius * T(0.5257311121191336060256690848479); // a / 2
        T z1 = radius * T(0.18759247408507989986013934690762); // x1 / gr
        T z2 = radius * T(0.79465447229176612295553092832763);

        vertex_positions =
        {
            {-x2, T(0), -z2},
            {x1, y2, -z2},
            {-x3, y1, -z1},
            {-x4, T(0), z1},
            {-x3, -y1, -z1},
            {x1, -y2, -z2},
            {x4, T(0), -z1},
            {x3, y1, z1},
            {-x1, y2, z2},
            {-x1, -y2, z2},
            {x3, -y1, z1},
            {x2, T(0), z2},
        };
    }
    else // bEdge
    {
/*
        const T gr = (T(1) + sqrt(T(5))) / T(2);
        const T r = sqrt((T(5) + sqrt(T(5))) / T(2));
        const T n1 = T(1) / r;
        const T ngr = gr / r;

        vertex_positions =
        {
            {{T(0), n1, ngr}, {T(0), n1, ngr}},
            {{T(0), -n1, ngr}, {T(0), -n1, ngr}},
            {{ngr, T(0), n1}, {ngr, T(0), n1}},
            {{-ngr, T(0), n1}, {-ngr, T(0), n1}},
            {{n1, ngr, T(0)}, {n1, ngr, T(0)}},
            {{-n1, ngr, T(0)}, {-n1, ngr, T(0)}},
            {{n1, -ngr, T(0)}, {n1, -ngr, T(0)}},
            {{-n1, -ngr, T(0)}, {-n1, -ngr, T(0)}},
            {{ngr, T(0), -n1}, {ngr, T(0), -n1}},
            {{-ngr, T(0), -n1}, {-ngr, T(0), -n1}},
            {{T(0), n1, -ngr}, {T(0), n1, -ngr}},
            {{T(0), -n1, -ngr}, {T(0), -n1, -ngr}},
        };

        // reorder above so that this is not necessary ...
        vertex_position_ids =
        {
             0, 1, 2,
             0, 3, 1,
             0, 5, 3,
             0, 4, 5,
             0, 2, 4,
             2, 6, 8,
             2, 1, 6,
             1, 7, 6,
             1, 3, 7,
             3, 9, 7,
             3, 5, 9,
             5, 10, 9,
             5, 4, 10,
             4, 8, 10,
             4, 2, 8,
             8, 6, 11,
             6, 7, 11,
             7, 9, 11,
             9, 10, 11,
            10, 8, 11
        };
*/
    }

    std::vector<id_type> face_vertex_position_ids =
    {
        0, 1, 5,
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 5, 4,
        1, 6, 5,
        2, 7, 1,
        3, 8, 2,
        4, 9, 3,
        5, 10, 4,
        6, 1, 7,
        7, 2, 8,
        8, 3, 9,
        9, 4, 10,
        10, 5, 6,
        11, 6, 7,
        11, 7, 8,
        11, 8, 9,
        11, 9, 10,
        11, 10, 6
    };

    vertices_attributes_type<M> avs(vertex_positions.size());
    for (size_t i = 0; i < vertex_positions.size(); ++i)
    {
        avs[i].set_position(vertex_positions[i]);
        if (bSmooth)
        {
            auto normal = direction(vertex_positions[i]); // simpler than surface_vertex_normal and can be done earlier ...
            avs[i].set_normal(normal);
            avs[i].set_texcoord(calculate_spherical_texcoord<M>(normal));
        }
    }

    m.create_halfedge_vertex( 5,  1,  2,  0, avs[face_vertex_position_ids[ 0]]);
    m.create_halfedge_vertex(17,  2,  0,  0, avs[face_vertex_position_ids[ 1]]);
    m.create_halfedge_vertex(12,  0,  1,  0, avs[face_vertex_position_ids[ 2]]);

    m.create_halfedge_vertex( 8,  4,  5,  1, avs[face_vertex_position_ids[ 3]]);
    m.create_halfedge_vertex(20,  5,  3,  1, avs[face_vertex_position_ids[ 4]]);
    m.create_halfedge_vertex( 0,  3,  4,  1, avs[face_vertex_position_ids[ 5]]);

    m.create_halfedge_vertex(11,  7,  8,  2, avs[face_vertex_position_ids[ 6]]);
    m.create_halfedge_vertex(23,  8,  6,  2, avs[face_vertex_position_ids[ 7]]);
    m.create_halfedge_vertex( 3,  6,  7,  2, avs[face_vertex_position_ids[ 8]]);

    m.create_halfedge_vertex(14, 10, 11,  3, avs[face_vertex_position_ids[ 9]]);
    m.create_halfedge_vertex(26, 11,  9,  3, avs[face_vertex_position_ids[10]]);
    m.create_halfedge_vertex( 6,  9, 10,  3, avs[face_vertex_position_ids[11]]);

    m.create_halfedge_vertex( 2, 13, 14,  4, avs[face_vertex_position_ids[12]]);
    m.create_halfedge_vertex(29, 14, 12,  4, avs[face_vertex_position_ids[13]]);
    m.create_halfedge_vertex( 9, 12, 13,  4, avs[face_vertex_position_ids[14]]);

    m.create_halfedge_vertex(30, 16, 17,  5, avs[face_vertex_position_ids[15]]);
    m.create_halfedge_vertex(43, 17, 15,  5, avs[face_vertex_position_ids[16]]);
    m.create_halfedge_vertex( 1, 15, 16,  5, avs[face_vertex_position_ids[17]]);

    m.create_halfedge_vertex(33, 19, 20,  6, avs[face_vertex_position_ids[18]]);
    m.create_halfedge_vertex(31, 20, 18,  6, avs[face_vertex_position_ids[19]]);
    m.create_halfedge_vertex( 4, 18, 19,  6, avs[face_vertex_position_ids[20]]);

    m.create_halfedge_vertex(36, 22, 23,  7, avs[face_vertex_position_ids[21]]);
    m.create_halfedge_vertex(34, 23, 21,  7, avs[face_vertex_position_ids[22]]);
    m.create_halfedge_vertex( 7, 21, 22,  7, avs[face_vertex_position_ids[23]]);

    m.create_halfedge_vertex(39, 25, 26,  8, avs[face_vertex_position_ids[24]]);
    m.create_halfedge_vertex(37, 26, 24,  8, avs[face_vertex_position_ids[25]]);
    m.create_halfedge_vertex(10, 24, 25,  8, avs[face_vertex_position_ids[26]]);

    m.create_halfedge_vertex(42, 28, 29,  9, avs[face_vertex_position_ids[27]]);
    m.create_halfedge_vertex(40, 29, 27,  9, avs[face_vertex_position_ids[28]]);
    m.create_halfedge_vertex(13, 27, 28,  9, avs[face_vertex_position_ids[29]]);

    m.create_halfedge_vertex(15, 31, 32, 10, avs[face_vertex_position_ids[30]]);
    m.create_halfedge_vertex(19, 32, 30, 10, avs[face_vertex_position_ids[31]]);
    m.create_halfedge_vertex(46, 30, 31, 10, avs[face_vertex_position_ids[32]]);

    m.create_halfedge_vertex(18, 34, 35, 11, avs[face_vertex_position_ids[33]]);
    m.create_halfedge_vertex(22, 35, 33, 11, avs[face_vertex_position_ids[34]]);
    m.create_halfedge_vertex(49, 33, 34, 11, avs[face_vertex_position_ids[35]]);

    m.create_halfedge_vertex(21, 37, 38, 12, avs[face_vertex_position_ids[36]]);
    m.create_halfedge_vertex(25, 38, 36, 12, avs[face_vertex_position_ids[37]]);
    m.create_halfedge_vertex(52, 36, 37, 12, avs[face_vertex_position_ids[38]]);

    m.create_halfedge_vertex(24, 40, 41, 13, avs[face_vertex_position_ids[39]]);
    m.create_halfedge_vertex(28, 41, 39, 13, avs[face_vertex_position_ids[40]]);
    m.create_halfedge_vertex(55, 39, 40, 13, avs[face_vertex_position_ids[41]]);

    m.create_halfedge_vertex(27, 43, 44, 14, avs[face_vertex_position_ids[42]]);
    m.create_halfedge_vertex(16, 44, 42, 14, avs[face_vertex_position_ids[43]]);
    m.create_halfedge_vertex(58, 42, 43, 14, avs[face_vertex_position_ids[44]]);

    m.create_halfedge_vertex(59, 46, 47, 15, avs[face_vertex_position_ids[45]]);
    m.create_halfedge_vertex(32, 47, 45, 15, avs[face_vertex_position_ids[46]]);
    m.create_halfedge_vertex(48, 45, 46, 15, avs[face_vertex_position_ids[47]]);

    m.create_halfedge_vertex(47, 49, 50, 16, avs[face_vertex_position_ids[48]]);
    m.create_halfedge_vertex(35, 50, 48, 16, avs[face_vertex_position_ids[49]]);
    m.create_halfedge_vertex(51, 48, 49, 16, avs[face_vertex_position_ids[50]]);

    m.create_halfedge_vertex(50, 52, 53, 17, avs[face_vertex_position_ids[51]]);
    m.create_halfedge_vertex(38, 53, 51, 17, avs[face_vertex_position_ids[52]]);
    m.create_halfedge_vertex(54, 51, 52, 17, avs[face_vertex_position_ids[53]]);

    m.create_halfedge_vertex(53, 55, 56, 18, avs[face_vertex_position_ids[54]]);
    m.create_halfedge_vertex(41, 56, 54, 18, avs[face_vertex_position_ids[55]]);
    m.create_halfedge_vertex(57, 54, 55, 18, avs[face_vertex_position_ids[56]]);

    m.create_halfedge_vertex(56, 58, 59, 19, avs[face_vertex_position_ids[57]]);
    m.create_halfedge_vertex(44, 59, 57, 19, avs[face_vertex_position_ids[58]]);
    m.create_halfedge_vertex(45, 57, 58, 19, avs[face_vertex_position_ids[59]]);

    if (bSmooth)
    {
        id_type surface_id = m.create_surface(submesh_id, SurfaceName::Body);

        for (size_t i = 0; i < 20; ++i)
        {
            size_t j = 3 * i;
            id_type face_id = m.create_face(surface_id, j);
            auto& face = m.face(face_id);
            auto normal = geometry::normal(face);
            face.attributes().set_normal(normal);
            fix_spherical_texcoords(face);
        }
    }
    else
    {
        const std::array<texcoord_type, 3> face_texcoords
        {{
            {T(0.5), T(0)},
            {T(0), T(1)},
            {T(1), T(1)},
        }};

        for (size_t i = 0; i < 20; ++i)
        {
            size_t j = 3 * i;
            std::vector<point_type> points
            {
                vertex_positions[face_vertex_position_ids[j + 0]],
                vertex_positions[face_vertex_position_ids[j + 1]],
                vertex_positions[face_vertex_position_ids[j + 2]],
            };
            auto normal = direction(vertex_centroid(points)); // geometry::normal(points) ...
            id_type surface_id = m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, i), {normal});
            id_type face_id = m.create_face(surface_id, j, {normal});

            size_t k = 0;
            for (auto& halfedge : m.face(face_id).halfedges())
            {
                halfedge.vertex_attributes().set_normal(normal);
                halfedge.vertex_attributes().set_texcoord(face_texcoords[k++]); // triangular texcoords? ...
            }
        }
    }

    m.check();
    assert(check_spherical(m, radius).ok());
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_geodesic_sphere(M& mesh, const std::string& name, data_value_type<M> radius, size_t nSubdivisions, bool bVertex, bool bSmooth)
{
    using T = M::value_type;

    assert(radius > T(0));
    assert(nSubdivisions > 0);

    // Approximate a sphere by tessellating an icosahedron

    M m{};
    create_icosahedron(m, name, radius, bVertex, bSmooth);
    if (nSubdivisions == 1)
    {
        mesh.insert(m);
        return mesh.submesh_id(name);
    }

    auto nFacesOrig = m.face_count();
    for (size_t i = 0; i < nFacesOrig; ++i)
    {
        // Split all unmarked edges
        const id_type idHalfedge0 = m.face(i).halfedge_id();
        id_type idHalfedge = idHalfedge0;
        do
        {
            id_type idHalfedgeNext = m.halfedge(idHalfedge).next_id();

            // Split unmarked edge
            if (!m.halfedge(idHalfedge).marked())
            {
                split_edge(m, idHalfedge, nSubdivisions);

                // Mark partners of this edge's segments so that adjacent face edges will not be split
                id_type idHalfedgeEdge = idHalfedge;
                for (int j = 0; j < nSubdivisions; ++j)
                {
                    m.halfedge(idHalfedgeEdge).partner().set_marked();
                    idHalfedgeEdge = m.halfedge(idHalfedgeEdge).next_id();
                }
            }

            idHalfedge = idHalfedgeNext;
        } while (idHalfedge != idHalfedge0);

        // Recursively triangulate by row
        triangulate_rows(m, m.face(i).halfedge_id(), nSubdivisions);
    }

    project_vertices(m.vertices().begin(), m.vertices().end(), radius);

    if (bSmooth)
    {
        calculate_spherical_normals(m);
        calculate_spherical_texcoords(m); // spherical texture coordinates, with smooth icosahedron to start, splits should produce mostly correct texture coordinates ...
    }
    else
    {
        for (auto& face : m.faces())
        {
            create_facet(m, face);
        }
    }

    m.check();
    assert(check_spherical(m, radius).ok());
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_extrusion(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const normal_vector_type<M>& normal, const typename M::vector_type& displacement, const Properties& properties)
{
    using T = M::value_type;
    using point_type = geometry::Polygon<M::vector_traits>::point_type;

    CylinderSpecification<T> spec(polygon.edge_count(), 1, T(0), T(0), T(0), T(0), properties);

    size_t face_count = polygon.edge_count();
    size_t nz = spec.axis_divisions; // nz > 1 not supported ...

    // this function is a mix of smooth and sectioned styles ...

    assert(face_count > 2);
    assert(nz > 0);

    size_t section_count = face_count;

    auto avs0 = vertices_attributes<M>(polygon, {T(0), T(1), T(1)});

for (const auto& av : avs0)
{
const auto& texcoord = av.texcoord();
assert(texcoord.y() == T(0) || texcoord.y() == T(1));
}

    geometry::Polygon<typename M::vector_traits> polygon1;
    std::transform(polygon.vertices().begin(), polygon.vertices().end(), std::back_inserter(polygon1.vertices()), [displacement](const point_type& point) -> point_type { return point + displacement; });
    auto avs1 = vertices_attributes<M>(polygon1, {T(0), T(1), T(0)});

for (const auto& av : avs1)
{
const auto& texcoord = av.texcoord();
assert(texcoord.y() == T(0) || texcoord.y() == T(1));
}

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });

    auto ts0 = texture_span<T>(0, 1, false, false);
    auto ts1 = texture_span<T>(1, 1, false, false);

    create_band(m, avs0, avs1, ts0, ts1, true, surface_id0, true);
for (const auto& face : m.faces())
{
for (const auto& halfedge : face.halfedges())
{
const auto& texcoord = halfedge.vertex_attributes().texcoord();
assert(texcoord.y() == T(0) || texcoord.y() == T(1));
}
}

    for (size_t i = 0; i < face_count; ++i)
    {
        m.surface(i).attributes().set_normal(m.face(i).attributes().normal());
    }

    seal_cylinder(m, submesh_id, spec, false, false, -normal, normal);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_extrusion(M& mesh, const std::string& name, const geometry::PolygonWithHoles<typename M::vector_traits>& polygon_holes, const normal_vector_type<M>& normal, const typename M::vector_type& displacement, const Properties& properties)
{
    using T = M::value_type;
    using point_type = geometry::Polygon<typename M::vector_traits>::point_type;

    CylinderSpecification<T> spec(polygon_holes.polygon().edge_count(), 1, T(0), T(0), T(0), T(0), properties);

    size_t nAzimuth = polygon_holes.polygon().edge_count();
    size_t nz = spec.axis_divisions; // nz > 1 not supported ...

    assert(nAzimuth > 2);
    assert(nz > 0);

    auto avs0 = vertices_attributes<M>(polygon_holes.polygon(), {T(0), T(1), T(1)});

    geometry::Polygon<typename M::vector_traits> polygon1;
    // ranges ...
    std::transform(polygon_holes.polygon().vertices().begin(), polygon_holes.polygon().vertices().end(), std::back_inserter(polygon1.vertices()), [displacement](const point_type& point) -> point_type { return point + displacement; });
    auto avs1 = vertices_attributes<M>(polygon1, {T(0), T(1), T(0)});

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = mesh.surface_store_count();

    id_type idHalfedge_lower = 0;
    id_type idHalfedge_upper = 0;
    std::vector<id_type> idHalfedgeHoles_lower(polygon_holes.hole_count());
    std::vector<id_type> idHalfedgeHoles_upper(polygon_holes.hole_count());

    for (size_t i = 0; i < nz; ++i)
    {
        auto ts0 = texture_span<T>(0, 1, false, false);
        auto ts1 = texture_span<T>(1, 1, false, false);

        //  weird to have a band per axis division, is this intentional ...
        id_type surface_id = m.create_surface(submesh_id, surface_name(SurfaceName::BodySection, surface_id0));
        create_band(m, avs0, avs1, ts0, ts1, true, surface_id, true);
idHalfedge_upper = m.halfedge_store_count() - 2 * nAzimuth; // after last layer ...

        for (size_t j = 0; j < polygon_holes.hole_count(); ++j)
        {
            const auto& polygon = polygon_holes.hole(j);

            avs0 = vertices_attributes<M>(polygon, {T(0), T(1), T(1)});

            polygon1.clear();
            std::transform(polygon.vertices().begin(), polygon.vertices().end(), std::back_inserter(polygon1.vertices()), [displacement](const point_type& point) -> point_type { return point + displacement; });
            avs1 = vertices_attributes<M>(polygon1, {T(0), T(1), T(0)});

            idHalfedgeHoles_lower[j] = m.halfedge_store_count();

            M mm{};
            surface_id = mm.create_surface(mm.create_submesh(name), surface_name(SurfaceName::BodySection, surface_id0 + 1 + j));
            create_band(mm, avs0, avs1, ts0, ts1, true, surface_id, true);
            m.insert(mm);

            assert(m.halfedge(idHalfedgeHoles_lower[j]).border());
            idHalfedgeHoles_upper[j] = m.halfedge_store_count() - 2 * polygon.edge_count();
            assert(m.halfedge(idHalfedgeHoles_upper[j]).border());
        }

//        idHalfedge_upper = m.halfedge_store_count() - 2 * nAzimuth; // after last layer ...
    }

    assert(m.halfedge(idHalfedge_lower).border());
    assert(m.halfedge(idHalfedge_upper).border());

    // for nz > 1, mesh per hole, keep track of border halfedges, then invert, create_border_with_holes_face

// seal_cylinder here needs to use create_border_with_holes_face ...
//    seal_cylinder(m, submesh_id, spec, false, false, -normal, normal);

    if (spec.axis_termination_lower.style == TerminationStyle::Butt)
    {
        id_type surface_id_lower = m.create_surface(submesh_id, spec.axis_termination_lower.name, {-normal});

        id_type face_id = create_border_with_holes_face(m, idHalfedge_lower, idHalfedgeHoles_lower, -normal, surface_id_lower);
        calculate_face_texcoords(m, face_id);
    }

    if (spec.axis_termination_upper.style == TerminationStyle::Butt)
    {
        id_type surface_id_upper = m.create_surface(submesh_id, spec.axis_termination_upper.name, {normal});

        id_type face_id = create_border_with_holes_face(m, idHalfedge_upper, idHalfedgeHoles_upper, normal, surface_id_upper);
        calculate_face_texcoords(m, face_id);
    }

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties)
{
//std::cout << "create_swept_volume polygon spec" << std::endl;
    assert(polygon.edge_count() > 2);
    assert(path.size() > 1);

    using T = M::value_type;

    size_t face_count = polygon.edge_count();
    size_t nz = path.size() - 1;

    CylinderSpecification<T> spec(face_count, nz, T(0), T(0), T(0), T(0), properties);

    assert(spec.azimuth_smooth);
    assert(spec.azimuth_interval.unit());

    bool cusped_lower = false;
    bool cusped_upper = false;
    bool closed = true;

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = m.create_surface(submesh_id, SurfaceName::Body);
 
    id_type j0_orig = m.halfedge_store_count();

    auto avs0 = vertices_attributes<M>(polygon, path[0], ts0);
    auto avs1 = vertices_attributes<M>(polygon, path[1], ts1);
    create_band(m, avs0, avs1, surface_id0, closed);

    for (size_t i = 2; i <= nz; ++i)
    {
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        auto avs1 = vertices_attributes<M>(polygon, path[i], ts1);
        connect_band(m, avs1, surface_id0, closed);
    }

    calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);

    typename M::vector_type normal_lower = math::normalize(-path.front().orientation().direction());
    typename M::vector_type normal_upper = math::normalize(path.back().orientation().direction());
    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper, normal_lower, normal_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties)
{
//std::cout << "create_swept_volume_edges polygon spec" << std::endl;
    assert(polygon.edge_count() > 2);
    assert(path.size() > 1);

    using T = M::value_type;

    size_t face_count = polygon.edge_count();
    size_t nz = path.size() - 1;

    CylinderSpecification<T> spec(face_count, nz, T(0), T(0), T(0), T(0), properties);

    size_t section_count = face_count;
    size_t section_face_count = 1;

    assert(!spec.azimuth_smooth);
    assert(spec.azimuth_interval.unit());

    bool cusped_lower = false;
    bool cusped_upper = false;
    bool closed = true;

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });
 
    id_type j0_orig = m.halfedge_store_count();

    auto avss0 = vertices_attributes_edges<M>(polygon, path[0], ts0);
    auto avss1 = vertices_attributes_edges<M>(polygon, path[1], ts1);
    create_band(m, avss0, avss1, surface_id0, closed);

    for (size_t i = 2; i <= nz; ++i)
    {
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        auto avss1 = vertices_attributes_edges<M>(polygon, path[i], ts1);
        connect_band(m, avss1, surface_id0, closed);
    }

    // Calculate normals for the upper vertices
    size_t n = section_face_count;
    id_type id = j0_orig + 2 * face_count;
    for (size_t i = 0; i < section_count; ++i)
    {
        calculate_upper_surface_vertex_normals_sequential(m, id, n);
        id += n;
    }

    typename M::vector_type normal_lower = math::normalize(-path.front().orientation().direction());
    typename M::vector_type normal_upper = math::normalize(path.back().orientation().direction());
    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper, normal_lower, normal_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, const Properties& properties)
{
//std::cout << "create_swept_volume_sections polygon spec" << std::endl;
    assert(polygon.edge_count() > 2);
    assert(path.size() > 1);

    using T = M::value_type;

    size_t face_count = polygon.edge_count();
    size_t nz = path.size() - 1;

    CylinderSpecification<T> spec(face_count, nz, T(0), T(0), T(0), T(0), properties);

    assert(polygon.sections() && !polygon.sections().value().empty());
    size_t section_count = polygon.sections().value().size();
    assert(section_count == 1 && spec.azimuth_smooth || !spec.azimuth_smooth); 
    assert(spec.azimuth_interval.unit());

    bool cusped_lower = false;
    bool cusped_upper = false;
    bool closed = true;

    auto ts0 = texture_span<T>(0, nz, cusped_lower, cusped_upper);
    auto ts1 = texture_span<T>(1, nz, cusped_lower, cusped_upper);

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(m, submesh_id, section_count, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });
 
    id_type j0_orig = m.halfedge_store_count();

    auto avss0 = vertices_attributes_sections<M>(polygon, path[0], ts0);
    auto avss1 = vertices_attributes_sections<M>(polygon, path[1], ts1);
    create_band(m, avss0, avss1, surface_id0, closed);

    for (size_t i = 2; i <= nz; ++i)
    {
        ts1 = texture_span<T>(i, nz, cusped_lower, cusped_upper);

        j0_orig = m.halfedge_store_count();

        auto avss1 = vertices_attributes_sections<M>(polygon, path[i], ts1);
        connect_band(m, avss1, surface_id0, closed);
    }

    // Calculate normals for the upper vertices
    id_type id = j0_orig + 2 * face_count;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss1[i].size() - 1;
        calculate_upper_surface_vertex_normals_sequential(m, id, n);
        id += n;
    }

    typename M::vector_type normal_lower = math::normalize(-path.front().orientation().direction());
    typename M::vector_type normal_upper = math::normalize(path.back().orientation().direction());
    seal_cylinder(m, submesh_id, spec, cusped_lower, cusped_upper, normal_lower, normal_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<geometry::Polygon<typename M::vector_traits>(const geometry::Polygon<typename M::vector_traits>&, data_value_type<M>)> f, const Properties& properties)
{
    assert(polygon.edge_count() > 2);
    assert(path.size() > 1);

    using T = M::value_type;

    size_t nAzimuth = polygon.edge_count();
    size_t nz = path.size() - 1;

    CylinderSpecification<T> spec(nAzimuth, nz, T(0), T(0), T(0), T(0), properties);

    assert(spec.azimuth_smooth); // need another version for sections ...
    assert(spec.azimuth_interval.unit());
    assert(spec.azimuth_divisions == polygon.edge_count());

    size_t face_count = spec.azimuth_divisions;
    bool closed = true;

    M m{};
    id_type submesh_id = m.create_submesh(name);
    id_type surface_id0 = nullid;
    if (spec.azimuth_smooth)
    {
        surface_id0 = m.create_surface(submesh_id, SurfaceName::Body);
    }
    else
    {
        surface_id0 = create_section_surfaces(m, submesh_id, nAzimuth, [](size_t i) { return surface_name(SurfaceName::BodySection, i); });
    }

    T t = T(1) / T(nz);

    auto ts0 = texture_span<T>(0, nz, false, false);
    auto ts1 = texture_span<T>(1, nz, false, false);

    auto avs0 = vertices_attributes<M>(f(polygon, T(0)), path[0], ts0);
    auto avs1 = vertices_attributes<M>(f(polygon, t), path[1], ts1);

    id_type j0_orig = m.halfedge_store_count();

    create_band(m, avs0, avs1, surface_id0, closed);

    for (size_t i = 2; i <= nz; ++i)
    {
        t = T(i) / T(nz);
        ts1 = texture_span<T>(i, nz, false, false);
        avs1 = vertices_attributes<M>(f(polygon, t), path[i], ts1);

        j0_orig = m.halfedge_store_count();

        connect_band(m, avs1, surface_id0, closed);
    }

    calculate_upper_surface_vertex_normals_sequential(m, j0_orig + 2 * face_count, face_count);

    typename M::vector_type normal_lower = math::normalize(-path.front().orientation().direction());
    typename M::vector_type normal_upper = math::normalize(path.back().orientation().direction());
    seal_cylinder(m, submesh_id, spec, false, false, normal_lower, normal_upper);

    m.check();
    mesh.insert(m);
    return mesh.submesh_id(name);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties)
{
    return nullid; // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume_edges(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties)
{
    return nullid; // ...
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_swept_volume_sections(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, const geometry::Path<typename M::vector_traits>& path, std::function<data_value_type<M>(data_value_type<M>)> fr, std::optional<std::function<data_value_type<M>(data_value_type<M>)>> fa, const Properties& properties)
{
    return nullid; // ...
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_end_round_regular(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo)
{
    using T = M::value_type;

    id_type submesh_id = mesh.submesh_id(nameSubmesh); // submesh_id parameter ...
    id_type surface_id_from = mesh.surface_id(submesh_id, nameSurfaceFrom);

    id_type idHalfedgeBorder = nullid;
    for (const auto& face : mesh.surface(surface_id_from).faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.partner().face().surface_id() != surface_id_from)
            {
                idHalfedgeBorder = halfedge.partner_id();
                break;
            }
        }

        if (idHalfedgeBorder != nullid)
        {
            break;
        }
    }

    auto up = mesh.surface(surface_id_from).attributes().normal();
    mesh.delete_surface(surface_id_from);
    id_type surface_id_to = mesh.create_surface(submesh_id, nameSurfaceTo, {up});

    // advance idHalfedgeBorder to the halfedge where the next texcoord x value is minimum (typically 0) to align body and end textures
    // the border could be made up of several surfaces, this will typically find the first surface edge with a texcoord of 0
    // could use a border iterator here ...
    T txMin = mesh.halfedge(idHalfedgeBorder).next().vertex_attributes().texcoord().x();
    id_type idEnd = idHalfedgeBorder;
    id_type id = idHalfedgeBorder;
    do
    {
        assert(mesh.halfedge(id).border());
        T tx = mesh.halfedge(id).next().vertex_attributes().texcoord().x();
        if (tx < txMin)
        {
            txMin = tx;
            idHalfedgeBorder = id;
        }
        id = next_border_halfedge_id(mesh, id);
    } while (id != idEnd);

    auto halfedge = mesh.halfedge(idHalfedgeBorder);
    auto polygon = to_border_polygon(mesh, idHalfedgeBorder);
    auto c0 = polygon.centroid();
    auto nAzimuth = polygon.vertex_count();
    auto normal0 = halfedge.vertex_attributes().normal();
    T theta1 = math::PiHalf<T>;
    T theta0 = elevation_angle(normal0);
    T r0 = (polygon.vertex(0) - c0).norm();
    auto center = c0 + up * (-r0 * tan(theta0));
    T r = (polygon.vertex(0) - center).norm();
    T rr = r / r0;
    typename M::vertex_attributes_type avApex = {center + r * up, up, {T(0.5), T(0)}};

    auto idPartners = border_halfedges(mesh, idHalfedgeBorder);
    assert(idPartners.size() == nAzimuth);

    T hBand = std::accumulate(idPartners.begin(), idPartners.end(), T(0), [mesh](const T& a, id_type id) { return a + to_segment(mesh.halfedge(id).next()).length(); }) / nAzimuth;
    size_t nBands = static_cast<size_t>(T(2) * r * (theta1 - theta0) / hBand);
    if (nBands < 3)
    {
        nBands = 3;
    } 

    for (size_t i = 1; i < nBands; ++i)
    {
        auto t = T(i) / nBands;
        auto theta = math::lerp(theta0, theta1, t);

        auto scale = rr * math::denoise011(cos(theta));
        typename M::vector_type offset = {T(0), T(0), r * math::denoise011(sin(theta))};
        auto matrix = geometry::translation(-c0.vector()) * geometry::scaling(scale) * geometry::translation(to_vector(center + offset));
        auto polygon1 = polygon;
        polygon1.transform(matrix);
        auto ty1 = T(1) - t;

        texture_span_type<M> ts1{T(0), T(1), ty1}; // ...
        auto avs1 = vertices_attributes<M>(polygon1, std::nullopt, ts1); // tx values from avs0, should be identical, but precision ...
        if (i == 1)
        {
            connect_band(mesh, avs1, idPartners, surface_id_to, true);
            continue;
        }

        connect_band(mesh, avs1, surface_id_to, true);
    }

    vertices_attributes_type<M> avs1(nAzimuth, avApex);
    connect_apex_cusp(mesh, avs1, surface_id_to, true);

    // calculate_upper_surface_vertex_normals_sequential? ...

    return;
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type::value_type average(const V& v, size_t n, std::function<typename V::value_type::value_type(const typename V::value_type&)> f)
{
    return std::accumulate(v.begin(), v.end(), typename V::value_type::value_type(0), [f](const typename V::value_type::value_type& a, const typename V::value_type& b) { return a + f(b); }) / n;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_end_round(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo)
{
    using T = M::value_type;

    id_type submesh_id = mesh.submesh_id(nameSubmesh); // submesh_id parameter ...
    id_type surface_id_from = mesh.surface_id(submesh_id, nameSurfaceFrom);

    id_type idHalfedgeBorder = nullid;
    for (const auto& face : mesh.surface(surface_id_from).faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.partner().face().surface_id() != surface_id_from)
            {
                idHalfedgeBorder = halfedge.partner_id();
                break;
            }
        }

        if (idHalfedgeBorder != nullid)
        {
            break;
        }
    }

    auto up = mesh.surface(surface_id_from).attributes().normal();
    mesh.delete_surface(surface_id_from);
    id_type surface_id_to = mesh.create_surface(submesh_id, nameSurfaceTo, {up});

    bool closed = true;

    // advance idHalfedgeBorder to the halfedge where the next texcoord x value is minimum (typically 0) to align body and end textures
    // the border could be made up of several surfaces, this will typically find the first surface edge with a texcoord of 0
    // could use a border iterator here ...
    T txMin = mesh.halfedge(idHalfedgeBorder).next().vertex_attributes().texcoord().x();
    id_type idEnd = idHalfedgeBorder;
    id_type id = idHalfedgeBorder;
    do
    {
        assert(mesh.halfedge(id).border());
        T tx = mesh.halfedge(id).next().vertex_attributes().texcoord().x();
        if (tx < txMin)
        {
            txMin = tx;
            idHalfedgeBorder = id;
        }
        id = next_border_halfedge_id(mesh, id);
    } while (id != idEnd);

    auto avs0 = border_vertices_attributes(mesh, idHalfedgeBorder);
    auto polygon = to_border_polygon(mesh, idHalfedgeBorder);
    auto c0 = polygon.centroid(); // only use of polygon ...
    auto nAzimuth = avs0.size();
    // theta's are elevation relative to plane of polygon ...
    T theta1 = math::PiHalf<T>;
    T theta0 = average(avs0, nAzimuth, [theta1, up](const typename M::vertex_attributes_type& av) { return theta1 - angle_between_unit(av.normal(), up); });
    T r0 = average(avs0, nAzimuth, [c0](const typename M::vertex_attributes_type& av) { return (av.position() - c0).norm(); });
    auto center = c0 + up * (-r0 * tan(theta0));
    T r = average(avs0, nAzimuth, [center](const typename M::vertex_attributes_type& av) { return (av.position() - center).norm(); });
    typename M::vertex_attributes_type avApex = {center + r * up, up, {T(0.5), T(0)}};

    auto idPartners = border_halfedges(mesh, idHalfedgeBorder);
    assert(idPartners.size() == nAzimuth);

    T hBand = std::accumulate(idPartners.begin(), idPartners.end(), T(0), [mesh](const T& a, id_type id) { return a + to_segment(mesh.halfedge(id).next()).length(); }) / nAzimuth;
    size_t nBands = static_cast<size_t>(T(2) * r * (theta1 - theta0) / hBand);
    if (nBands < 3)
    {
        nBands = 3;
    } 

    for (size_t i = 1; i < nBands; ++i)
    {
        auto t = T(i) / nBands;
        auto ty1 = T(1) - t;
        vertices_attributes_type<M> avs1(nAzimuth + 1);

        for (size_t j = 0; j < nAzimuth; ++j)
        {
            auto matrixDirection = geometry::slerp_unit(avs0[j].normal(), avApex.normal(), t);
//            auto matrixPosition = translation(-center) * matrixDirection * translation(center);
auto matrixPosition = geometry::translation(-center.vector()) * matrixDirection * geometry::translation(center.vector());
            auto position = avs0[j].position() * matrixPosition;
            auto normal = normalize(avs0[j].normal() * matrixDirection);
            typename M::vertex_attributes_type::texcoord_type texcoord = {avs0[j].texcoord().x(), ty1};
            avs1[j] = {position, normal, texcoord};
        }

        avs1[nAzimuth] = avs1[0];
        avs1[nAzimuth].texcoord().x() = T(1);

        texture_span_type<M> ts1{T(0), T(1), T(0)}; // ...
        if (i == 1)
        {
            connect_band(mesh, avs1, idPartners, surface_id_to, closed);
            continue;
        }

        connect_band(mesh, avs1, surface_id_to, closed);
    }

    vertices_attributes_type<M> avs1(nAzimuth, avApex);
    connect_apex_cusp(mesh, avs1, surface_id_to, closed);

    // calculate_upper_surface_vertex_normals_sequential? ...

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_end_peaked(M& mesh, const std::string& nameSubmesh, const std::string& nameSurfaceFrom, const std::string& nameSurfaceTo, data_value_type<M> scale)
{
    // ...
    return;
}

#endif // QUETZAL_MODEL_PRIMITIVES_HPP
