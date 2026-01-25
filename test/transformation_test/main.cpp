//------------------------------------------------------------------------------
// transformation_test
// main.cpp
//------------------------------------------------------------------------------

#define _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS

#include "quetzal/math/Matrix_test.hpp"
#include "quetzal/geometry/transformation_matrix_test.hpp"

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
//#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/matrix_util.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <vector>
#include <cassert>

#include "Eigen/Eigen"
#include "Eigen/Core"

//#include <complex>
//#define lapack_complex_float std::complex<float>
//#define lapack_complex_double std::complex<double>
//#include "lapack.h"

using namespace std;
using namespace quetzal;

namespace g = quetzal::geometry;
namespace m = quetzal::math;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = typename mesh_type::point_type;
    using vector_type = point_type::vector_type;
    using projected_type = g::Projection<vector_traits>::projected_type;

    using point2_type = g::Point<math::HomogeneousTraits<value_type, 2>>;
    using matrix2_type = math::Matrix<value_type, 3, 3>;

    math::Random g_random;

    const size_t N_BENCHMARK = 10000;

    static const filesystem::path dirOut = "out/";
    static const filesystem::path dirReference = "reference/";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dirOut / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    string result(bool b)
    {
        return b ? "success" : "fail";
    }

    //--------------------------------------------------------------------------
    template<typename T>
    point_type to_point3(g::Point<math::HomogeneousTraits<T, 2>> t)
    {
        return {t[0], t[1], T(0), t[2]};
    }

    //--------------------------------------------------------------------------
    template<typename T>
    void set_random_elements(T& collection)
    {
        for (size_t i = 0; i < collection.size(); ++i)
        {
            collection.data()[i] = g_random.gen(typename T::value_type(-100.0), typename T::value_type(100.0));
        }
        return;
    }

    //--------------------------------------------------------------------------
    //template<typename M, typename P = g::Projection<vector_traits>::projected_type>
    template<typename M>
    void show_results2(const M& matrix, std::span<const projected_type> from, std::span<const projected_type> to)
    {
        for (auto [original, target] : std::ranges::zip_view(from, to))
        {
            auto transformed = original * matrix;
// consider identifier and projective divide ...
//            cout << halfedge.id() << "\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << "\t\t" << transformed.projective_divide() << endl;
//            cout << to_fixed_width(texcoord_orig) << to_fixed_width(transformed) << "\t" << result(float_eq(transformed, texcoord_orig)) << endl;
            cout << to_fixed_width(transformed) << "\t" << result(float_eq(transformed, target)) << endl;
        }
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void show_results3(const M& matrix, std::span<const point_type> from, std::span<const point_type> to)
    {
        for (auto [original, target] : std::ranges::zip_view(from, to))
        {
            auto transformed = original * matrix;
// consider identifier and projective divide ...
//            cout << halfedge.id() << "\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << "\t\t" << transformed.projective_divide() << endl;
//            cout << to_fixed_width(texcoord_orig) << to_fixed_width(transformed) << "\t" << result(float_eq(transformed, texcoord_orig)) << endl;
            cout << to_fixed_width(transformed) << "\t" << result(float_eq(transformed, target)) << endl;
        }
    }
/*
    //--------------------------------------------------------------------------
    void affine_mapping_test1(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        using projected_type = g::Projection<vector_traits>::projected_type;
        using texcoord_type = typename mesh_type::vertex_attributes_type::texcoord_type;

        vector<projected_type> positions;
        vector<texcoord_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
            //cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << positions.back() << "\t\t" << texcoords.back() << endl;
        }

        auto matrix_linear = geometry::linear_mapping(texcoords[1] - texcoords[0], texcoords[2] - texcoords[0], positions[1] - positions[0], positions[2] - positions[0]);
        cout << "linear" << endl;
        cout << matrix_linear;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position()) - positions[0];
            auto texcoord_orig = attributes.texcoord() - texcoords[0];
            auto transformed = texcoord_orig * matrix_linear;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, position_orig)) << endl;
            //assert(float_eq(transformed, position_orig));
        }

        auto matrix = g::affine_mapping<projected_type>(texcoords, positions);
        cout << "affine" << endl;
        cout << matrix;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
            auto transformed = texcoord_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, position_orig)) << endl;
            //assert(float_eq(transformed, position_orig));
        }
/*
        texcoord_type x = {0.0f, -1.0f};
        auto y = x * matrix;
        cout << x << "\t" << y << "\t\t" << y.vector() << "\t\t" << normalize(y.vector()) << endl;
        x = {0.0f, 1.0f};
        y = x * matrix;
        cout << x << "\t" << y << "\t\t" << y.vector() << "\t\t" << normalize(y.vector()) << endl;
        x = {1.0f, 0.0f};
        y = x * matrix;
        cout << x << "\t" << y << "\t\t" << y.vector() << "\t\t" << normalize(y.vector()) << endl;
        x = {-1.0f, 0.0f};
        y = x * matrix;
        cout << x << "\t" << y << "\t\t" << y.vector() << "\t\t" << normalize(y.vector()) << endl;
* /
        return;
    }

    //--------------------------------------------------------------------------
    void affine_mapping_test2(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        using projected_type = g::Projection<vector_traits>::projected_type;
        using texcoord_type = typename mesh_type::vertex_attributes_type::texcoord_type;

        vector<projected_type> positions;
        vector<texcoord_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
            //cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << positions.back() << "\t\t" << texcoords.back() << endl;
        }

        auto matrix = g::affine_mapping<projected_type>(positions, texcoords);

        auto matrix_linear = geometry::linear_mapping(positions[1] - positions[0], positions[2] - positions[0], texcoords[1] - texcoords[0], texcoords[2] - texcoords[0]);
        cout << "linear" << endl;
        cout << matrix_linear;
        cout << "affine" << endl;
        cout << matrix;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
            auto transformed = position_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << endl;
            //assert(float_eq(transformed, texcoord_orig));
        }

        return;
    }
*/
/* different dimensions ...
    //--------------------------------------------------------------------------
    void projective_mapping_test(const mesh_type::face_type& face)
    {
        vector<typename mesh_type::point_type> positions;
        vector<typename mesh_type::vertex_attributes_type::texcoord_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            positions.push_back(attributes.position());
            texcoords.push_back(attributes.texcoord());
        }

        auto matrix = g::projective_mapping<typename mesh_type::point_type>(texcoords, positions);

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            assert(float_eq(attributes.position() * matrix, attributes.texcoord()));
        }

        return;
    }
*/
    //--------------------------------------------------------------------------
    void projective_mapping_test2(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        //using projected_type = g::Projection<vector_traits>::projected_type;

static_assert(projected_type::traits_type::size == 3);
static_assert(projected_type::traits_type::dimension == 2);
static_assert(projected_type::dimension == 2);
static_assert(projected_type::component_count == 3);

        vector<id_type> halfedge_ids;
        vector<projected_type> positions;
        vector<projected_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
        }

        for (auto [id, position, texcoord] : std::ranges::zip_view(halfedge_ids, positions, texcoords))
        {
            cout << id << to_fixed_width(face.mesh().halfedge(id).vertex_attributes().position()) << to_fixed_width(position) << to_fixed_width(texcoord) << endl;
        }

        cout << "projective_mapping_2p" << endl;
//        auto matrix = g::projective_mapping<projected_type>(positions, texcoords);
//        math::Matrix<value_type, 3, 3> matrix = g::projective_mapping<projected_type>(positions, texcoords);
        auto matrix = g::projective_mapping<projected_type>(positions, texcoords);
        cout << "transformation matrix" << endl << matrix;

        auto [m, singular] = gaussian_elimination(matrix);
        cout << singular << endl;
        cout << m;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
//            auto position_orig = projection(attributes.position());
            projected_type position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
//            auto transformed = position_orig * matrix;
            projected_type transformed = position_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << endl;
        }

        cout << "projective_mapping_2p_g" << endl;
        matrix = g::projective_mapping_g<projected_type>(positions, texcoords);
        cout << "transformation matrix" << endl << matrix;
        cout << "result" << endl;
        show_results2(matrix, positions, texcoords);

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
            auto transformed = position_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << endl;
        }

        cout << "projective_mapping_2p_g_" << endl;
        matrix = g::projective_mapping_g_<projected_type>(positions, texcoords);
        cout << "transformation matrix" << endl << matrix;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
            auto transformed = position_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << endl;
        }

        cout << "projective_mapping_2p_e" << endl;
        matrix = g::projective_mapping_e<projected_type>(positions, texcoords);
        cout << "transformation matrix" << endl << matrix;

        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            auto position_orig = projection(attributes.position());
            auto texcoord_orig = attributes.texcoord();
            auto transformed = position_orig * matrix;
            cout << halfedge.id() << "\t" << attributes.position() << "\t\t" << position_orig << "\t\t" << texcoord_orig << "\t\t" << transformed << "\t\t" << result(float_eq(transformed, texcoord_orig)) << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void projective_mapping_test3(const mesh_type::face_type& face)
    {
        vector<id_type> halfedge_ids;
        vector<point_type> positions;
        vector<point_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(attributes.position());
            texcoords.push_back(to_point3(attributes.texcoord()));
        }

        // actual halfedge positions produce 0.159041?
        // positions as they appear in output produce 0.315794?
        // positions as if they were "exact" produce ?

/*
        positions =
        {
            {43.3013, 75, 50, 1},
            {0, 86.60250000000001, 50, 1},
            {0, 50, 86.60250000000001, 1},
            {25, 43.3013, 86.60250000000001, 1},
        };
*/

        for (auto [id, position, texcoord] : std::ranges::zip_view(halfedge_ids, positions, texcoords))
        {
            cout << id << to_fixed_width(position) << to_fixed_width(texcoord) << endl;
        }

        cout << "projective_mapping_3" << endl;
        auto matrix = g::projective_mapping<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_i" << endl;
        matrix = g::projective_mapping_i<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_g" << endl;
        matrix = g::projective_mapping_g<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_g_" << endl;
        matrix = g::projective_mapping_g_<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_e" << endl;
        matrix = g::projective_mapping_e<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_i_e" << endl;
        matrix = g::projective_mapping_i_e<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_svd_e" << endl;
        matrix = g::projective_mapping_svd_e<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);
/*
        cout << "projective_mapping_3_o_sim" << endl;
        matrix = g::projective_mapping_o_sim<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_svd_o" << endl;
        matrix = g::projective_mapping_svd_o<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);

        cout << "projective_mapping_3_jsvd_o" << endl;
        matrix = g::projective_mapping_jsvd_o<point_type>(positions, texcoords);
        cout << "transformation matrix" << endl << to_fixed_width(matrix);
        cout << "result" << endl;
        show_results3(matrix, positions, texcoords);
*/
        return;
    }

    //--------------------------------------------------------------------------
    void eigen_test(const mesh_type::face_type& face)
    {
        using T = value_type;

        vector<id_type> halfedge_ids;
        vector<point_type> positions;
        vector<point_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(attributes.position());
            texcoords.push_back(to_point3(attributes.texcoord()));
        }

/*
        positions =
        {
            {43.3013, 75, 50, 1},
            {0, 86.60250000000001, 50, 1},
            {0, 50, 86.60250000000001, 1},
            {25, 43.3013, 86.60250000000001, 1},
        };

        {
            {43.3013, 75.0000, 50.0000, 1.0000},
            {0.0000, 86.6025, 50.0000, 1.0000},
            {0.0000, 50.0000, 86.6025, 1.0000},
            {25.0000, 43.3013, 86.6025, 1.0000},
        };
*/

        for (auto [id, position, texcoord] : std::ranges::zip_view(halfedge_ids, positions, texcoords))
        {
            cout << id << to_fixed_width(position) << to_fixed_width(texcoord) << endl;
        }

        math::Matrix<T, 4, 4> from;
        math::Matrix<T, 4, 4> to;
        for (size_t i = 0; i < 4; ++i)
        {
            size_t j = i * 4;
            auto p = positions[i].data();
            std::copy(p, p + 4, &from.data()[j]);
            auto q = texcoords[i].data();
            std::copy(q, q + 4, &to.data()[j]);
        }

        cout << "from" << endl;
        cout << from << endl;

        cout << "to" << endl;
        cout << to << endl;

        Eigen::Map<Eigen::Matrix<T, 4, 4, Eigen::RowMajor>> e_f(from.data());
//        Eigen::Matrix<T, 4, 4> e_from = e_f;
//        {
//            {43.3013, 75.0000, 50.0000, 1.0000},
//            {0.0000, 86.6025, 50.0000, 1.0000},
//            {0.0000, 50.0000, 86.6025, 1.0000},
//            {25.0000, 43.3013, 86.6025, 1.0000},
//        };

//        Eigen::Map<Eigen::Matrix<T, 4, 4, Eigen::RowMajor>> e_from(from.data());
//        Eigen::Map<Eigen::Matrix<T, 4, 4, Eigen::RowMajor>> e_to(to.data());
        Eigen::Matrix<T, 4, 4> e_from(from.data());
        Eigen::Matrix<T, 4, 4> e_to(to.data());

        cout << "e_from" << endl;
        cout << e_from << endl;

        cout << "e_to" << endl;
        cout << e_to << endl;

//        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> e_t = e_from.inverse() * e_to;
        Eigen::Matrix<T, 4, 4> e_t = e_from.transpose().inverse() * e_to.transpose();
        cout << "e_t" << endl;
        cout << e_t << endl;

        cout << "e_from * e_t" << endl;
        cout << (e_from * e_t) << endl;

        cout << "e_from * e_t.transpose()" << endl;
        cout << (e_from * e_t.transpose()) << endl;

        math::Matrix<T, 4, 4> t(std::span(e_t.data(), 16));

        cout << "from * t" << endl;
        cout << from * t << endl;

//        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> A(from.data());

        const T* pf = from.data();
//        Eigen::Matrix<T, 4, 4> A(pf);

        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> A
        {
            {pf[0], pf[1], pf[2], pf[3]},
            {pf[4], pf[5], pf[6], pf[7]},
            {pf[8], pf[9], pf[10], pf[11]},
            {pf[12], pf[13], pf[14], pf[15]},
        };
/*
        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> A
        {
            {43.3013, 75.0000, 50.0000, 1.0000},
            {0.0000, 86.6025, 50.0000, 1.0000},
            {0.0000, 50.0000, 86.6025, 1.0000},
            {25.0000, 43.3013, 86.6025, 1.0000},
        };
*/
        Eigen::Matrix<T, 4, 4> B
        {
            {0.1667, 0.3333, 0, 1},
            {0.25, 0.3333, 0, 1},
            {0.25, 0.1667, 0, 1},
            {0.1667, 0.1667, 0, 1},
        };

        auto C = A.inverse() * B;
        auto D = A.completeOrthogonalDecomposition().pseudoInverse() * B;
        auto At = A.transpose();

        cout << "A" << endl;
        cout << A << endl;
        cout << "B" << endl;
        cout << B << endl;
        cout << "C = A.inverse() * B" << endl;
        cout << C << endl;
        cout << "D = A.completeOrthogonalDecomposition().pseudoInverse() * B" << endl;
        cout << D << endl;
        cout << "A * C" << endl;
        cout << A * C << endl;
        cout << "A * D" << endl;
        cout << A * D << endl;

        Eigen::Map<Eigen::Matrix<T, 4, 4>> em(A.data());
        auto a = A.data();
        auto f = from.data();
        auto ft = transpose(from).data();
        auto e = em.data();
        cout << "a f ft e" << endl;
        for (size_t i = 0; i < 16; ++i)
        {
            cout << setw(16) << a[i] << setw(16) << f[i] << setw(16) << ft[i] << setw(16) << e[i] << endl;
        }

        // just remaining to replace A with from ...
        Eigen::Matrix<T, 4, 4> emi(A.data());
//        Eigen::Matrix<T, 4, 4> emi(ft);
//        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> emi(ft);
        cout << "emi" << endl;
        math::Matrix<T, 4, 4> mo;
        Eigen::Map<Eigen::Matrix<T, 4, 4>> emo(mo.data());
        emo = emi.completeOrthogonalDecomposition().pseudoInverse();
        cout << "mo (inverse)" << endl;
        cout << mo << endl;

        math::Matrix<T, 4, 4> ao(span(A.data(), 16));
        cout << "ao" << endl;
        cout << ao << endl;
        cout << "from" << endl;
        cout << from << endl;

        auto E = mo * to;
        cout << "E = mo * to" << endl;
        cout << E << endl;
        cout << "ao * E" << endl;
        cout << (ao * E) << endl;
        cout << "from * E" << endl;
        cout << (from * E) << endl;

        return;
    }

    //--------------------------------------------------------------------------
    void eigen_test2(const mesh_type::face_type& face)
    {
        using T = value_type;

        vector<id_type> halfedge_ids;
        vector<point_type> positions;
        vector<point_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(attributes.position());
            texcoords.push_back(to_point3(attributes.texcoord()));
        }

        for (auto [id, position, texcoord] : std::ranges::zip_view(halfedge_ids, positions, texcoords))
        {
            cout << id << to_fixed_width(position) << to_fixed_width(texcoord) << endl;
        }

        math::Matrix<T, 4, 4> from;
        math::Matrix<T, 4, 4> to;
        for (size_t i = 0; i < 4; ++i)
        {
            size_t j = i * 4;
            auto p = positions[i].data();
            std::copy(p, p + 4, &from.data()[j]);
            auto q = texcoords[i].data();
            std::copy(q, q + 4, &to.data()[j]);
        }

        cout << "from" << endl;
        cout << from << endl;

        cout << "to" << endl;
        cout << to << endl;

        Eigen::Matrix<T, 4, 4, Eigen::RowMajor> A
        {
            {43.3013, 75.0000, 50.0000, 1.0000},
            {0.0000, 86.6025, 50.0000, 1.0000},
            {0.0000, 50.0000, 86.6025, 1.0000},
            {25.0000, 43.3013, 86.6025, 1.0000},
        };

        cout << "A" << endl;
        cout << A << endl;

        const T* pa = A.data();
        const T* pf = from.data();

        auto p = pa;

        Eigen::Matrix<T, 4, 4> emi(p);
        cout << "emi" << endl;
        math::Matrix<T, 4, 4> mo;
        Eigen::Map<Eigen::Matrix<T, 4, 4>> emo(mo.data());
        emo = emi.completeOrthogonalDecomposition().pseudoInverse();
        cout << "mo (inverse)" << endl;
        cout << mo << endl;
        auto E = mo * to;
        cout << "E = mo * to" << endl;
        cout << E << endl;

        cout << "from" << endl;
        cout << from << endl;
        math::Matrix<T, 4, 4> ao(span(A.data(), 16));
        cout << "ao" << endl;
        cout << ao << endl;

        cout << "from * E" << endl;
        cout << (from * E) << endl;
        cout << "ao * E" << endl;
        cout << (ao * E) << endl;

        return;
    }

    //--------------------------------------------------------------------------
    void ai_test(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        vector<id_type> halfedge_ids;
        vector<projected_type> positions;
        vector<projected_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
        }

        using T = value_type;
        math::Matrix<T, 4, 3> from;
        math::Matrix<T, 4, 3> to;
        for (size_t i = 0; i < 4; ++i)
        {
            size_t j = i * 3;
            auto p = positions[i].data();
            std::copy(p, p + 3, &from.data()[j]);
            auto q = texcoords[i].data();
            std::copy(q, q + 3, &to.data()[j]);
        }

        cout << "from" << endl;
        cout << from << endl;

        cout << "to" << endl;
        cout << to << endl;

        // Construct the matrix for 8 equations and 9 unknowns
        Eigen::Matrix<T, 8, 9> A;
        for (int i = 0; i < 4; ++i)
        {
            T x = positions[i].x();
            T y = positions[i].y();
            T x_prime = texcoords[i].x();
            T y_prime = texcoords[i].y();

            A.row(2 * i) << -x, -y, -1, 0, 0, 0, x * x_prime, y * x_prime, x_prime;
            A.row(2 * i + 1) << 0, 0, 0, -x, -y, -1, x * y_prime, y * y_prime, y_prime;
        }

        Eigen::JacobiSVD<Eigen::Matrix<T, 8, 9>> svd(A, Eigen::ComputeFullV);
        Eigen::Matrix<T, 9, 1> h = svd.matrixV().col(8); // The last column of V gives the solution

        // Reshape h into a 3x3 matrix and normalize
        math::Matrix<T, 3, 3> H;
        T* hd = H.data();
        for (int i = 0; i < 9; ++i)
        {
            hd[i] = h[i];
        }
        H /= H(2, 2); // Normalize so that H(2,2) = 1
        H.transpose();

        auto result = from * H;
        cout << "result" << endl;
        cout << result << endl;

        for (int i = 0; i < 4; ++i)
        {
            result(i, 0) /= result(i, 2);
            result(i, 1) /= result(i, 2);
            result(i, 2) = T(1);
        }
        cout << "n result" << endl;
        cout << result << endl;

        auto matrix = g::projective_mapping_h<projected_type>(positions, texcoords);
        auto transformed = from * matrix;
        cout << "h result" << endl;
        // this should be a view ...
        for (size_t i = 0; i < transformed.rows; ++i)
        {
            projected_type point;
            for (size_t j = 0; j < transformed.cols; ++j)
            {
                point[j] = transformed(i, j);
            }
            cout << point << "    " << point.projective_divide() << endl;
        }

        size_t k = 0;
        for (size_t i = 0; i < transformed.rows; ++i)
        {
            projected_type point;
            auto p = &transformed.data()[k];
            std::copy(p, p + 3, point.data());
            cout << point.projective_divide() << endl;
            k += 3;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void ai2_test(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        vector<id_type> halfedge_ids;
        vector<projected_type> positions;
        vector<projected_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
        }

        using T = value_type;
        math::Matrix<T, 4, 3> from;
        math::Matrix<T, 4, 3> to;
        for (size_t i = 0; i < 4; ++i)
        {
            size_t j = i * 3;
            auto p = positions[i].data();
            std::copy(p, p + 3, &from.data()[j]);
            auto q = texcoords[i].data();
            std::copy(q, q + 3, &to.data()[j]);
        }

        cout << "from" << endl;
        cout << from << endl;

        cout << "to" << endl;
        cout << to << endl;

        // Construct the A matrix
        Eigen::Matrix<T, 8, 8> A;
        Eigen::Vector<T, Eigen::Dynamic> b(8);          
        for (int i = 0; i < 4; ++i)
        {
            T x = positions[i].x();
            T y = positions[i].y();
            T x_prime = texcoords[i].x();
            T y_prime = texcoords[i].y();

            A.row(2 * i) << x, y, 1, 0, 0, 0, -x * x_prime, -y * x_prime;
            b(2 * i) = x_prime;

            A.row(2 * i + 1) << 0, 0, 0, x, y, 1, -x * y_prime, -y * y_prime;
            b(2 * i + 1) = y_prime;
        }

        // Solve the linear system A * h = b
        Eigen::Vector<T, Eigen::Dynamic> h = A.colPivHouseholderQr().solve(b);

        // Reshape h into a 3x3 matrix, setting H(2,2) = 1 explicitly
        math::Matrix<T, 3, 3> H;
        T* hd = H.data();
        for (int i = 0; i < 8; ++i)
        {
            hd[i] = h[i];
        }
        hd[8] = T(1);
        H.transpose();

        auto result = from * H;
        cout << "result" << endl;
        cout << result << endl;

        for (int i = 0; i < 4; ++i)
        {
            result(i, 0) /= result(i, 2);
            result(i, 1) /= result(i, 2);
            result(i, 2) = T(1);
        }
        cout << "n result" << endl;
        cout << result << endl;

        auto matrix = g::projective_mapping_h2<projected_type>(positions, texcoords);
        auto transformed = from * matrix;
        cout << "h result" << endl;
        // this should be a view ...
        for (size_t i = 0; i < transformed.rows; ++i)
        {
            projected_type point;
            for (size_t j = 0; j < transformed.cols; ++j)
            {
                point[j] = transformed(i, j);
            }
            cout << point << "    " << point.projective_divide() << endl;
        }

        size_t k = 0;
        for (size_t i = 0; i < transformed.rows; ++i)
        {
            projected_type point;
            auto p = &transformed.data()[k];
            std::copy(p, p + 3, point.data());
            cout << point.projective_divide() << endl;
            k += 3;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void face_test(const mesh_type::face_type& face, const g::Projection<vector_traits>& projection)
    {
        vector<id_type> halfedge_ids;
        vector<projected_type> positions;
        vector<projected_type> texcoords;
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            halfedge_ids.push_back(halfedge.id());
            positions.push_back(projection(attributes.position()));
            texcoords.push_back(attributes.texcoord());
        }

        auto matrix = g::projective_mapping_h<projected_type>(positions, texcoords);
        for (auto [original, target] : std::ranges::zip_view(positions, texcoords))
        {
            auto transformed = original * matrix;
            transformed.projective_divide();
            cout << target << "    " << transformed << "    " << boolalpha << float_eq(transformed, target, 100) << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void quad_test(const mesh_type& mesh)
    {
        for (const auto& face : mesh.faces())
        {
            if (face.halfedge_count() == 4)
            {
                face_test(face, face.attributes().normal());
            }
        }
    }

    //--------------------------------------------------------------------------
    matrix2_type texcoord_to_position_matrix(span<const point2_type> texcoords, span<const point2_type> positions)
    {
        auto u0 = texcoords[0].x();
        auto v0 = texcoords[0].y();
        auto u1 = texcoords[1].x();
        auto v1 = texcoords[1].y();
        auto u2 = texcoords[2].x();
        auto v2 = texcoords[2].y();

        auto x0 = positions[0].x();
        auto y0 = positions[0].y();
        auto x1 = positions[1].x();
        auto y1 = positions[1].y();
        auto x2 = positions[2].x();
        auto y2 = positions[2].y();

        auto denom = u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2;

        auto a = -(v1 * x0 - v2 * x0 - v0 * x1 + v2 * x1 + v0 * x2 - v1 * x2) / denom;
        auto b = -(-u1 * x0 + u2 * x0 + u0 * x1 - u2 * x1 - u0 * x2 + u1 * x2) / denom;
        auto c = -(-u2 * v1 * x0 + u1 * v2 * x0 + u2 * v0 * x1 - u0 * v2 * x1 - u1 * v0 * x2 + u0 * v1 * x2) / denom;
        auto d = -(v1 * y0 - v2 * y0 - v0 * y1 + v2 * y1 + v0 * y2 - v1 * y2) / denom;
        auto e = -(-u1 * y0 + u2 * y0 + u0 * y1 - u2 * y1 - u0 * y2 + u1 * y2) / denom;
        auto f = -(-u2 * v1 * y0 + u1 * v2 * y0 + u2 * v0 * y1 - u0 * v2 * y1 - u1 * v0 * y2 + u0 * v1 * y2) / denom;

        return
        {
            a, d, 0.0,
            b, e, 0.0,
            c, f, 1.0,
        };
    }

    //--------------------------------------------------------------------------
    double benchmark_affine(size_t n)
    {
        vector<point2_type> from(3);
        vector<point2_type> to(3);
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            for (auto x : from) set_random_elements(x);
            for (auto x : to) set_random_elements(x);

            Timestamp ts0;
            auto a = g::affine_mapping<point2_type>(from, to);
            duration += Timestamp::now() - ts0;
        }

        return duration;
    }

    //--------------------------------------------------------------------------
    double benchmark_affine_manual(size_t n)
    {
        vector<point2_type> from(3);
        vector<point2_type> to(3);
        double duration = 0;

        for (size_t i = 0; i < n; ++i)
        {
            for (auto x : from) set_random_elements(x);
            for (auto x : to) set_random_elements(x);

            Timestamp ts0;
            auto a = texcoord_to_position_matrix(from, to);
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

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;
    //mesh_type meshBase;
    //vector_type axis;
    //g::transformation_matrix_type<vector_traits> matrix;

    name = "sphere";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 12, 6, value_type(100.0));
    model::write_obj(mesh, to_filename(name));

    const auto& face = mesh.face(50);

    g::Projection<vector_traits> projection(face.attributes().normal());
/*
    cout << "affine1" << endl;
    affine_mapping_test1(face, projection);

    cout << "affine2" << endl;
    affine_mapping_test2(face, projection);
*/
    cout << "projective_mapping_test2" << endl;
    projective_mapping_test2(face, projection);

    projection.init({face.attributes().normal(), normalize(face.halfedge().vertex_attributes().position() - face.halfedge().next().vertex_attributes().position())});
    cout << "projective_mapping_test2_oriented" << endl;
    projective_mapping_test2(face, projection);

    cout << "projective_mapping_test3" << endl;
    projective_mapping_test3(face);

    // the above do position to texcoord, check the other way too ...

    cout << "****  eigen_test" << endl;
    eigen_test(face);

    cout << "****  eigen_test2" << endl;
    eigen_test2(face);

    cout << "****  ai_test" << endl;
    ai_test(face, projection);

    cout << "****  ai2_test" << endl;
    ai2_test(face, projection);

    cout << "****  quad_test" << endl;
    quad_test(mesh);

    cout << "benchmark: " << N_BENCHMARK << " reps" << endl;
    double d = 0;

    d = benchmark_affine(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "affine", d) <<  endl;

    d = benchmark_affine_manual(N_BENCHMARK);
    cout << format("{}: {: .4f} s", "affine manual", d) <<  endl;

//    lapack_test();

    return 0;
}
