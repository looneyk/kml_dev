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
        Center,
        Butt,
        // square, conical, round ...
    };

    //--------------------------------------------------------------------------
    TerminationStyle to_termination_style(const std::string& style_name)
    {
        if (style_name == "center") { return TerminationStyle::Center; }
        if (style_name == "butt") { return TerminationStyle::Butt; }
        return TerminationStyle::None;
    }

    //--------------------------------------------------------------------------
    struct TerminationSpecification
    {
        TerminationStyle style;
        bool subdivision; // With Butt, use common center point?, not currently supported (deprecate and use conical with height 0 if that is desired?) ...
        std::vector<std::string> names; // std::inplace_vector<std::string, 2> names ...

        void init(const std::string& style_name, const std::string& base_name);
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct ClosedDimensionSpecification
    {
        math::Interval<T> interval;
        size_t edge_count;
        bool smooth;
        TerminationSpecification termination;
    };

    //--------------------------------------------------------------------------
    // this could work as a single DimensionSpecification ...
    template<typename T>
    struct OpenDimensionSpecification
    {
        std::vector<math::Interval<T>> intervals; // std::inplace_vector<math::Interval<T>, 3> intervals ...
        size_t edge_count;
        bool smooth;
        std::array<TerminationSpecification, 2> terminations; // std::inplace_vector<TerminationSpecification, 2> terminations ...
    };
/*
    Primary, double-ended (z, elevation, major) dimension specification
    secondary, single-ended (azimuth, minor) dimension specification

    //--------------------------------------------------------------------------
    template<typename T>
    struct DimensionSpecification
    {
        math::Interval<T> interval;
        math::Interval<T> secondary_interval;
        math::Interval<T> tertiary_interval;
        math::Interval<T> intervals[];
        size_t edge_count;
        bool smooth;
        TerminationSpecification lower;
        TerminationSpecification upper;
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct PrimitiveSpecification
    {
        std::string shell_name;
        DimensionSpecification<T> primary;
        DimensionSpecification<T> secondary;

        PrimitiveSpecification();
        void apply(const Properties& properties);
    };
*/
    // support custom properties by adding an optional key/value callback to apply ...

    //--------------------------------------------------------------------------
    template<typename T>
    struct CylinderSpecification
    {
        std::string shell_name;

        // primary parametric dimension - z, radius, azimuth offset (revs) are all dependent on t
        math::Interval<T> axis_z_interval;
        math::Interval<T> axis_radius_interval;
        math::Interval<T> axis_azimuth_offset_interval;
        size_t axis_edge_count;
        bool axis_smooth;
        TerminationSpecification axis_termination_lower; // Supported: Butt
        TerminationSpecification axis_termination_upper; // Supported: Butt

        // secondary parametric dimension - azimuth
        ClosedDimensionSpecification<T> azimuth;

        CylinderSpecification();
        CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, const Properties& properties);
        CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, T revsLower, T revsUpper, const Properties& properties);
        void apply(const Properties& properties);
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct SphereSpecification
    {
        std::string shell_name;
        T radius;

        // primary parametric dimension - elevation
        math::Interval<T> elevation_interval;
        size_t elevation_edge_count;
        bool elevation_smooth;
        TerminationSpecification elevation_termination_lower; // Supported: Butt, Center
        TerminationSpecification elevation_termination_upper; // Supported: Butt, Center

        // secondary parametric dimension - azimuth
        ClosedDimensionSpecification<T> azimuth;

        SphereSpecification();
        SphereSpecification(size_t nAzimuth, size_t nElevation, T radius, const Properties& properties);
        void apply(const Properties& properties);
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct TorusSpecification
    {
        std::string shell_name;

        // for radius intervals, use 1/2 param radius callbacks like sphere instead and remove them here ...

        // primary parametric dimension - major azimuth, major radius, minor radius
        math::Interval<T> major_azimuth_interval;
        math::Interval<T> major_radius_interval; // major spiral (weird) ...
        math::Interval<T> minor_radius_interval; // not spiral like above, but horn shape, depends on primary ...
        size_t major_edge_count;
        bool major_smooth;
        TerminationSpecification major_termination_lower; // Supported: Butt
        TerminationSpecification major_termination_upper; // Supported: Butt

        // secondary parametric dimension - minor azimuth
        // minor_radius_interval // minor spiral, depends on secondary ...
        ClosedDimensionSpecification<T> minor; // minor_azimuth? ...

        TorusSpecification();
        TorusSpecification(size_t nMajor, size_t nMinor, T rMajorLower, T rMajorUpper, T rMinorLower, T rMinorUpper, const Properties& properties);
        void apply(const Properties& properties);
    };

} // namespace quetzal::model

//------------------------------------------------------------------------------
void quetzal::model::TerminationSpecification::init(const std::string& style_name, const std::string& base_name)
{
    style = to_termination_style(style_name);

    switch (style)
    {
        case TerminationStyle::None:
            names.resize(0);
            break;

        case TerminationStyle::Center:
            names.resize(2);
            names = {base_name + "0", base_name + "1"};
            break;

        case TerminationStyle::Butt:
            names.resize(1);
            names = {base_name};
            break;

        default:
            assert(false);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::CylinderSpecification<T>::CylinderSpecification() :
    shell_name{"body"},
    axis_z_interval{},
    axis_radius_interval{},
    axis_azimuth_offset_interval{T(0), T(0)},
    axis_edge_count{0},
    axis_smooth{true},
    axis_termination_lower{TerminationStyle::Butt, false, {"end0"}},
    axis_termination_upper{TerminationStyle::Butt, false, {"end1"}},
    azimuth({T(0), T(1)}, 0, true, {TerminationStyle::Center, true, {"side0", "side1"}})
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::CylinderSpecification<T>::CylinderSpecification(size_t nAzimuth, size_t nz, T rLower, T rUpper, T zLower, T zUpper, const Properties& properties) :
    CylinderSpecification()
{
    axis_z_interval.set_bounds(zLower, zUpper);
    axis_radius_interval.set_bounds(rLower, rUpper);
    axis_edge_count = nz;
    azimuth.edge_count = nAzimuth;
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
    axis_edge_count = nz;
    azimuth.edge_count = nAzimuth;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::CylinderSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "shell_name") { shell_name = value; }
        else if (key == "z_lower") { axis_z_interval.set_lower(to_type<T>(value)); }
        else if (key == "z_upper") { axis_z_interval.set_upper(to_type<T>(value)); }
        else if (key == "radius") { auto t = to_type<T>(value); axis_radius_interval.set_bounds(t, t); }
        else if (key == "radius_lower") { axis_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "radius_upper") { axis_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_offset") { auto t = to_type<T>(value); axis_azimuth_offset_interval.set_bounds(t, t); }
        else if (key == "azimuth_offset_lower") { axis_azimuth_offset_interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_offset_upper") { axis_azimuth_offset_interval.set_upper(to_type<T>(value)); }
        else if (key == "axis_edge_count") { axis_edge_count = std::stoul(value); }
        else if (key == "axis_smooth") { axis_smooth = to_bool(value); }
        else if (key == "axis_lower_style") { axis_termination_lower.init(value, "end"); }
        else if (key == "axis_lower_name") { axis_termination_lower.names[0] = value; }
        else if (key == "axis_upper_style") { axis_termination_upper.init(value, "end"); }
        else if (key == "axis_upper_name") { axis_termination_upper.names[1] = value; }
        else if (key == "azimuth_lower") { azimuth.interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_upper") { azimuth.interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_edge_count") { azimuth.edge_count = std::stoul(value); }
        else if (key == "azimuth_smooth") { azimuth.smooth = to_bool(value); }
        else if (key == "azimuth_style") { azimuth.termination.init(value, "side"); }
        else if (key == "azimuth_banded") { azimuth.termination.subdivision = to_bool(value); }
        else if (key == "azimuth_name") { azimuth.termination.names[0] = value; }
        else if (key == "azimuth_lower_name") { azimuth.termination.names[0] = value; }
        else if (key == "azimuth_upper_name") { azimuth.termination.names[1] = value; }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::SphereSpecification<T>::SphereSpecification() :
    shell_name{"body"},
    radius(),
    elevation_interval{T(0), T(1)},
    elevation_edge_count{0},
    elevation_smooth{true},
    elevation_termination_lower{TerminationStyle::Butt, false, {"end0"}},
    elevation_termination_upper{TerminationStyle::Butt, false, {"end1"}},
    azimuth({T(0), T(1)}, 0, true, {TerminationStyle::Center, true, {"side0", "side1"}})
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::SphereSpecification<T>::SphereSpecification(size_t nAzimuth, size_t nElevation, T radius, const Properties& properties) :
    SphereSpecification()
{
    this->radius = radius;
    elevation_edge_count = nElevation;
    azimuth.edge_count = nAzimuth;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::SphereSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "shell_name") { shell_name = value; }
        else if (key == "radius") { radius = to_type<T>(value); }
        else if (key == "elevation_lower") { elevation_interval.set_lower(to_type<T>(value)); }
        else if (key == "elevation_upper") { elevation_interval.set_upper(to_type<T>(value)); }
        else if (key == "elevation_edge_count") { elevation_edge_count = std::stoul(value); }
        else if (key == "elevation_smooth") { elevation_smooth = to_bool(value); }
        else if (key == "elevation_lower_style") { elevation_termination_lower.init(value, "end"); }
        else if (key == "elevation_lower_name") { elevation_termination_lower.names[0] = value; }
        else if (key == "elevation_upper_style") { elevation_termination_upper.init(value, "end"); }
        else if (key == "elevation_upper_name") { elevation_termination_upper.names[1] = value; }
        else if (key == "azimuth_lower") { azimuth.interval.set_lower(to_type<T>(value)); }
        else if (key == "azimuth_upper") { azimuth.interval.set_upper(to_type<T>(value)); }
        else if (key == "azimuth_edge_count") { azimuth.edge_count = std::stoul(value); }
        else if (key == "azimuth_smooth") { azimuth.smooth = to_bool(value); }
        else if (key == "azimuth_style") { azimuth.termination.init(value, "side"); }
        else if (key == "azimuth_banded") { azimuth.termination.subdivision = to_bool(value); }
        else if (key == "azimuth_name") { azimuth.termination.names[0] = value; }
        else if (key == "azimuth_lower_name") { azimuth.termination.names[0] = value; }
        else if (key == "azimuth_upper_name") { azimuth.termination.names[1] = value; }
        else if (key == "smooth") { azimuth.smooth = to_bool(value); } // azimuth_smooth used for full body smooth ...
        else if (key == "faceted") { azimuth.smooth = !to_bool(value); } // azimuth_smooth used for full body smooth ...
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::TorusSpecification<T>::TorusSpecification() :
    shell_name{"body"},
    major_azimuth_interval{T(0), T(1)},
    major_radius_interval{},
    minor_radius_interval{},
    major_edge_count{},
    major_smooth{true},
    major_termination_lower{TerminationStyle::Butt, false, {"end0"}},
    major_termination_upper{TerminationStyle::Butt, false, {"end1"}},
    minor({T(0), T(1)}, 0, true, {TerminationStyle::Center, true, {"side0", "side1"}})
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::model::TorusSpecification<T>::TorusSpecification(size_t nMajor, size_t nMinor, T rMajorLower, T rMajorUpper, T rMinorLower, T rMinorUpper, const Properties& properties) :
    TorusSpecification()
{
    major_radius_interval.set_bounds(rMajorLower, rMajorUpper);
    minor_radius_interval.set_bounds(rMinorLower, rMinorUpper);
    major_edge_count = nMajor;
    minor.edge_count = nMinor;
    apply(properties);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::model::TorusSpecification<T>::apply(const Properties& properties)
{
    for (const auto& [key, value] : properties)
    {
        if (key == "shell_name") { shell_name = value; }
        else if (key == "major_azimuth_lower") { major_azimuth_interval.set_lower(to_type<T>(value)); }
        else if (key == "major_azimuth_upper") { major_azimuth_interval.set_upper(to_type<T>(value)); }
        else if (key == "major_radius") { auto t = to_type<T>(value); major_radius_interval.set_bounds(t, t); }
        else if (key == "major_radius_lower") { major_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "major_radius_upper") { major_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "minor_radius") { auto t = to_type<T>(value); minor_radius_interval.set_bounds(t, t); }
        else if (key == "minor_radius_lower") { minor_radius_interval.set_lower(to_type<T>(value)); }
        else if (key == "minor_radius_upper") { minor_radius_interval.set_upper(to_type<T>(value)); }
        else if (key == "major_edge_count") { major_edge_count = std::stoul(value); }
        else if (key == "major_smooth") { major_smooth = to_bool(value); }
        else if (key == "major_lower_style") { major_termination_lower.init(value, "end"); }
        else if (key == "major_lower_name") { major_termination_lower.names[0] = value; }
        else if (key == "major_upper_style") { major_termination_upper.init(value, "end"); }
        else if (key == "major_upper_name") { major_termination_upper.names[1] = value; }
        else if (key == "minor_azimuth_lower") { minor.interval.set_lower(to_type<T>(value)); }
        else if (key == "minor_azimuth_upper") { minor.interval.set_upper(to_type<T>(value)); }
        else if (key == "minor_edge_count") { minor.edge_count = std::stoul(value); }
        else if (key == "minor_smooth") { minor.smooth = to_bool(value); }
        else if (key == "minor_style") { minor.termination.init(value, "side"); }
        else if (key == "minor_banded") { minor.termination.subdivision = to_bool(value); }
        else if (key == "minor_name") { minor.termination.names[0] = value; }
        else if (key == "minor_lower_name") { minor.termination.names[0] = value; }
        else if (key == "minor_upper_name") { minor.termination.names[1] = value; }
    }

    return;
}

#endif // QUETZAL_MODEL_PRIMITIVES_SPECIFICATION_HPP
