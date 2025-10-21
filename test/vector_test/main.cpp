//------------------------------------------------------------------------------
// vector_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/math/Vector.hpp"
#include "quetzal/math/VectorTraits.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include <vector>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using TraitsF2 = math::VectorTraits<float, 2>;
    using TraitsF3 = math::VectorTraits<float, 3>;
    using TraitsF4 = math::VectorTraits<float, 4>;

    using VectorF2 = math::Vector<TraitsF2>;
    using VectorF3 = math::Vector<TraitsF3>;
    using VectorF4 = math::Vector<TraitsF4>;

    using Normal = geometry::Normal<TraitsF3>;

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    float f = 0.0f;
    bool b = false;

    VectorF2 v2a(1.0f, 2.0f);
    VectorF2 v2b(1.0f, 2.0f, 3.0f); // error
    VectorF2 v2c(1.0f, 2.0f, 3.0f, 4.0f); // error

    VectorF3 v3a(1.0f, 2.0f); // error
    VectorF3 v3b(1.0f, 2.0f, 3.0f);
    VectorF3 v3c(1.0f, 2.0f, 3.0f, 4.0f); // error

    VectorF4 v4a(1.0f, 2.0f); // error
    VectorF4 v4b(1.0f, 2.0f, 3.0f); // error
    VectorF4 v4c(1.0f, 2.0f, 3.0f, 4.0f);

    VectorF2 v2d{1.0f, 2.0f};
    VectorF2 v2e{1.0f, 2.0f, 3.0f}; // error
    VectorF2 v2f{1.0f, 2.0f, 3.0f, 4.0f}; // error

    VectorF3 v3d{1.0f, 2.0f}; // error
    VectorF3 v3e{1.0f, 2.0f, 3.0f};
    VectorF3 v3f{1.0f, 2.0f, 3.0f, 4.0f}; // error

    VectorF4 v4d{1.0f, 2.0f}; // error
    VectorF4 v4e{1.0f, 2.0f, 3.0f}; // error
    VectorF4 v4f{1.0f, 2.0f, 3.0f, 4.0f};

    VectorF2 v2g = {1.0f, 2.0f};
    VectorF2 v2h = {1.0f, 2.0f, 3.0f}; // error
    VectorF2 v2i = {1.0f, 2.0f, 3.0f, 4.0f}; // error

    VectorF3 v3g = {1.0f, 2.0f}; // error
    VectorF3 v3h = {1.0f, 2.0f, 3.0f};
    VectorF3 v3i = {1.0f, 2.0f, 3.0f, 4.0f}; // error

    VectorF4 v4g = {1.0f, 2.0f}; // error
    VectorF4 v4h = {1.0f, 2.0f, 3.0f}; // error
    VectorF4 v4i = {1.0f, 2.0f, 3.0f, 4.0f};

    v2g = {1.0f, 2.0f};
    v2g = {1.0f, 2.0f, 3.0f}; // error
    v2g = {1.0f, 2.0f, 3.0f, 4.0f}; // error

    v3h = {1.0f, 2.0f}; // error
    v3h = {1.0f, 2.0f, 3.0f};
    v3h = {1.0f, 2.0f, 3.0f, 4.0f}; // error

    v4i = {1.0f, 2.0f}; // error
    v4i = {1.0f, 2.0f, 3.0f}; // error
    v4i = {1.0f, 2.0f, 3.0f, 4.0f};

    v2a.set_x(0.0f);
    v2a.set_y(0.0f);
    v2a.set_z(0.0f); // error
    v2a.set_w(0.0f); // error

    v3b.set_x(0.0f);
    v3b.set_y(0.0f);
    v3b.set_z(0.0f);
    v3b.set_w(0.0f); // error

    v4c.set_x(0.0f);
    v4c.set_y(0.0f);
    v4c.set_z(0.0f);
    v4c.set_w(0.0f);

    vector<VectorF3> vv =
    {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };

    VectorF3 va = {1.0f, 2.0f, 3.0f};
    VectorF3 vb = {4.0f, 5.0f, 6.0f};
    VectorF3 vc = va;
    VectorF3 vd = va + vb;
    VectorF3 ve = vb - va;
    VectorF3 vf = va * 2.0f;
    VectorF3 vg = 2.0f * va;
    VectorF3 vh = va / 2.0f;
    VectorF3 vi = {5.0f, 7.0f, 9.0f};

    b = (va + vb == vi);
    b = vf == vg;
    b = vf != vg;

    f = va[0];
    f = va[1];
    f = va[2];
    f = va[3]; // runtime error

    VectorF3 vj = normalize(va);
    f = dot(va, vb);
    VectorF3 vl = cross(va, vb);
    f = cross(va, vb).norm();

    assert(va + vb == vi);
//    assert(va + vb == {5.0f, 7.0f, 9.0f});
//    assert(vb - va == {3.0f, 3.0f, 3.0f});

    // why doesn't vector work for Normal initialization?
    // too far removed

    std::vector<float> v{1.0f, 0.0f, 0.0f};
    std::array<float, 3> a{1.0f, 0.0f, 0.0f};
    float c[3] = {1.0f, 0.0f, 0.0f};
    std::span<float> s1(v);
    std::span<float> s2(a);
    std::span<float> s3(c);
    VectorF3 v0a(1.0f, 0.0f, 0.0f);
    VectorF3 v0b{1.0f, 0.0f, 0.0f};
    VectorF3 v0c({1.0f, 0.0f, 0.0f});
    VectorF3 v1(v);
    VectorF3 v2(s1);
    VectorF3 v3(a);
    VectorF3 v4(c);
    Normal fa0({1.0f, 0.0f, 0.0f});
    Normal fa0a{{1.0f, 0.0f, 0.0f}};
    Normal fa1 = {{1.0f, 0.0f, 0.0f}};
    Normal fa2(v1);
    Normal fa2a{v1};
    Normal fa3 = {v1};
    Normal fa3a = v1;
    Normal fa4(s1);
    Normal fa4a{s1};
    Normal fa5 = {s1};
    //Normal fa5a = s1;
    Normal fa6(v);
    Normal fa6a{v};
    Normal fa7 = {v};
    //Normal fa7a = v;
    Normal fa8(a);
    Normal fa8a{a};
    Normal fa9 = {a};
    //Normal fa9a = a;
    Normal fa10(c);
    Normal fa10a{c};
    Normal fa11 = {c};
    //Normal fa11a = c;

    return 0;
}
