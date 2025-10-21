//------------------------------------------------------------------------------
// matrix_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/math/Matrix_test.hpp"

#include "quetzal/common/Timestamp.hpp"
#include "quetzal/geometry/Point.hpp"
//#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/vector_util.hpp"
#include "quetzal/math/math_xm.hpp"
#include <concepts>
#include <ranges>
#include <string>
#include <iostream>

#include "Eigen/Eigen"
#include "Eigen/Core"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    const size_t dimension = 2;
    const bool xm_compatible = same_as<value_type, float> && dimension == 3;
    using vector_traits = math::HomogeneousTraits<value_type, dimension>;
    using matrix_type = math::Matrix<value_type, dimension + 1, dimension + 1>;
    using p_matrix_type = math::Matrix<value_type, 4, dimension + 1>;

    math::Random g_random;

    const size_t N_TESTS = 10;
    const size_t N_BENCHMARK = 10000;

    int ulp = 100000;

    //--------------------------------------------------------------------------
    template<typename M>
    void set_random_elements(M& matrix)
    {
        for (size_t i = 0; i < matrix.size(); ++i)
        {
            matrix.data()[i] = g_random.gen(-100.0f, 100.0f);
        }
        return;
    }

    //--------------------------------------------------------------------------
    string check(bool b)
    {
        return b ? "ok" : "bad";
    }

    //--------------------------------------------------------------------------
    template<typename M> requires M::square
    void test_determinant_accuracy(const M& matrix)
    {
        auto d = matrix.determinant();
        auto d_g = matrix.determinant_g();
        auto d_x = [&]() { if constexpr (xm_compatible) return determinant_xm(matrix); else return value_type(0); }();
        auto d_e = matrix.determinant_e();

        cout << "d:   " << d << " " << check(math::float_eq(d, d_e, ulp)) << endl;
        cout << "d_g: " << d_g << " " << check(math::float_eq(d_g, d_e, ulp)) << endl;
        if constexpr (xm_compatible)
        {
            cout << "d_x: " << d_x << " " << check(math::float_eq(d_x, d_e, ulp)) << endl;
        }
        cout << "d_e: " << d_e << endl;
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M> requires M::square
    void test_inverse_accuracy(const M& matrix)
    {
        auto m = inverse(matrix);
        auto m_g = inverse_g(matrix);
        auto m_g_ = inverse_g_(matrix);
        auto m_x = [&]() { if constexpr (xm_compatible) return inverse_xm(matrix); else return M{}; }();
        auto m_e = inverse_e(matrix);

        cout << "inverse: " << check(float_eq(m, m_e, ulp)) << endl;
        cout << m;
        cout << "inverse_g: " << check(float_eq(m_g.value(), m_e, ulp)) << endl;
        if (m_g.has_value())
        {
            cout << m_g.value();
        }
        else
        {
            cout << "[singular]" << endl;
        }
        cout << "inverse_g_: " << check(float_eq(m_g_, m_e, ulp)) << endl;
        cout << m_g_;
        if constexpr (xm_compatible)
        {
            cout << "inverse_x: " << check(float_eq(m_x, m_e, ulp)) << endl;
            cout << m_x;
        }
        cout << "inverse_e" << endl;
        cout << m_e;

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void test_pseudo_inverse_accuracy(const M& matrix)
    {
        auto m = pseudo_inverse(matrix);
        auto m_g = pseudo_inverse_g(matrix);
        auto m_g_ = pseudo_inverse_g_(matrix);
        auto m_e = pseudo_inverse_e(matrix);
        auto m_svd_e = pseudo_inverse_svd_e(matrix);
//        auto m_svd_o = pseudo_inverse_svd_o(matrix);
        cout << "pseudo_inverse: " << check(float_eq(m, m_e, ulp)) << endl;
        cout << m;
        cout << "pseudo_inverse_g: " << check(float_eq(m_g, m_e, ulp)) << endl;
        cout << m_g;
        cout << "pseudo_inverse_g_: " << check(float_eq(m_g_, m_e, ulp)) << endl;
        cout << m_g_;
        cout << "pseudo_inverse_e" << endl;
        cout << m_e;
        cout << "pseudo_inverse_svd_e: " << check(float_eq(m_svd_e, m_e, ulp)) << endl;
        cout << m_svd_e;
//        cout << "pseudo_inverse_svd_o: " << check(float_eq(m_svd_o, m_e, ulp)) << endl;
//        cout << m_svd_o;
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void test_pseudo_inverse_svd(const M& matrix)
    {
        auto m_e = pseudo_inverse_e(matrix);
        auto m_svd_e = pseudo_inverse_svd_e(matrix);
//        auto m_svd_o = pseudo_inverse_svd_o(matrix);
        cout << "pseudo_inverse_e" << endl;
        cout << m_e;
        cout << "pseudo_inverse_svd_e" << endl;
        cout << m_svd_e;
//        cout << "pseudo_inverse_svd_o" << endl;
//        cout << m_svd_o;
        return;
    }

    //--------------------------------------------------------------------------
    double benchmark_multiplication(size_t n)
    {
        matrix_type A;
        matrix_type B;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(A);
            set_random_elements(B);
     
            Timestamp ts0;
            auto C = A * B;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_multiplication_e(size_t n)
    {
        matrix_type A;
        matrix_type B;
        matrix_type C;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(A);
            set_random_elements(B);
     
            Timestamp ts0;
            Eigen::Map<Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols>> D(A.data());
            Eigen::Map<Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols>> E(B.data());
            Eigen::Map<Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols>> F(C.data());
            F = D * E;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_multiplication_e_direct(size_t n)
    {
        matrix_type A;
        matrix_type B;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(A);
            set_random_elements(B);
     
            Timestamp ts0;
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> C(A.data());
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> D(B.data());
            auto E = C * D;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_multiplication_e_native(size_t n)
    {
        matrix_type A;
        matrix_type B;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(A);
            set_random_elements(B);

            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> C(A.data());
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> D(B.data());
     
            Timestamp ts0;
            auto E = C * D;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto d = matrix.determinant();
            d;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant_g(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto d = matrix.determinant_g();
            d;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant_x(size_t n)
    {
        double duration = 0;

#if xm_compatible
        if constexpr (xm_compatible)
        {
            matrix_type matrix;
            for (size_t i = 0; i < n; ++i)
            {
                set_random_elements(matrix);
     
                Timestamp ts0;
                auto d = determinant_xm(matrix);
                d;
                duration += Timestamp::now() - ts0;
            }
        }
#else
        n;
#endif

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant_e(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto d = matrix.determinant_e();
            d;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant_e_direct(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
            auto d = A.determinant();
            d;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_determinant_e_native(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
     
            Timestamp ts0;
            auto d = A.determinant();
            d;
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = inverse(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_g(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = inverse_g(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_g_(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = inverse_g_(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_x(size_t n)
    {
        double duration = 0;

#if xm_compatible
        if constexpr (xm_compatible)
        {
            matrix_type matrix;
            for (size_t i = 0; i < n; ++i)
            {
                set_random_elements(matrix);
     
                Timestamp ts0;
                auto m = inverse_xm(matrix);
                duration += Timestamp::now() - ts0;
            }
        }
#else
        n;
#endif

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_e(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = inverse_e(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_e_direct(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
            auto m = A.inverse();
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_inverse_e_native(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
     
            Timestamp ts0;
            auto m = A.inverse();
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = pseudo_inverse(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse_g(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = pseudo_inverse_g(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse_g_(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = pseudo_inverse_g_(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse_e(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            auto m = pseudo_inverse_e(matrix);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse_e_direct(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
     
            Timestamp ts0;
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
            auto m = A.completeOrthogonalDecomposition().pseudoInverse();
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_pseudo_inverse_e_native(size_t n)
    {
        matrix_type matrix;
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            set_random_elements(matrix);
            Eigen::Matrix<value_type, matrix_type::rows, matrix_type::cols> A(matrix.data());
     
            Timestamp ts0;
            auto m = A.completeOrthogonalDecomposition().pseudoInverse();
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    matrix_type a(1);
    matrix_type b = matrix_type::Identity();
    matrix_type c;
    matrix_type b1(1);
    geometry::Point<vector_traits> u = {};
    math::Vector<vector_traits> v;

    assert(b1 == b);

    c = -a;
    c = a + b;
    c = a - b;
    c = a * b;
    c = a * 2.0f;
    c = 2.0f * a;

    a += b;
    a -= b;
    a *= b;
    a *= 2.0f;
    a /= 2.0f;

    auto av = a * v;
    auto va = v * a;

    v *= a;

    auto e = transpose(a);
    e = inverse(a);
    e.transpose();
    e.inverse();

    matrix_type matrix;
    for (size_t i = 0; i < N_TESTS; ++i)
    {
        set_random_elements(matrix);
        cout << "matrix" << endl;
        cout << matrix;

//        test_determinant_accuracy(matrix);
//        test_inverse_accuracy(matrix);
        test_pseudo_inverse_accuracy(matrix);

        test_pseudo_inverse_svd(matrix);
    }

    p_matrix_type pmatrix;
    for (size_t i = 0; i < N_TESTS; ++i)
    {
        set_random_elements(pmatrix);
        cout << "pmatrix" << endl;
        cout << pmatrix;

        test_pseudo_inverse_accuracy(pmatrix);
    }

    cout << "benchmark: " << N_BENCHMARK << " reps" << endl;
    double d = 0;

    d = benchmark_multiplication(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "multiplication", d) <<  endl;

    d = benchmark_multiplication_e(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "multiplication_e", d) <<  endl;

    d = benchmark_multiplication_e_direct(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "multiplication_e_direct", d) <<  endl;

    d = benchmark_multiplication_e_native(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "multiplication_e_native", d) <<  endl;

    d = benchmark_determinant(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "determinant", d) <<  endl;

    d = benchmark_determinant_g(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "determinant_g", d) <<  endl;

    if constexpr (xm_compatible)
    {
        d = benchmark_determinant_x(N_BENCHMARK);
        cout << format("{}: {: .4f} s", "determinant_x", d) <<  endl;
    }

    d = benchmark_determinant_e(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "determinant_e", d) <<  endl;

    d = benchmark_determinant_e_direct(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "determinant_e_direct", d) <<  endl;

    d = benchmark_determinant_e_native(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "determinant_e_native", d) <<  endl;

    d = benchmark_inverse(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse", d) <<  endl;

    d = benchmark_inverse_g(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse_g", d) <<  endl;

    d = benchmark_inverse_g_(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse_g_", d) <<  endl;

    if constexpr (xm_compatible)
    {
        d = benchmark_inverse_x(N_BENCHMARK);
        cout << format("{}: {: .4f} s", "inverse_x", d) <<  endl;
    }

    d = benchmark_inverse_e(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse_e", d) <<  endl;

    d = benchmark_inverse_e_direct(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse_e_direct", d) <<  endl;

    d = benchmark_inverse_e_native(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "inverse_e_native", d) <<  endl;

    d = benchmark_pseudo_inverse(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse", d) <<  endl;

    d = benchmark_pseudo_inverse_g(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse_g", d) <<  endl;

    d = benchmark_pseudo_inverse_g_(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse_g_", d) <<  endl;

    d = benchmark_pseudo_inverse_e(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse_e", d) <<  endl;

    d = benchmark_pseudo_inverse_e_direct(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse_e_direct", d) <<  endl;

    d = benchmark_pseudo_inverse_e_native(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "pseudo_inverse_e_native", d) <<  endl;

    return 0;
}
