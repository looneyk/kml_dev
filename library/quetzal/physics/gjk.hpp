#if !defined(QUETZAL_PHYSICS_GJK_HPP)
#define QUETZAL_PHYSICS_GJK_HPP
//------------------------------------------------------------------------------
// physics
// gjk.hpp
//
// GJK (Gilbert-Johnson-Keerthi) intersection testing for convex BoundingVolumes.
// Works for any pair of concrete BoundingVolume subtypes (AxisAlignedBoundingBox,
// BoundingSphere, ...) without either side needing to know the other's concrete
// type, since it operates entirely through each shape's support() mapping.
//
// Scoped to Traits::dimension == 3: the terminal simplex case (tetrahedron) and
// the intermediate cases (line, triangle) below are 3D-specific.
//------------------------------------------------------------------------------

#include "quetzal/geometry/BoundingVolume.hpp"
#include "quetzal/math/Vector.hpp"
#include <array>
#include <cassert>

namespace quetzal::physics
{

    // Generic convex-vs-convex intersection test via GJK, operating purely through support().
    // This is the fallback stage only: it does no AABB pre-cull and no dispatch to the cheaper
    // closed-form tests that exist for known pairs (disjoint(AABB,AABB), sphere-sphere,
    // sphere-AABB); a full intersects() would try those first and only reach this for pairs it
    // has no cheap answer for.
    template<typename Traits> requires (Traits::dimension == 3)
    bool intersects_gjk(const geometry::BoundingVolume<Traits>& a, const geometry::BoundingVolume<Traits>& b);

} // namespace quetzal::physics

namespace quetzal::physics::gjk_detail
{

    // A GJK simplex lives in Minkowski-difference space, so its points are Vectors (differences),
    // not Points (positions). points[count - 1] is always the most recently added point ("A" in
    // the Voronoi-region case analysis below); earlier points are in insertion order before that.
    template<typename Traits>
    struct Simplex
    {
        using vector_type = math::Vector<Traits>;

        std::array<vector_type, 4> points;
        size_t count = 0;

        void push(const vector_type& p)
        {
            assert(count < points.size());
            points[count++] = p;
        }
    };

    // Reduces simplex to the smallest sub-simplex closest to the origin and updates direction to
    // point from that feature toward the origin. Returns true once the simplex (a tetrahedron) is
    // found to actually contain the origin, meaning the two shapes intersect. By construction, the
    // newest point (A) was just chosen as extremal along the previous direction, so the origin can
    // never be closest to a lone vertex or to the "old" edge/face opposite A -- only to an edge or
    // face touching A, or the simplex interior -- which is why the cases below skip those regions.
    template<typename Traits>
    bool do_simplex(Simplex<Traits>& simplex, math::Vector<Traits>& direction)
    {
        using vector_type = math::Vector<Traits>;
        using value_type = typename Traits::value_type;

        if (simplex.count == 2)
        {
            const vector_type A = simplex.points[1];
            const vector_type B = simplex.points[0];
            vector_type AB = B - A;
            vector_type AO = -A;

            if (dot(AB, AO) > value_type(0))
            {
                vector_type n = cross(cross(AB, AO), AB);
                direction = n.zero() ? AO : n; // origin on line AB: fall back to AO for progress
            }
            else
            {
                simplex.points[0] = A;
                simplex.count = 1;
                direction = AO;
            }

            return false;
        }

        if (simplex.count == 3)
        {
            const vector_type A = simplex.points[2];
            const vector_type B = simplex.points[1];
            const vector_type C = simplex.points[0];
            vector_type AB = B - A;
            vector_type AC = C - A;
            vector_type AO = -A;
            vector_type ABC = cross(AB, AC);

            if (dot(cross(ABC, AC), AO) > value_type(0))
            {
                if (dot(AC, AO) > value_type(0))
                {
                    simplex.points[0] = C;
                    simplex.points[1] = A;
                    simplex.count = 2;
                    return do_simplex(simplex, direction);
                }

                simplex.points[0] = B;
                simplex.points[1] = A;
                simplex.count = 2;
                return do_simplex(simplex, direction);
            }

            if (dot(cross(AB, ABC), AO) > value_type(0))
            {
                simplex.points[0] = B;
                simplex.points[1] = A;
                simplex.count = 2;
                return do_simplex(simplex, direction);
            }

            // Origin projects inside the triangle -- decide which side of it to search
            if (dot(ABC, AO) > value_type(0))
            {
                simplex.points[0] = C;
                simplex.points[1] = B;
                simplex.points[2] = A;
                direction = ABC;
            }
            else
            {
                simplex.points[0] = B;
                simplex.points[1] = C;
                simplex.points[2] = A;
                direction = -ABC;
            }

            return false;
        }

        // simplex.count == 4: tetrahedron ABCD
        const vector_type A = simplex.points[3];
        const vector_type B = simplex.points[2];
        const vector_type C = simplex.points[1];
        const vector_type D = simplex.points[0];
        vector_type AB = B - A;
        vector_type AC = C - A;
        vector_type AD = D - A;
        vector_type AO = -A;

        vector_type ABC = cross(AB, AC);
        vector_type ACD = cross(AC, AD);
        vector_type ADB = cross(AD, AB);

        if (dot(ABC, AO) > value_type(0))
        {
            simplex.points[0] = C;
            simplex.points[1] = B;
            simplex.points[2] = A;
            simplex.count = 3;
            return do_simplex(simplex, direction);
        }

        if (dot(ACD, AO) > value_type(0))
        {
            simplex.points[0] = D;
            simplex.points[1] = C;
            simplex.points[2] = A;
            simplex.count = 3;
            return do_simplex(simplex, direction);
        }

        if (dot(ADB, AO) > value_type(0))
        {
            simplex.points[0] = B;
            simplex.points[1] = D;
            simplex.points[2] = A;
            simplex.count = 3;
            return do_simplex(simplex, direction);
        }

        return true; // origin is inside all three new faces -- inside the tetrahedron
    }

} // namespace quetzal::physics::gjk_detail

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
bool quetzal::physics::intersects_gjk(const geometry::BoundingVolume<Traits>& a, const geometry::BoundingVolume<Traits>& b)
{
    using vector_type = math::Vector<Traits>;
    using value_type = typename Traits::value_type;

    auto minkowski_support = [&a, &b](const vector_type& d) -> vector_type
    {
        return a.support(d) - b.support(-d);
    };

    vector_type direction = a.centroid() - b.centroid();
    if (direction.zero())
    {
        direction = vector_type::x_direction();
    }

    gjk_detail::Simplex<Traits> simplex;
    simplex.push(minkowski_support(direction));
    direction = -simplex.points[0];

    constexpr int max_iterations = 32;
    for (int iteration = 0; iteration < max_iterations; ++iteration)
    {
        vector_type p = minkowski_support(direction);
        if (dot(p, direction) < value_type(0))
        {
            return false;
        }

        simplex.push(p);

        if (gjk_detail::do_simplex(simplex, direction))
        {
            return true;
        }
    }

    return false; // failed to converge -- treat conservatively as non-intersecting
}

#endif // QUETZAL_PHYSICS_GJK_HPP
