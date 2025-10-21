#if !defined(QUETZAL_GEOMETRY_CURVENURBS_HPP)
#define QUETZAL_GEOMETRY_CURVENURBS_HPP
//------------------------------------------------------------------------------
// geometry
// CurveNURBS.hpp
// NURBS Curve
//------------------------------------------------------------------------------

#include "Curve.hpp"
//#include "quetzal/math/binomial.hpp"
#include <span>
#include <vector>

#include <iostream>

namespace quetzal::geometry
{

    // knot generation, uniform, chord
    // open, closed (terminology?), bezier

    //--------------------------------------------------------------------------
    template<typename P, typename T = P::value_type>
    class CurveNURBS : public Curve<P, T>
    {
    public:

        using value_type = T;
        using typename Curve<P, T>::parameter_type;
        using typename Curve<P, T>::point_type;
        using typename Curve<P, T>::vector_type;
        using points_type = std::vector<point_type>;
        using knot_type = parameter_type;
        using knots_type = std::vector<knot_type>;

        CurveNURBS(size_t degree, const points_type& points, const knots_type& knots);
        CurveNURBS(size_t degree, const points_type& points);
        CurveNURBS(const CurveNURBS&) = default;
        CurveNURBS(CurveNURBS&&) = default;
        virtual ~CurveNURBS() = default;

        CurveNURBS& operator=(const CurveNURBS&) = default;
        CurveNURBS& operator=(CurveNURBS&&) = default;

        virtual std::array<parameter_type, 2> domain_interval() const override;

        virtual point_type position(parameter_type t) const override;
        virtual vector_type direction(parameter_type t) const override; // derivative, unit vector

        size_t degree() const;
        void set_degree(size_t degree); // any change requires knots to be regenerated

        size_t order() const;

        const points_type& points() const;
        points_type& points(); // any change requires knots to be regenerated
// insert_point(pos, point) and remove_point(pos) so that knots can be recalculated? ...
// or just leave it up to the client to call generate_knots as necessary ...
// or both ...

        size_t point_count() const;
        const point_type& point(size_t i) const;
        point_type& point(size_t i);

        const knots_type& knots() const;
        knots_type& knots(); // any change to knot vector size needs to be accompanied by a change in degree and/or control points

        size_t knot_count() const;
        const knot_type& knot(size_t i) const;
        knot_type& knot(size_t i);

        void generate_knots();

    private:

        size_t find_span(parameter_type t) const;
        std::vector<T> basis_functions(parameter_type t, size_t i) const;

        size_t m_degree;
        points_type m_points;
        knots_type m_knots;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename P, typename T>
quetzal::geometry::CurveNURBS<P, T>::CurveNURBS(size_t degree, const points_type& points, const knots_type& knots) :
    Curve<P, T>(),
    m_degree(degree),
    m_points(points),
    m_knots(knots)
{
    assert(m_degree == knots.size() - points.size() - 1);
}

//------------------------------------------------------------------------------
template<typename P, typename T>
quetzal::geometry::CurveNURBS<P, T>::CurveNURBS(size_t degree, const points_type& points) :
    Curve<P, T>(),
    m_degree(degree),
    m_points(points),
    m_knots()
{
    generate_knots();
}

//------------------------------------------------------------------------------
template<typename P, typename T>
std::array<typename quetzal::geometry::CurveNURBS<P, T>::parameter_type, 2> quetzal::geometry::CurveNURBS<P, T>::domain_interval() const
{
    return {m_knots.front(), m_knots.back()};
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::point_type quetzal::geometry::CurveNURBS<P, T>::position(parameter_type t) const
{
    size_t i = find_span(t);
    auto B = basis_functions(t, i);

    point_type point;
    for (size_t j = 0; j <= m_degree; ++j)
    {
		point += B[j] * m_points[i - m_degree + j].vector();
    }

    return point;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::vector_type quetzal::geometry::CurveNURBS<P, T>::direction(parameter_type t) const
{
    // direction approximation ...
    auto [lower, upper] = domain_interval();
    value_type dt = (upper - lower) / T(1000);
    value_type t0 = t - dt;
    if (t0 < lower) t0 = lower;
    value_type t1 = t + dt;
    if (t1 > upper) t0 = upper;
    return normalize(position(t1) - position(t0));
/*
    vector_type v;

    // ...
    t;

    math::MatrixSimple<T> bc = math::binomial_coefficients<T>(7); // ...
    std::cout << bc;

    auto bc2 = math::binomial_coefficients<T>(4); // ...
    bc2;

    return normalize(v);
*/
}

//------------------------------------------------------------------------------
template<typename P, typename T>
size_t quetzal::geometry::CurveNURBS<P, T>::degree() const
{
    return m_degree;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
void quetzal::geometry::CurveNURBS<P, T>::set_degree(size_t degree)
{
    m_degree = degree;
    generate_knots();
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
size_t quetzal::geometry::CurveNURBS<P, T>::order() const
{
    return m_degree + 1;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
const typename quetzal::geometry::CurveNURBS<P, T>::points_type& quetzal::geometry::CurveNURBS<P, T>::points() const
{
    return m_points;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::points_type& quetzal::geometry::CurveNURBS<P, T>::points()
{
    return m_points;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
size_t quetzal::geometry::CurveNURBS<P, T>::point_count() const
{
    return m_points.size();
}

//------------------------------------------------------------------------------
template<typename P, typename T>
const typename quetzal::geometry::CurveNURBS<P, T>::point_type& quetzal::geometry::CurveNURBS<P, T>::point(size_t i) const
{
    assert(i < m_points.size());
    return m_points[i];
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::point_type& quetzal::geometry::CurveNURBS<P, T>::point(size_t i)
{
    assert(i < m_points.size());
    return m_points[i];
}

//------------------------------------------------------------------------------
template<typename P, typename T>
const typename quetzal::geometry::CurveNURBS<P, T>::knots_type& quetzal::geometry::CurveNURBS<P, T>::knots() const
{
    return m_knots;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::knots_type& quetzal::geometry::CurveNURBS<P, T>::knots()
{
    return m_knots;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
size_t quetzal::geometry::CurveNURBS<P, T>::knot_count() const
{
    return m_knots.size();
}

//------------------------------------------------------------------------------
template<typename P, typename T>
const typename quetzal::geometry::CurveNURBS<P, T>::knot_type& quetzal::geometry::CurveNURBS<P, T>::knot(size_t i) const
{
    assert(i < m_knots.size());
    return m_knots[i];
}

//------------------------------------------------------------------------------
template<typename P, typename T>
typename quetzal::geometry::CurveNURBS<P, T>::knot_type& quetzal::geometry::CurveNURBS<P, T>::knot(size_t i)
{
    assert(i < m_knots.size());
    return m_knots[i];
}

//------------------------------------------------------------------------------
template<typename P, typename T>
void quetzal::geometry::CurveNURBS<P, T>::generate_knots()
{
    m_knots.resize(m_points.size() + m_degree + 1);
    for (size_t i = 0; i < m_knots.size(); ++i)
    {
        if (i <= m_degree)
        {
            m_knots[i] = T(0);
        }
        else if (i >= m_points.size())
        {
            m_knots[i] = T(1);
        }
        else
        {
            m_knots[i] = T(i - m_degree) / (m_points.size() - m_degree);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
size_t quetzal::geometry::CurveNURBS<P, T>::find_span(parameter_type t) const
{

    if (t >= m_knots[m_points.size()])
    {
        return m_points.size() - 1;
    }

    if (t <= m_knots[m_degree])
    {
        return m_degree;
    }

//if (t == 1.0f) return m_knots.size() - 1;

    size_t low = 0;
    size_t high = m_points.size() + 1;
//size_t low = m_degree;
//size_t high = m_knots.size();
    size_t mid = (low + high) / 2;

    while (t < m_knots[mid] || t >= m_knots[mid + 1])
    {
        if (t < m_knots[mid])
            high = mid;
        else
            low = mid;
        mid = (low + high) / 2;
    }

    return mid;
}

//------------------------------------------------------------------------------
template<typename P, typename T>
std::vector<T> quetzal::geometry::CurveNURBS<P, T>::basis_functions(parameter_type t, size_t i) const
{
    size_t order = m_degree + 1;
    std::vector<value_type> sides(2 * order);
    auto left = std::span{sides.begin(), order};
    auto right = std::span{sides.begin() + order, order};

    std::vector<T> N(order);

    N[0] = value_type(1);
    for (size_t j = 1; j < order; ++j)
    {
        left[j] = t - m_knots[i + 1 - j];
        right[j] = m_knots[i + j] - t;
        value_type saved = value_type(0);

        for (size_t r = 0; r < j; ++r)
        {
            value_type temp = N[r] / (right[r + 1] + left[j - r]);
            N[r] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }

        N[j] = saved;
    }

    return N;
}

#endif // QUETZAL_GEOMETRY_CURVENURBS_HPP
