#if !defined(QUETZAL_MODEL_PRIMITIVES_SPECIFICATION_HPP)
#define QUETZAL_MODEL_PRIMITIVES_SPECIFICATION_HPP
//------------------------------------------------------------------------------
// model
// primitives_specification.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Properties.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Interval.hpp"
#include <string>

namespace quetzal::model
{

    //--------------------------------------------------------------------------
    enum class TerminationStyle
    {
        None,
        Butt,
        Center,
        Direct,
    };

    //--------------------------------------------------------------------------
    TerminationStyle to_termination_style(const std::string& name)
    {
        if (name == "butt") { return TerminationStyle::Butt; }
        if (name == "center") { return TerminationStyle::Center; }
        if (name == "direct") { return TerminationStyle::Direct; }
        return TerminationStyle::None;
    }

    //--------------------------------------------------------------------------
    struct SideTerminationSpecification
    {
        TerminationStyle style;
        bool subdivision; // Banding
        std::string name;
        std::string lower_name;
        std::string upper_name;
    };

    //--------------------------------------------------------------------------
    struct EndTerminationSpecification
    {
        TerminationStyle style;
        bool subdivision; // Center point, not currently supported
        std::string name;
    };
/*
    //--------------------------------------------------------------------------
    template<typename T>
    struct DimensionSpecification
    {
        math::Interval<T> interval;
        math::Interval<T> secondary_interval;
        math::Interval<T> tertiary_interval;
        math::Interval<T> intervals[];
        size_t divisions;
        bool smooth;
        TerminationSpecification lower;
        TerminationSpecification upper;
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct PrimitiveSpecification
    {
        std::string outer_name;
        DimensionSpecification<T> primary;
        DimensionSpecification<T> secondary;

        PrimitiveSpecification();
        void apply(const Properties& properties);
    };
*/
    //--------------------------------------------------------------------------
    template<typename T>
    struct CylinderSpecification
    {
        std::string outer_name;

        // primary parametric dimension - z, radius, azimuth offset (revs)
        math::Interval<T> axis_z_interval;
        math::Interval<T> axis_radius_interval;
        math::Interval<T> axis_azimuth_offset_interval;
        size_t axis_divisions;
        bool axis_smooth;
        EndTerminationSpecification axis_termination_lower; // Supported: Butt
        EndTerminationSpecification axis_termination_upper; // Supported: Butt

        // secondary parametric dimension - azimuth
        math::Interval<T> azimuth_interval;
        size_t azimuth_divisions;
        bool azimuth_smooth;
        SideTerminationSpecification azimuth_termination; // Supported: Butt, Center

        CylinderSpecification();
        CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, const Properties& properties);
        CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, T revsLower, T revsUpper, const Properties& properties);
        void apply(const Properties& properties);
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct SphereSpecification
    {
        std::string outer_name;
        T radius;

        // primary parametric dimension - elevation
        math::Interval<T> elevation_interval;
        size_t elevation_divisions;
        bool elevation_smooth;
        EndTerminationSpecification elevation_termination_lower; // Supported: Butt, Center
        EndTerminationSpecification elevation_termination_upper; // Supported: Butt, Center

        // secondary parametric dimension - azimuth
        math::Interval<T> azimuth_interval;
        size_t azimuth_divisions;
        bool azimuth_smooth;
        SideTerminationSpecification azimuth_termination; // Supported: Butt, Center

        SphereSpecification();
        SphereSpecification(size_t nAzimuth, size_t nElevation, T radius, const Properties& properties);
        void apply(const Properties& properties);
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct TorusSpecification
    {
        std::string outer_name;

        // primary parametric dimension - major azimuth, major radius, minor radius
        math::Interval<T> major_azimuth_interval;
        math::Interval<T> major_radius_interval; // major spiral (weird) ...
        math::Interval<T> minor_radius_interval; // not spiral like above, but horn shape, depends on primary ...
        // major_z_interval (helix) ...
        // minor_azimuth_offset_interval (twist) ...
        size_t major_divisions;
        bool major_smooth;
        EndTerminationSpecification major_termination_lower; // Supported: Butt
        EndTerminationSpecification major_termination_upper; // Supported: Butt

        // secondary parametric dimension - minor azimuth
        math::Interval<T> minor_azimuth_interval;
        // minor_radius_interval // minor spiral, depends on secondary ...
        size_t minor_divisions;
        bool minor_smooth;
        SideTerminationSpecification minor_termination; // Supported: Butt, center

        TorusSpecification();
        TorusSpecification(size_t nMajor, size_t nMinor, T rMajorLower, T rMajorUpper, T rMinorLower, T rMinorUpper, const Properties& properties);
        void apply(const Properties& properties);
    };

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::CylinderSpecification<T>::CylinderSpecification() :
    outer_name{"body"},
    axis_z_interval{},
    axis_radius_interval{},
    axis_azimuth_offset_interval{T(0), T(0)},
    axis_divisions{0},
    axis_smooth{true},
    axis_termination_lower{TerminationStyle::Butt, false, "end0"},
    axis_termination_upper{TerminationStyle::Butt, false, "end1"},
    azimuth_interval{T(0), T(1)},
    azimuth_divisions{0},
    azimuth_smooth{true},
    azimuth_termination{TerminationStyle::Center, true, "side", "side0", "side1"}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::CylinderSpecification<T>::CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, const Properties& properties) :
    CylinderSpecification()
{
    axis_z_interval.set_bounds(zLower, zUpper);
    axis_radius_interval.set_bounds(rLower, rUpper);
    axis_divisions = nz;
    azimuth_divisions = nAzimuth;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::CylinderSpecification<T>::CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, T revsLower, T revsUpper, const Properties& properties) :
    CylinderSpecification()
{
    axis_z_interval.set_bounds(zLower, zUpper);
    axis_radius_interval.set_bounds(rLower, rUpper);
    axis_azimuth_offset_interval.set_bounds(revsLower, revsUpper);
    axis_divisions = nz;
    azimuth_divisions = nAzimuth;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::CylinderSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "outer_name") { outer_name = value; }
        else if (key == "z_lower") { axis_z_interval.set_lower(to_type<T>(value)); }
        else if (key == "z_upper") { axis_z_interval.set_upper(to_type<T>(value)); }
        else if (key == "radius") { auto t = to_type<T>(value); axis_radius_interval.set_bounds(t, t); }
        else if (key == "radius_lower") { axis_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "radius_upper") { axis_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_offset") { auto t = to_type<T>(value); axis_azimuth_offset_interval.set_bounds(t, t); }
        else if (key == "azimuth_offset_lower") { axis_azimuth_offset_interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_offset_upper") { axis_azimuth_offset_interval.set_upper(to_type<T>(value)); }
        else if (key == "axis_divisions") { axis_divisions = std::stoul(value); }
        else if (key == "axis_smooth") { axis_smooth = to_bool(value); }
        else if (key == "axis_lower_style") { axis_termination_lower.style = to_termination_style(value); }
        else if (key == "axis_lower_name") { axis_termination_lower.name = value; }
        else if (key == "axis_upper_style") { axis_termination_upper.style = to_termination_style(value); }
        else if (key == "axis_upper_name") { axis_termination_upper.name = value; }
        else if (key == "azimuth_lower") { azimuth_interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_upper") { azimuth_interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_divisions") { azimuth_divisions = std::stoul(value); }
        else if (key == "azimuth_smooth") { azimuth_smooth = to_bool(value); }
        else if (key == "azimuth_style") { azimuth_termination.style = to_termination_style(value); }
        else if (key == "azimuth_banded") { azimuth_termination.subdivision = to_bool(value); }
        else if (key == "azimuth_name") { azimuth_termination.name = value; }
        else if (key == "azimuth_lower_name") { azimuth_termination.lower_name = value; }
        else if (key == "azimuth_upper_name") { azimuth_termination.upper_name = value; }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::SphereSpecification<T>::SphereSpecification() :
    outer_name{"body"},
    radius(),
    elevation_interval{T(0), T(1)},
    elevation_divisions{0},
    elevation_smooth{true},
    elevation_termination_lower{TerminationStyle::Butt, false, "end0"},
    elevation_termination_upper{TerminationStyle::Butt, false, "end1"},
    azimuth_interval{T(0), T(1)},
    azimuth_divisions{0},
    azimuth_smooth{true},
    azimuth_termination{TerminationStyle::Center, true, "side", "side0", "side1"}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::SphereSpecification<T>::SphereSpecification(size_t nAzimuth, size_t nElevation, T radius, const Properties& properties) :
    SphereSpecification()
{
    this->radius = radius;
    elevation_divisions = nElevation;
    azimuth_divisions = nAzimuth;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::SphereSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "outer_name") { outer_name = value; }
        else if (key == "radius") { radius = to_type<T>(value); }
        else if (key == "elevation_lower") { elevation_interval.set_lower(to_type<T>(value)); }
        else if (key == "elevation_upper") { elevation_interval.set_upper(to_type<T>(value)); }
        else if (key == "elevation_divisions") { elevation_divisions = std::stoul(value); }
        else if (key == "elevation_smooth") { elevation_smooth = to_bool(value); }
        else if (key == "elevation_lower_style") { elevation_termination_lower.style = to_termination_style(value); }
        else if (key == "elevation_lower_name") { elevation_termination_lower.name = value; }
        else if (key == "elevation_upper_style") { elevation_termination_upper.style = to_termination_style(value); }
        else if (key == "elevation_upper_name") { elevation_termination_upper.name = value; }
        else if (key == "azimuth_lower") { azimuth_interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_upper") { azimuth_interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_divisions") { azimuth_divisions = std::stoul(value); }
        else if (key == "azimuth_smooth") { azimuth_smooth = to_bool(value); }
        else if (key == "azimuth_style") { azimuth_termination.style = to_termination_style(value); }
        else if (key == "azimuth_banded") { azimuth_termination.subdivision = to_bool(value); }
        else if (key == "azimuth_name") { azimuth_termination.name = value; }
        else if (key == "azimuth_lower_name") { azimuth_termination.lower_name = value; }
        else if (key == "azimuth_upper_name") { azimuth_termination.upper_name = value; }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::TorusSpecification<T>::TorusSpecification() :
    outer_name{"body"},
    major_azimuth_interval{T(0), T(1)},
    major_radius_interval{},
    minor_radius_interval{},
    major_divisions{},
    major_smooth{true},
    major_termination_lower{TerminationStyle::Butt, false, "end0"},
    major_termination_upper{TerminationStyle::Butt, false, "end1"},
    minor_azimuth_interval{T(0), T(1)},
    minor_divisions{0},
    minor_smooth{true},
    minor_termination{TerminationStyle::Center, true, "side", "side0", "side1"}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::TorusSpecification<T>::TorusSpecification(size_t nMajor, size_t nMinor, T rMajorLower, T rMajorUpper, T rMinorLower, T rMinorUpper, const Properties& properties) :
    TorusSpecification()
{
    major_radius_interval.set_bounds(rMajorLower, rMajorUpper);
    minor_radius_interval.set_bounds(rMinorLower, rMinorUpper);
    major_divisions = nMajor;
    minor_divisions = nMinor;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::TorusSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "outer_name") { outer_name = value; }
        else if (key == "major_azimuth_lower") { major_azimuth_interval.set_lower(to_type<T>(value)); }
        else if (key == "major_azimuth_upper") { major_azimuth_interval.set_upper(to_type<T>(value)); }
        else if (key == "major_radius") { auto t = to_type<T>(value); major_radius_interval.set_bounds(t, t); }
        else if (key == "major_radius_lower") { major_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "major_radius_upper") { major_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "minor_radius") { auto t = to_type<T>(value); minor_radius_interval.set_bounds(t, t); }
        else if (key == "minor_radius_lower") { minor_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "minor_radius_upper") { minor_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "major_divisions") { major_divisions = std::stoul(value); }
        else if (key == "major_smooth") { major_smooth = to_bool(value); }
        else if (key == "major_lower_style") { major_termination_lower.style = to_termination_style(value); }
        else if (key == "major_lower_name") { major_termination_lower.name = value; }
        else if (key == "major_upper_style") { major_termination_upper.style = to_termination_style(value); }
        else if (key == "major_upper_name") { major_termination_upper.name = value; }
        else if (key == "minor_azimuth_lower") { minor_azimuth_interval.set_lower(to_type<T>(value)); }
        else if (key == "minor_azimuth_upper") { minor_azimuth_interval.set_upper(to_type<T>(value)); }
        else if (key == "minor_divisions") { minor_divisions = std::stoul(value); }
        else if (key == "minor_smooth") { minor_smooth = to_bool(value); }
        else if (key == "minor_style") { minor_termination.style = to_termination_style(value); }
        else if (key == "minor_banded") { minor_termination.subdivision = to_bool(value); }
        else if (key == "minor_name") { minor_termination.name = value; }
        else if (key == "minor_lower_name") { minor_termination.lower_name = value; }
        else if (key == "minor_upper_name") { minor_termination.upper_name = value; }
    }

    return;
}

#endif // QUETZAL_MODEL_PRIMITIVES_SPECIFICATION_HPP
