#if !defined(QUETZAL_MATH_DISCRETEVECTOR_HPP)
#define QUETZAL_MATH_DISCRETEVECTOR_HPP
//------------------------------------------------------------------------------
// math
// DiscreteVector.hpp
//------------------------------------------------------------------------------

#include "Vector.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <iosfwd>
#include <istream>
#include <ostream>
#include <type_traits>
#include <cstdlib>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    // A vector over a signed integral type, exact where Vector is approximate. Displacement
    // on a lattice: a step between cells, an extent in cells, an offset from one position to
    // another. Signed because a displacement runs in either direction along every axis
    template<std::signed_integral T, size_t N> requires (N > 0)
    class DiscreteVector
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_type dimension = N;
        using rep_type = std::array<value_type, dimension>;

        DiscreteVector();
        DiscreteVector(const rep_type& components);

        template<typename U> requires (N == 1) && std::convertible_to<U, T>
        DiscreteVector(const U& x);

        template<typename U> requires (N == 2) && std::convertible_to<U, T>
        DiscreteVector(const U& x, const U& y);

        template<typename U> requires (N == 3) && std::convertible_to<U, T>
        DiscreteVector(const U& x, const U& y, const U& z);

        template<typename U> requires (N == 4) && std::convertible_to<U, T>
        DiscreteVector(const U& x, const U& y, const U& z, const U& w);

        DiscreteVector(const DiscreteVector&) = default;
        DiscreteVector(DiscreteVector&&) = default;
        ~DiscreteVector() = default;

        DiscreteVector& operator=(const DiscreteVector&) = default;
        DiscreteVector& operator=(DiscreteVector&&) = default;
        DiscreteVector& operator=(const value_type(&components)[dimension]);

        DiscreteVector operator+() const;
        DiscreteVector operator-() const;
        DiscreteVector& operator+=(const DiscreteVector& other);
        DiscreteVector& operator-=(const DiscreteVector& other);
        DiscreteVector& operator*=(value_type f);
        DiscreteVector& operator/=(value_type f);

        value_type operator[](size_type i) const;
        value_type& operator[](size_type i);

        value_type x() const requires (N >= 1);
        value_type y() const requires (N >= 2);
        value_type z() const requires (N >= 3);
        value_type w() const requires (N >= 4);

        void set_x(value_type x) requires (N >= 1);
        void set_y(value_type y) requires (N >= 2);
        void set_z(value_type z) requires (N >= 3);
        void set_w(value_type w) requires (N >= 4);

        template<typename U> requires (N == 1) && std::convertible_to<U, T>
        void set(const U& x);

        template<typename U> requires (N == 2) && std::convertible_to<U, T>
        void set(const U& x, const U& y);

        template<typename U> requires (N == 3) && std::convertible_to<U, T>
        void set(const U& x, const U& y, const U& z);

        template<typename U> requires (N == 4) && std::convertible_to<U, T>
        void set(const U& x, const U& y, const U& z, const U& w);

        const rep_type& rep() const;

        void clear();

        bool zero() const;

        // The unit step along whichever axis this most nearly points down, ties going to the
        // later axis, and zero when it points nowhere. Quantizing to a lattice is a choice
        // of axis rather than a scaling of each component: reducing (9, 8) componentwise
        // gives a diagonal that was never asked for
        DiscreteVector direction() const;

        template<size_type I> requires (I < dimension)
        value_type& get() &
        {
            return m_components[I];
        }

        template<size_type I> requires (I < dimension)
        const value_type& get() const&
        {
            return m_components[I];
        }

        template<size_type I> requires (I < dimension)
        value_type&& get() &&
        {
            return std::move(m_components[I]);
        }

    private:

        rep_type m_components;
    };

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator+(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator-(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs);

    // The scalar is not deduced, so that a literal of another integral type still scales a
    // vector rather than failing to agree with it
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(std::type_identity_t<T> lhs, DiscreteVector<T, N> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator/(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    bool operator==(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    bool operator!=(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscreteVector<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, const DiscreteVector<T, N>& v);

    //--------------------------------------------------------------------------
    // The unit step a vector most nearly points down, zero when it points nowhere
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> direction(const DiscreteVector<T, N>& v);

    // The same of a continuous direction. The result type is named rather than deduced,
    // there being nothing in the argument to say what lattice it is being quantized onto.
    // The two dimensions must agree: a caller quantizing a direction through space onto a
    // flat grid reduces it to that plane first, rather than having a component dropped here
    template<typename V, typename Traits> requires (Traits::dimension == V::dimension)
    V direction(const Vector<Traits>& v);

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>::DiscreteVector() :
        m_components{}
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>::DiscreteVector(const rep_type& components) :
        m_components(components)
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 1) && std::convertible_to<U, T>
    DiscreteVector<T, N>::DiscreteVector(const U& x) :
        m_components{static_cast<T>(x)}
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 2) && std::convertible_to<U, T>
    DiscreteVector<T, N>::DiscreteVector(const U& x, const U& y) :
        m_components{static_cast<T>(x), static_cast<T>(y)}
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 3) && std::convertible_to<U, T>
    DiscreteVector<T, N>::DiscreteVector(const U& x, const U& y, const U& z) :
        m_components{static_cast<T>(x), static_cast<T>(y), static_cast<T>(z)}
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 4) && std::convertible_to<U, T>
    DiscreteVector<T, N>::DiscreteVector(const U& x, const U& y, const U& z, const U& w) :
        m_components{static_cast<T>(x), static_cast<T>(y), static_cast<T>(z), static_cast<T>(w)}
    {
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>& DiscreteVector<T, N>::operator=(const value_type(&components)[dimension])
    {
        for (size_type i = 0; i < dimension; ++i)
        {
            m_components[i] = components[i];
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> DiscreteVector<T, N>::operator+() const
    {
        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> DiscreteVector<T, N>::operator-() const
    {
        DiscreteVector v;

        for (size_type i = 0; i < dimension; ++i)
        {
            v[i] = -m_components[i];
        }

        return v;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>& DiscreteVector<T, N>::operator+=(const DiscreteVector& other)
    {
        for (size_type i = 0; i < dimension; ++i)
        {
            m_components[i] += other.m_components[i];
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>& DiscreteVector<T, N>::operator-=(const DiscreteVector& other)
    {
        for (size_type i = 0; i < dimension; ++i)
        {
            m_components[i] -= other.m_components[i];
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>& DiscreteVector<T, N>::operator*=(value_type f)
    {
        for (size_type i = 0; i < dimension; ++i)
        {
            m_components[i] *= f;
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N>& DiscreteVector<T, N>::operator/=(value_type f)
    {
        for (size_type i = 0; i < dimension; ++i)
        {
            m_components[i] /= f;
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type DiscreteVector<T, N>::operator[](size_type i) const
    {
        assert(i < dimension);
        return m_components[i];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type& DiscreteVector<T, N>::operator[](size_type i)
    {
        assert(i < dimension);
        return m_components[i];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type DiscreteVector<T, N>::x() const requires (N >= 1)
    {
        return m_components[0];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type DiscreteVector<T, N>::y() const requires (N >= 2)
    {
        return m_components[1];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type DiscreteVector<T, N>::z() const requires (N >= 3)
    {
        return m_components[2];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscreteVector<T, N>::value_type DiscreteVector<T, N>::w() const requires (N >= 4)
    {
        return m_components[3];
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    void DiscreteVector<T, N>::set_x(value_type x) requires (N >= 1)
    {
        m_components[0] = x;
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    void DiscreteVector<T, N>::set_y(value_type y) requires (N >= 2)
    {
        m_components[1] = y;
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    void DiscreteVector<T, N>::set_z(value_type z) requires (N >= 3)
    {
        m_components[2] = z;
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    void DiscreteVector<T, N>::set_w(value_type w) requires (N >= 4)
    {
        m_components[3] = w;
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 1) && std::convertible_to<U, T>
    void DiscreteVector<T, N>::set(const U& x)
    {
        m_components[0] = static_cast<T>(x);
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 2) && std::convertible_to<U, T>
    void DiscreteVector<T, N>::set(const U& x, const U& y)
    {
        m_components[0] = static_cast<T>(x);
        m_components[1] = static_cast<T>(y);
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 3) && std::convertible_to<U, T>
    void DiscreteVector<T, N>::set(const U& x, const U& y, const U& z)
    {
        m_components[0] = static_cast<T>(x);
        m_components[1] = static_cast<T>(y);
        m_components[2] = static_cast<T>(z);
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    template<typename U> requires (N == 4) && std::convertible_to<U, T>
    void DiscreteVector<T, N>::set(const U& x, const U& y, const U& z, const U& w)
    {
        m_components[0] = static_cast<T>(x);
        m_components[1] = static_cast<T>(y);
        m_components[2] = static_cast<T>(z);
        m_components[3] = static_cast<T>(w);
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    const typename DiscreteVector<T, N>::rep_type& DiscreteVector<T, N>::rep() const
    {
        return m_components;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    void DiscreteVector<T, N>::clear()
    {
        m_components.fill(value_type(0));
        return;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    bool DiscreteVector<T, N>::zero() const
    {
        return std::all_of(m_components.begin(), m_components.end(), [](value_type v) { return v == value_type(0); });
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> DiscreteVector<T, N>::direction() const
    {
        size_type axis = 0;
        value_type largest = value_type(0);

        // Ties go to the later axis, so the comparison is not strict
        for (size_type i = 0; i < dimension; ++i)
        {
            value_type magnitude = std::abs(m_components[i]);
            if (magnitude > value_type(0) && magnitude >= largest)
            {
                largest = magnitude;
                axis = i;
            }
        }

        if (largest == value_type(0))
        {
            return {};
        }

        DiscreteVector result;
        result[axis] = m_components[axis] > value_type(0) ? value_type(1) : value_type(-1);

        return result;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator+(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs)
    {
        return lhs += rhs;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator-(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs)
    {
        return lhs -= rhs;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs)
    {
        return lhs *= rhs;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(std::type_identity_t<T> lhs, DiscreteVector<T, N> rhs)
    {
        return rhs *= lhs;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator/(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs)
    {
        return lhs /= rhs;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    bool operator==(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs)
    {
        return lhs.rep() == rhs.rep();
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    bool operator!=(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs)
    {
        return !(lhs == rhs);
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscreteVector<T, N>& v)
    {
        for (size_t i = 0; i < N; ++i)
        {
            is >> v[i];
        }

        return is;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, const DiscreteVector<T, N>& v)
    {
        for (size_t i = 0; i < N; ++i)
        {
            os << (i > 0 ? " " : "") << v[i];
        }

        return os;
    }

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> direction(const DiscreteVector<T, N>& v)
    {
        return v.direction();
    }

    //--------------------------------------------------------------------------
    template<typename V, typename Traits> requires (Traits::dimension == V::dimension)
    V direction(const Vector<Traits>& v)
    {
        using component_type = typename Traits::value_type;
        using value_type = typename V::value_type;

        size_t axis = 0;
        component_type largest = component_type(0);

        // Ties go to the later axis, so the comparison is not strict
        for (size_t i = 0; i < V::dimension; ++i)
        {
            component_type magnitude = std::abs(v[i]);
            if (!float_eq0(magnitude) && magnitude >= largest)
            {
                largest = magnitude;
                axis = i;
            }
        }

        if (float_eq0(largest))
        {
            return {};
        }

        V result;
        result[axis] = v[axis] > component_type(0) ? value_type(1) : value_type(-1);

        return result;
    }

} // namespace quetzal::math

namespace std
{
    template<typename T, size_t N>
    struct tuple_size<quetzal::math::DiscreteVector<T, N>> : std::integral_constant<size_t, N> {};

    template<size_t I, typename T, size_t N>
    struct tuple_element<I, quetzal::math::DiscreteVector<T, N>> { using type = T; };
}

#endif // QUETZAL_MATH_DISCRETEVECTOR_HPP
