#if !defined(QUETZAL_GEOMETRY_CURVE_HPP)
#define QUETZAL_GEOMETRY_CURVE_HPP
//------------------------------------------------------------------------------
// geometry
// Curve.hpp
// Abstract base class, interface for parametric curve
//------------------------------------------------------------------------------

#include <array>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename P, typename T = P::value_type>
    class Curve
    {
    public:

        using parameter_type = T;
        using point_type = P;
        using vector_type = P::vector_type;

        Curve() = default;
        Curve(const Curve&) = default;
        Curve(Curve&&) = default;
        virtual ~Curve() = default;

        Curve& operator=(const Curve&) = default;
        Curve& operator=(Curve&&) = default;

        point_type operator()(parameter_type t) const;

        virtual std::array<parameter_type, 2> domain_interval() const = 0;

        virtual point_type position(parameter_type t) const = 0;
        virtual vector_type direction(parameter_type t) const = 0; // derivative, unit vector, tangent_direction, ...
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::Curve<P, T>::point_type quetzal::geometry::Curve<P, T>::operator()(parameter_type t) const
{
    return position(t);
}

#endif // QUETZAL_GEOMETRY_CURVE_HPP
