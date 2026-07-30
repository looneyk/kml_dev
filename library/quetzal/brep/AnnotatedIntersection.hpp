#if !defined(QUETZAL_BREP_ANNOTATED_INTERSECTION_HPP)
#define QUETZAL_BREP_ANNOTATED_INTERSECTION_HPP
//------------------------------------------------------------------------------
// brep
// AnnotatedIntersection.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "quetzal/geometry/Locus.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Segment.hpp"
#include <cassert>
#include <format>
#include <span>
#include <vector>

namespace quetzal::brep
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct AnnotatedPoint : geometry::Point<Traits>
    {
        using point_type = geometry::Point<Traits>;

        AnnotatedPoint() = default;
        AnnotatedPoint(const point_type& point, id_type halfedge_id) :
            point_type(point),
            m_halfedge_id(halfedge_id)
        {
        }

        bool operator==(const AnnotatedPoint&) const = default;

        id_type m_halfedge_id = nullid;
    };

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct AnnotatedSegment : geometry::Segment<Traits>
    {
        using segment_type = geometry::Segment<Traits>;

        AnnotatedSegment() = default;
        AnnotatedSegment(const segment_type& segment, id_type halfedge_id0, id_type halfedge_id1) :
            segment_type(segment),
            m_halfedge_id0(halfedge_id0),
            m_halfedge_id1(halfedge_id1)
        {
        }

        bool operator==(const AnnotatedSegment&) const = default;

        id_type m_halfedge_id0 = nullid;
        id_type m_halfedge_id1 = nullid;
    };

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct AnnotatedIntersection
    {
        geometry::Locus m_locus;
        AnnotatedPoint<Traits> m_point;
        AnnotatedSegment<Traits> m_segment;

        bool operator==(const AnnotatedIntersection&) const = default;
    };

    template<typename Traits>
    using annotated_intersections_type = std::vector<AnnotatedIntersection<Traits>>;

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::brep::AnnotatedPoint<Traits>>
{
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::brep::AnnotatedPoint<Traits>& ap, FormatContext& ctx) const
    {
        auto out = m_point_formatter.format(ap, ctx);
        return std::format_to(out, " (h {})", quetzal::brep::id_string(ap.m_halfedge_id));
    }
};

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::brep::AnnotatedSegment<Traits>>
{
    std::formatter<quetzal::geometry::Segment<Traits>> m_segment_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_segment_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::brep::AnnotatedSegment<Traits>& as, FormatContext& ctx) const
    {
        auto out = m_segment_formatter.format(as, ctx);
        return std::format_to(out, " (h {}, h {})", quetzal::brep::id_string(as.m_halfedge_id0), quetzal::brep::id_string(as.m_halfedge_id1));
    }
};

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::brep::AnnotatedIntersection<Traits>>
{
    std::formatter<quetzal::brep::AnnotatedPoint<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::brep::AnnotatedIntersection<Traits>& ai, FormatContext& ctx) const
    {
        using quetzal::geometry::Locus;

        if (ai.m_locus == Locus::Point)
        {
            return m_point_formatter.format(ai.m_point, ctx);
        }

        assert(ai.m_locus == Locus::Segment);
        std::formatter<quetzal::brep::AnnotatedSegment<Traits>> segment_formatter;
        segment_formatter.m_segment_formatter.m_point_formatter = m_point_formatter.m_point_formatter;
        return segment_formatter.format(ai.m_segment, ctx);
    }
};

#endif // QUETZAL_BREP_ANNOTATED_INTERSECTION_HPP
