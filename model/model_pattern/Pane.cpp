//------------------------------------------------------------------------------
// model_pattern
// Pane.cpp
//------------------------------------------------------------------------------

#include "Pane.hpp"

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
Pane::Pane(id_type id, const polygon_type& polygon, const vector_type& direction, size_t nSection, size_t nSections) :
    m_polygon(polygon),
    m_bounds(pane_bounds(polygon)),
    m_direction(direction),
    m_references{id},
    m_nSection(nSection),
    m_nSections(nSections)
{
}

//------------------------------------------------------------------------------
const typename Pane::polygon_type& Pane::polygon() const
{
    return m_polygon;
}

//------------------------------------------------------------------------------
const geometry::AxisAlignedBoundingBox<typename Pane::traits_type>& Pane::bounds() const
{
    return m_bounds;
}

//------------------------------------------------------------------------------
const typename Pane::vector_type& Pane::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
const typename Pane::vector_type Pane::size() const
{
    return m_bounds.extent();
}

//------------------------------------------------------------------------------
double Pane::area() const
{
    auto extent = m_bounds.extent();
    return extent.x() * extent.y();
}

//------------------------------------------------------------------------------
size_t Pane::section_count() const
{
    return m_nSections;
}

//------------------------------------------------------------------------------
size_t Pane::section() const
{
    return m_nSection;
}

//------------------------------------------------------------------------------
size_t Pane::reference_count() const
{
    return m_references.size();
}

//------------------------------------------------------------------------------
const typename Pane::references_type& Pane::references() const
{
    return m_references;
}

//------------------------------------------------------------------------------
void Pane::insert_reference(id_type id)
{
    m_references.push_back(id);
    return;
}

//------------------------------------------------------------------------------
geometry::AxisAlignedBoundingBox<Pane::traits_type> quetzal::pane_bounds(const Pane::polygon_type& polygon)
{
    geometry::AxisAlignedBoundingBox<Pane::traits_type> aabb;

    //aabb.insert(polygon.vertices());

    for (const auto& vertex : polygon.vertices())
    {
        aabb.insert(vertex);
    }

    return aabb;
}

//------------------------------------------------------------------------------
bool quetzal::float_eq(const Pane& lhs, const Pane& rhs)
{
    if (lhs.polygon().vertex_count() != rhs.polygon().vertex_count())
    {
        return false;
    }

    if (!float_eq(lhs.direction(), rhs.direction()))
    {
        return false;
    }

    double epsilon = 0.01;
    size_t n = lhs.polygon().vertex_count();
    size_t j = nullid;

    for (size_t i = 0; i < n; ++i)
    {
        if (epsilon_eq(rhs.polygon().vertex(i), lhs.polygon().vertex(0), epsilon))
        {
            j = i;
        }
    }

    if (j == nullid)
    {
        return false;
    }

    for (size_t i = 0; i < n; ++i)
    {
        const auto& va = lhs.polygon().vertex(i);
        const auto& vb = rhs.polygon().vertex((j + i) % n);

        //        if (!float_eq(va.position(), vb.position(), 1024) || !float_eq(va.texcoord(), vb.texcoord(), 1024))
        //        if (!float_eq(va.position(), vb.position(), 1024))
        if (!epsilon_eq(va, vb, epsilon))
        {
            return false;
        }
    }

    // split panes may introduce additional complexity here ...

    return true;
}
