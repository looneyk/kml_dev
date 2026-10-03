//------------------------------------------------------------------------------
// stratum_sim
// Character.cpp
//------------------------------------------------------------------------------

#include "Character.hpp"
#include "DisplayName.hpp"
#include "load_character.hpp"
#include "make_document.hpp"
#include "quetzal/stratum_format/CharacterDocument.hpp"
#include "quetzal/stratum_format/read_character.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/write_character.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <cassert>

namespace
{

    static constexpr int character_speed = 100;

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Character::Character(World& world) :
    m_world(world),
    m_entity()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::clear()
{
    // There is no character before the first game is created
    if (m_entity)
    {
        m_entity.destruct();
        m_entity = flecs::entity();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::create(const CharacterSpec& character_spec)
{
    assert(!m_entity);

    m_entity = m_world.entities().entity().add<Blocking>().add<ControlledActor>().set<Speed>(Speed {character_speed})
        .set<PersistentId>(m_world.next_id());

    if (!character_spec.name.empty())
    {
        m_entity.set<DisplayName>(DisplayName {character_spec.name});
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::CharacterPlacement quetzal::stratum_sim::Character::open(const std::filesystem::path& game_directory)
{
    assert(!m_entity);

    std::filesystem::path filepath = game_directory / stratum_format::character_filename;
    stratum_format::CharacterDocument document = stratum_format::read_character(filepath);

    if (std::optional<std::string> error = check_buildable(m_world.entities(), document))
    {
        throw std::runtime_error(std::format("{}: {}", filepath.string(), *error));
    }

    m_entity = m_world.entities().entity().add<ControlledActor>();
    load_character(m_entity, document);

    return CharacterPlacement {PersistentId {document.zone_id}, document.placement.position(), document.placement.direction()};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::save(const std::filesystem::path& game_directory) const
{
    assert(m_entity);

    stratum_format::write(game_directory / stratum_format::character_filename, make_character_document(m_entity));
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Character::entity() const
{
    return m_entity;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::set_intent(const Intent& intent)
{
    m_entity.set<Intent>(intent);
    return;
}
