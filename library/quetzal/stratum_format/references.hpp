#if !defined(QUETZAL_STRATUM_FORMAT_REFERENCES_HPP)
#define QUETZAL_STRATUM_FORMAT_REFERENCES_HPP
//------------------------------------------------------------------------------
// stratum_format
// references.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace quetzal::stratum_format
{

    // A reference to an entity is written { "entity": id }, and anything else is an ordinary value.
    // Finding and translating references needs no knowledge of what a file component means

    raw_values_type make_reference(stored_entity_id_type id);

    // Returns the id if the value is a reference: an object whose only key is entity, with a whole,
    // non-negative number
    std::optional<stored_entity_id_type> reference_id(const raw_values_type& v);

    // Returns every reference within an object of file components, in the order they are written
    std::vector<stored_entity_id_type> references(const raw_values_type& components);

    // Replaces each reference within an object of file components with the id translate returns for it
    void translate_references(raw_values_type& components, const std::function<stored_entity_id_type(stored_entity_id_type)>& translate);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REFERENCES_HPP
