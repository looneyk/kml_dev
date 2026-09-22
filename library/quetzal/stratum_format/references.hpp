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

    values_type make_reference(stored_entity_id_type id);

    // The id where the value is a reference: an object whose only key is entity, with a whole,
    // non-negative number
    std::optional<stored_entity_id_type> reference_id(const values_type& v);

    // Every reference within an object of file components, in the order they are written
    std::vector<stored_entity_id_type> references(const values_type& components);

    // Each reference within an object of file components replaced by what translate returns for it
    void translate_references(values_type& components, const std::function<stored_entity_id_type(stored_entity_id_type)>& translate);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REFERENCES_HPP
