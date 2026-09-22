#if !defined(QUETZAL_BREP_DEBUG_HPP)
#define QUETZAL_BREP_DEBUG_HPP
//------------------------------------------------------------------------------
// brep
// debug.hpp
//
// Diagnostic dumps for degenerate intersection cases. Kept apart from validation.hpp so it can depend on the connection
// types in border.hpp
//------------------------------------------------------------------------------

#include "border.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "quetzal/common/id.hpp"
#include <array>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <vector>

namespace quetzal::brep
{

    // Every halfedge with its origin at position, with the face corner it turns through and its
    // border/partner state, followed by the fan rotation next_border_halfedge_id performs from each
    // Use to check that the face corners at a vertex tile its fan without overlap
    template<typename M>
    void dump_vertex_fan(const M& mesh, const typename M::point_type& position, const std::map<id_type, std::string>& halfedge_labels = {});

    // dump_vertex_fan at each position where more than one connection trace meets, labelled by
    // trace and border slot
    template<typename M>
    void dump_multitrace_vertices(const M& mesh, const connection_traces_type& connections);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_vertex_fan(const M& mesh, const typename M::point_type& position, const std::map<id_type, std::string>& halfedge_labels)
{
    auto label = [&halfedge_labels](id_type halfedge_id) -> std::string
    {
        auto i = halfedge_labels.find(halfedge_id);
        return i != halfedge_labels.end() ? i->second : std::string("-");
    };

    std::vector<id_type> outgoing;
    for (const auto& halfedge : mesh.halfedge_store())
    {
        if (!halfedge.deleted() && vertex_position(halfedge) == position)
        {
            outgoing.push_back(halfedge.id());
        }
    }

    std::cout << std::format("vertex fan {}, {} halfedges with origin here", position, outgoing.size()) << std::endl;
    for (id_type halfedge_id : outgoing)
    {
        const auto& halfedge = mesh.halfedge(halfedge_id);
        const auto& position_tip = vertex_position(halfedge.next());
        std::cout << std::format("    h {:>3} face {:>3} (n {}) normal {} in {:>3} out {:>3} {} tip {} dir {} {}",
            halfedge_id, id_string(halfedge.face_id()), halfedge.face().halfedge_count(), halfedge.face().attributes().normal(),
            id_string(halfedge.prev_id()), id_string(halfedge.next_id()),
            halfedge.border() ? "BORDER    " : std::format("partner {:>3}", id_string(halfedge.partner_id())),
            position_tip, normalize(position_tip - position), label(halfedge_id)) << std::endl;
    }

    // The rotation next_border_halfedge_id performs, from each halfedge leaving this position
    std::cout << "    fan rotation, partner().next() until a border is reached" << std::endl;
    for (id_type halfedge_id : outgoing)
    {
        std::string chain = std::format("{}", halfedge_id);
        id_type id = halfedge_id;
        for (size_t step = 0; step <= outgoing.size(); ++step)
        {
            if (mesh.halfedge(id).border())
            {
                chain += " [BORDER]";
                break;
            }

            id = mesh.halfedge(id).partner().next_id();
            chain += std::format(" -> {}", id);
            if (id == halfedge_id)
            {
                chain += " [CLOSED]";
                break;
            }
        }

        std::cout << std::format("      from h {:>3} ({}): {}", halfedge_id, label(halfedge_id), chain) << std::endl;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_multitrace_vertices(const M& mesh, const connection_traces_type& connections)
{
    static constexpr std::array<const char*, 4> slot_names = {"a_ext", "a_int", "b_ext", "b_int"};

    std::map<id_type, std::string> halfedge_labels;
    std::vector<std::pair<typename M::point_type, std::set<size_t>>> position_traces;

    for (size_t trace = 0; trace < connections.size(); ++trace)
    {
        for (const auto& borders : connections[trace])
        {
            for (size_t slot = 0; slot < borders.size(); ++slot)
            {
                halfedge_labels[borders[slot]] = std::format("t{}.{}", trace, slot_names[slot]);

                const auto& position = vertex_position(mesh.halfedge(borders[slot]));
                auto i = std::ranges::find_if(position_traces, [&position](const auto& entry) { return entry.first == position; });
                if (i == position_traces.end())
                {
                    position_traces.emplace_back(position, std::set<size_t>{trace});
                }
                else
                {
                    i->second.insert(trace);
                }
            }
        }
    }

    for (const auto& [position, traces] : position_traces)
    {
        if (traces.size() > 1)
        {
            std::cout << std::format("\nmultitrace vertex, {} traces meet here", traces.size()) << std::endl;
            dump_vertex_fan(mesh, position, halfedge_labels);
        }
    }

    return;
}

#endif // QUETZAL_BREP_DEBUG_HPP
