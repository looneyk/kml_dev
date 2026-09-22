#if !defined(QUETZAL_PHYSICS_GJK_HPP)
#define QUETZAL_PHYSICS_GJK_HPP
//------------------------------------------------------------------------------
// physics
// gjk.hpp
//
// GJK (Gilbert-Johnson-Keerthi) intersection testing for convex BoundingVolumes of any concrete type, through each
// shape's support mapping. Three dimensional only, as are the simplex cases
//------------------------------------------------------------------------------

#include "quetzal/geometry/BoundingVolume.hpp"
#include "quetzal/math/Vector.hpp"
#include <array>
#include <cassert>

namespace quetzal::physics
{

    // Convex intersection test through support(), with no bounding box pre-cull and no closed-form tests for known pairs
    template<typename Traits> requires (Traits::dimension == 3)
    bool intersects_gjk(const geometry::BoundingVolume<Traits>& a, const geometry::BoundingVolume<Traits>& b);

} // namespace quetzal::physics

namespace quetzal::physics::gjk_detail
{

    // A simplex in Minkowski-difference space, so its points are Vectors. points[count - 1] is the newest point, A in
    // the cases below, and the others are in insertion order
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

    // Reduces the simplex to the sub-simplex closest to the origin and directs direction from it toward the origin.
    // True once a tetrahedron contains the origin, so the shapes intersect. A was extremal along the previous direction,
    // so the origin can be closest only to a feature touching A or to the interior
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
