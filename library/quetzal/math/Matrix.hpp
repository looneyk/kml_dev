#if !defined(QUETZAL_MATH_MATRIX_HPP)
#define QUETZAL_MATH_MATRIX_HPP
//------------------------------------------------------------------------------
// math
// Matrix.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include "math_util.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>
#include <span>
#include <cmath>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename T, size_t M = 4, size_t N = M>
    class Matrix
    {
    public:

        static_assert(M > 0 && N > 0);

        static constexpr size_t rows = M;
        static constexpr size_t cols = N;
        static constexpr bool square = (M == N);

        using value_type = T;
        using submatrix_type = Matrix<T, M - 1, N - 1>;

        constexpr Matrix();
        constexpr explicit Matrix(size_t I) requires Matrix<T, M, N>::square; // I == 1 for identity matrix (would be good if 0 was supported as well for symmetry...)
        Matrix(std::span<const value_type> elements);
        Matrix(std::initializer_list<value_type> elements); // waiting for the fix ...

        Matrix(const Matrix&) = default;
        Matrix(Matrix&&) = default;
        ~Matrix() = default;

        Matrix& operator=(const Matrix&) = default;
        Matrix& operator=(Matrix&&) = default;

        Matrix& operator+=(const Matrix& other);
        Matrix& operator-=(const Matrix& other);
        Matrix& operator*=(value_type factor);
        Matrix& operator/=(value_type divisor);

        Matrix operator+() const;
        Matrix operator-() const;

        const value_type& operator()(size_t m, size_t n) const;
        value_type& operator()(size_t m, size_t n);

        void clear();

        const value_type* data() const;
        value_type* data();

        constexpr size_t size() const noexcept;

        submatrix_type submatrix(size_t m, size_t n) const;

        // Operations on square matrices
        // problem with square ...
        // https://stackoverflow.com/questions/75035621/why-cant-a-member-function-require-a-static-constexpr-member-of-the-same-class

        Matrix& operator*=(const Matrix& other) requires Matrix<T, M, N>::square;

        bool identity() const;

        Matrix& transpose() requires Matrix<T, M, N>::square;
        Matrix& inverse() requires Matrix<T, M, N>::square;

        value_type determinant() const requires Matrix<T, M, N>::square;
        value_type trace() const requires Matrix<T, M, N>::square;

        value_type minor(size_t m, size_t n) const requires Matrix<T, M, N>::square;
        value_type cofactor(size_t m, size_t n) const requires Matrix<T, M, N>::square;

        constexpr static Matrix<T, M, N> Identity() requires Matrix<T, M, N>::square;

        // Elementary row operations
        void swap_rows(size_t i, size_t j);
        void scale_row(T factor, size_t i);
        void add_row_multiple(T factor, size_t from, size_t to);

    private:

        inline size_t offset(size_t i, size_t j) const
        {
            return i * cols + j;
        }

        using rep_type = std::array<value_type, M * N>;
        alignas(16) rep_type m_rep;
    };

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> operator+(Matrix<T, M, N> lhs, const Matrix<T, M, N>& rhs);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> operator-(Matrix<T, M, N> lhs, const Matrix<T, M, N>& rhs);

    template<typename T, size_t M, size_t N, size_t P>
    Matrix<T, M, P> operator*(const Matrix<T, M, N>& lhs, const Matrix<T, N, P>& rhs);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> operator*(Matrix<T, M, N> lhs, const T& rhs);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> operator*(const T& lhs, Matrix<T, M, N> rhs);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> operator/(Matrix<T, M, N> lhs, const T& rhs);

    template<typename T, size_t M, size_t N>
    bool operator==(const Matrix<T, M, N>& lhs, const Matrix<T, M, N>& rhs);

    template<typename T, size_t M, size_t N>
    Matrix<T, N, M> transpose(const Matrix<T, M, N>& matrix);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> inverse(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square);

    template<typename T, size_t M, size_t N>
    Matrix<T, N, M> pseudo_inverse(const Matrix<T, M, N>& matrix);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> adjugate(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square);

    template<typename T, size_t M, size_t N>
    Matrix<T, M, N> comatrix(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square);

    template<typename T, size_t M, size_t N>
    std::tuple<quetzal::math::Matrix<T, M, N>, T> gaussian_elimination(Matrix<T, M, N> matrix);

    template<typename T, size_t M, size_t N>
    std::tuple<quetzal::math::Matrix<T, M, N>, bool> gauss_jordan_elimination(Matrix<T, M, N> matrix);

    template<typename T, size_t M, size_t N>
    bool float_eq(const Matrix<T, M, N>& lhs, const Matrix<T, M, N>& rhs, int ulp = 128);

    template<typename T, size_t M, size_t N>
    std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& matrix);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
constexpr quetzal::math::Matrix<T, M, N>::Matrix() :
    m_rep{}
{
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
constexpr quetzal::math::Matrix<T, M, N>::Matrix(size_t I) requires Matrix<T, M, N>::square :
    m_rep{}
{
    assert(I == 1);

    size_t k = 0;
    constexpr size_t stride = cols + 1;
    for (size_t i = 0; i < M; ++i)
    {
        m_rep[k] = T(1);
        k += stride;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>::Matrix(std::span<const value_type> elements) :
    m_rep{}
{
    assert(elements.size() == m_rep.size());

    std::ranges::copy(elements, m_rep.data());
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>::Matrix(std::initializer_list<value_type> elements) :
    m_rep{}
{
    assert(elements.size() == m_rep.size());

    std::ranges::copy(elements, m_rep.data());
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::operator+=(const Matrix& other)
{
    std::ranges::transform(m_rep, other.m_rep, m_rep.begin(), [](auto t, auto u) { return t + u; });
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::operator-=(const Matrix& other)
{
    std::ranges::transform(m_rep, other.m_rep, m_rep.begin(), [](auto t, auto u) { return t - u; });
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::operator*=(value_type factor)
{
    std::ranges::transform(m_rep, m_rep.begin(), [factor](auto t) { return t * factor; });
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::operator/=(value_type divisor)
{
    return *this *= (T(1) / divisor);
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::Matrix<T, M, N>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::Matrix<T, M, N>::operator-() const
{
    Matrix<T, M, N> result;
    std::ranges::transform(m_rep, result.m_rep.begin(), [](auto t) { return -t; });
    return result;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
const typename quetzal::math::Matrix<T, M, N>::value_type& quetzal::math::Matrix<T, M, N>::operator()(size_t m, size_t n) const
{
    return m_rep[offset(m, n)];
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type& quetzal::math::Matrix<T, M, N>::operator()(size_t m, size_t n)
{
    return m_rep[offset(m, n)];
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
void quetzal::math::Matrix<T, M, N>::clear()
{
    m_rep.fill(T(0));
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
const typename quetzal::math::Matrix<T, M, N>::value_type* quetzal::math::Matrix<T, M, N>::data() const
{
    return m_rep.data();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type* quetzal::math::Matrix<T, M, N>::data()
{
    return m_rep.data();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
constexpr size_t quetzal::math::Matrix<T, M, N>::size() const noexcept
{
    return m_rep.size();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>::submatrix_type quetzal::math::Matrix<T, M, N>::submatrix(size_t m, size_t n) const
{
    static_assert(rows > 1 && cols > 1);
    assert(m < rows);
    assert(n < cols);

    submatrix_type matrix;

    size_t k = 0;
    size_t kSub = 0;
    for (size_t i = 0; i < rows; ++i)
    {
        if (i == m)
        {
            k += N;
            continue;
        }

        for (size_t j = 0; j < cols; ++j)
        {
            if (j == n)
            {
                ++k;
                continue;
            }

            matrix.data()[kSub++] = m_rep[k++];
        }
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::operator*=(const Matrix<T, M, N>& other) requires Matrix<T, M, N>::square
{
    *this = *this * other;
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
bool quetzal::math::Matrix<T, M, N>::identity() const
{
    if (!Matrix<T, M, N>::square)
    {
        return false;
    }

    size_t k = 0;
    constexpr size_t stride = rows + 1;
    for (size_t i = 0; i < m_rep.size(); ++i)
    {
        if (i == k)
        {
            if (m_rep[i] != T(1))
            {
                return false;
            }

            k += stride;
        }
        else
        {
            if (m_rep[i] != T(0))
            {
                return false;
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::transpose() requires Matrix<T, M, N>::square
{
    *this = quetzal::math::transpose(*this);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N>& quetzal::math::Matrix<T, M, N>::inverse() requires Matrix<T, M, N>::square
{
    *this = quetzal::math::inverse(*this);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type quetzal::math::Matrix<T, M, N>::determinant() const requires Matrix<T, M, N>::square
{
    if constexpr (M == 1)
    {
        return m_rep[0];
    }
    else if constexpr (M == 2)
    {
        return m_rep[0] * m_rep[3] - m_rep[1] * m_rep[2];
    }
    else if constexpr (M == 3)
    {
        return m_rep[0] * (m_rep[4] * m_rep[8] - m_rep[5] * m_rep[7])
            + m_rep[1] * (m_rep[5] * m_rep[6] - m_rep[3] * m_rep[8])
            + m_rep[2] * (m_rep[3] * m_rep[7] - m_rep[4] * m_rep[6]);
    }
    else if constexpr (M == 4)
    {
        return m_rep[0] * minor(0, 0) - m_rep[1] * minor(0, 1) + m_rep[2] * minor(0, 2) - m_rep[3] * minor(0, 3);
    }
    else
    {
        value_type sign = 1;
        value_type d = 0;

        for (size_t j = 0; j < N; ++j)
        {
            d += sign * m_rep[j] * minor(0, j);
            sign = -sign;
        }

        return d;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type quetzal::math::Matrix<T, M, N>::trace() const requires Matrix<T, M, N>::square
{
    value_type t = 0;

    size_t k = 0;
    constexpr size_t stride = cols + 1;
    for (size_t i = 0; i < M; ++i)
    {
        t += m_rep[k];
        k += stride;
    }

    return t;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type quetzal::math::Matrix<T, M, N>::minor(size_t m, size_t n) const requires Matrix<T, M, N>::square
{
    assert(M > 1);
    assert(m < M);
    assert(n < N);

    return submatrix(m, n).determinant();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
typename quetzal::math::Matrix<T, M, N>::value_type quetzal::math::Matrix<T, M, N>::cofactor(size_t m, size_t n) const requires Matrix<T, M, N>::square
{
    return (even(m + n) ? 1 : -1) * minor(m, n);
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
constexpr quetzal::math::Matrix<T, M, N> quetzal::math::Matrix<T, M, N>::Identity() requires Matrix<T, M, N>::square
{
    return Matrix<T, M, N>(1);
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
void quetzal::math::Matrix<T, M, N>::swap_rows(size_t i, size_t j)
{
    std::span a(&m_rep[i * cols], cols);
    std::span b(&m_rep[j * cols], cols);

    for (auto k : std::ranges::views::zip(a, b))
    {
        auto& [ea, eb] = k;
        std::swap(ea, eb);
    }

    //swap(a, b);
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
void quetzal::math::Matrix<T, M, N>::scale_row(T factor, size_t i)
{
    std::span a(&m_rep[i * cols], cols);
    std::ranges::for_each(a, [&](T& t) { t *= factor; } );
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
void quetzal::math::Matrix<T, M, N>::add_row_multiple(T factor, size_t from, size_t to)
{
    factor;
    auto a = std::span(&m_rep[from * cols], cols);
    auto b = std::span(&m_rep[to * cols], cols);
    std::ranges::transform(a, b, b.begin(), [&](T& s, T& t) { return t + factor * s; } );
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::operator+(Matrix<T, M, N> lhs, const Matrix<T, M, N>& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::operator-(Matrix<T, M, N> lhs, const Matrix<T, M, N>& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N, size_t P>
quetzal::math::Matrix<T, M, P> quetzal::math::operator*(const Matrix<T, M, N>& lhs, const Matrix<T, N, P>& rhs)
{
    Matrix<T, M, P> matrix;
    auto pdest = matrix.data();
    auto plhs = lhs.data();

    for (size_t i = 0; i < M; ++i)
    {
        auto prhs = rhs.data();

        for (size_t j = 0; j < N; ++j)
        {
            for (size_t k = 0; k < P; ++k)
            {
                pdest[k] += plhs[j] * prhs[k];
            }

            prhs += P;
        }

        plhs += N;
        pdest += P;
    }

    return matrix;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::operator*(Matrix<T, M, N> lhs, const T& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::operator*(const T& lhs, Matrix<T, M, N> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::operator/(Matrix<T, M, N> lhs, const T& rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
bool quetzal::math::operator==(const Matrix<T, M, N>& lhs, const Matrix<T, M, N>& rhs)
{
    return std::equal(lhs.data(), lhs.data() + lhs.size(), rhs.data());
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, N, M> quetzal::math::transpose(const Matrix<T, M, N>& matrix)
{
    Matrix<T, N, M> m;
    size_t k = 0;
    for (size_t j = 0; j < N; ++j)
    {
        for (size_t i = 0; i < M; ++i)
        {
            m.data()[k++] = matrix.data()[i * N + j];
        }
    }

    return m;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::inverse(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square)
{
    auto d = matrix.determinant();

    if (math::float_eq0(d))
    {
        return Matrix<T, M, N>(); // fill with infinity, assert, return optional ...
    }

    auto rep = matrix.data();

    if constexpr (M == 1)
    {
        return {T(1) / d};
    }
    else if constexpr (M == 2)
    {
        return {rep[3] / d, -rep[1] / d, -rep[2] / d, rep[0] / d};
    }
    else if constexpr (M == 3)
    {
        Matrix<T, M, N> m
        {
            rep[4] * rep[8] - rep[5] * rep[7],
            rep[2] * rep[7] - rep[1] * rep[8],
            rep[1] * rep[5] - rep[2] * rep[4],
            rep[5] * rep[6] - rep[3] * rep[8],
            rep[0] * rep[8] - rep[2] * rep[6],
            rep[2] * rep[3] - rep[0] * rep[5],
            rep[3] * rep[7] - rep[4] * rep[6],
            rep[1] * rep[6] - rep[0] * rep[7],
            rep[0] * rep[4] - rep[1] * rep[3],
        };

        return m / d;
    }
    else
    {
        return adjugate(matrix) / d;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, N, M> quetzal::math::pseudo_inverse(const Matrix<T, M, N>& matrix)
{
    Matrix<T, N, M> transposed = transpose(matrix);
    return inverse(transposed * matrix) * transposed;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::adjugate(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square)
{
    return transpose(comatrix(matrix));
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
quetzal::math::Matrix<T, M, N> quetzal::math::comatrix(const Matrix<T, M, N>& matrix) requires (Matrix<T, M, N>::square)
{
    Matrix<T, M, N> m;

    size_t k = 0;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            m.data()[k++] = matrix.cofactor(i, j);
        }
    }

    return m;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::tuple<quetzal::math::Matrix<T, M, N>, T> quetzal::math::gaussian_elimination(Matrix<T, M, N> matrix)
{
    auto factor = T(1);

    for (size_t k = 0, l = 0; k < M && l < N; ++k, ++l)
    {
        // Find row with greatest first element
        size_t imax = k;
        T amax = std::abs(matrix(k, l));
        for (size_t i = k + 1; i < M; ++i)
        {
            auto a = std::abs(matrix(i, l));
            if (a > amax)
            {
                imax = i;
                amax = a;
            }
        }

        if (float_eq0(amax))
        {
            ++l;
            continue;
        }

        // Move that row
        if (imax != k)
        {
            matrix.swap_rows(imax, k);
            factor *= T(-1);
        }

        // Set all lower rows to have 0 in this column
        auto s = matrix(k, l);
        assert(float_ne0(s));

        for (size_t i = k + 1; i < M; ++i)
        {
            T& t = matrix(i, l);
            matrix.add_row_multiple(-t / s, k, i);
            assert(float_eq0(t));
            t = T(0);
        }
    }

    return {matrix, factor};
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::tuple<quetzal::math::Matrix<T, M, N>, bool> quetzal::math::gauss_jordan_elimination(Matrix<T, M, N> matrix)
{
    bool nonsingular = true;

    for (size_t k = 0, l = 0; k < M && l < N; ++k, ++l) // l < M? ...
    {
        // Find row with greatest first element
        size_t imax = k;
        T amax = std::abs(matrix(k, l));
        for (size_t i = k + 1; i < M; ++i)
        {
            auto a = std::abs(matrix(i, l));
            if (a > amax)
            {
                imax = i;
                amax = a;
            }
        }

        if (float_eq0(amax))
        {
            ++l;
            nonsingular = false;
            continue;
        }

        // Move that row and set first element 1
        if (imax != k)
        {
            matrix.swap_rows(imax, k);
        }

        assert(float_ne0(matrix(k, l)));
        matrix.scale_row(T(1) / matrix(k, l), k);
        matrix(k, l) = T(1);

        // Set all other rows 0 in this column
        for (size_t i = 0; i < M; ++i)
        {
            if (i != k)
            {
                T& t = matrix(i, l);
                matrix.add_row_multiple(-matrix(i, l), k, i);
                assert(float_eq0(t));
                t = T(0);
            }
        }
    }

    return {matrix, nonsingular};
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
bool quetzal::math::float_eq(const Matrix<T, M, N>& lhs, const Matrix<T, M, N>& rhs, int ulp)
{
    for (size_t i = 0; i < lhs.size(); ++i)
    {
        if (!float_eq(lhs.data()[i], rhs.data()[i], ulp))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::ostream& quetzal::math::operator<<(std::ostream& os, const Matrix<T, M, N>& matrix)
{
    size_t k = 0;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            if (j > 0)
            {
                os << '\t';
            }

            os << matrix.data()[k++];
        }

        os << '\n';
    }

    return os;
}

#endif // QUETZAL_MATH_MATRIX_HPP
