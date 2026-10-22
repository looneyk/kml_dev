//------------------------------------------------------------------------------
// stratum_sim_test
// game.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscretePlacementIndex.hpp"
#include "quetzal/stratum_sim/DisplayName.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Config.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/ai.hpp"
#include "quetzal/stratum_sim/PersistentSettings.hpp"
#include "quetzal/stratum_sim/setting_definitions.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/file_components.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;

namespace
{

    TEST_SUITE_BEGIN("game");

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "game")
    {
        const stratum_sim::Config config {"out/elsewhere/saves", "out/elsewhere/working", 3};
        stratum_sim::Game game(config);

        CHECK_CASE("a game keeps the config it was constructed with", game.config().saves_directory == config.saves_directory
            && game.config().zone_count_max == config.zone_count_max);
        CHECK_CASE("and its world keeps no more zones loaded than the config's maximum", game.world().zone_count_max() == 3);

        // A game played in two regions, with a character carrying something, then cleared
        stratum_sim::Game played(g_test_config);
        stratum_sim::World& world = played.world();
        stratum_sim::Session& session = played.session();
        played.generation() = test_generation();

        enter_zone_file(played, "data/corridor.zone");
        flecs::entity character = session.character().entity();
        stratum_sim::PersistentId character_id = character.get<stratum_sim::PersistentId>();
        flecs::entity carried = world.entities().entity();
        stratum_sim::set_parent(carried, character);

        flecs::entity elsewhere = world.create_zone(world.create_region(test_region_spec("door")), stratum_sim::ZoneSpec {});
        world.make_reference(world.create_entity(elsewhere, stratum_sim::placement_type()));

        played.clear();

        CHECK_CASE("clearing a game ends play", !session.loaded() && session.scheduler().count() == 0);
        CHECK_CASE("deletes the character and what it carries", !session.character().entity() && !character.is_alive()
            && !carried.is_alive() && !world.entity(character_id));
        CHECK_CASE("and every region and zone", world.entities().count<stratum_sim::Region>() == 0
            && world.entities().count<stratum_sim::Zone>() == 0 && world.zone_count() == 0);
        CHECK_CASE("leaving no entity with a persistent id", world.entities().count<stratum_sim::PersistentId>() == 0);
        CHECK_CASE("and keeps the registrations", stratum_sim::entity_type(world.entities(), "door")
            && stratum_sim::access_type(world.entities(), "staircase") && stratum_sim::file_component(world.entities(), "name") != 0
            && played.generation().m_generator_selectors.contains("door"));

        // One game after another, each created from specs. The games' directories are made afresh on every run
        filesystem::remove_all(g_test_config.saves_directory);
        filesystem::remove_all(g_test_config.working_directory);
        stratum_sim::Game sequence(g_test_config);
        stratum_sim::Session& sequence_session = sequence.session();
        stratum_sim::World& sequence_world = sequence.world();
        sequence.generation() = test_generation();
        flecs::entity staircase_type = stratum_sim::access_type(sequence_world.entities(), "staircase");

        const stratum_sim::RegionSpec region_spec {{string(stratum_sim::generator_selector_key), "staircase"},
            {string(stratum_sim::entry_access_key), "staircase,1"}, {format("{}{}", stratum_sim::zone_prefix, stratum_sim::level_key), "-1"}};

        auto persistent_id_of = [](flecs::entity e) { return e.get<stratum_sim::PersistentId>().id; };
        auto name_of = [](flecs::entity e) { return e.has<stratum_sim::DisplayName>() ? e.get<stratum_sim::DisplayName>().name : string(); };

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Ailis"});
        flecs::entity first_character = sequence_session.character().entity();
        flecs::entity first_zone = sequence_session.zone();
        flecs::entity first_region = stratum_sim::region_of(first_zone);

        CHECK_CASE("creating a game makes its directory in the working directory, named by the character",
            filesystem::is_directory(g_test_config.working_directory / "Ailis"));
        CHECK_CASE("and the character, with the spec's name and the first id", persistent_id_of(first_character) == 0
            && name_of(first_character) == "Ailis");
        CHECK_CASE("and starts play in the first zone of a region made from the spec, at the entry point it states",
            sequence_session.loaded() && first_zone.get<stratum_sim::ZoneLevel>().level == -1
            && first_region.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && stratum_sim::placement_of(first_character).position()
                == stratum_sim::entry_point(first_zone, staircase_type, 1).get<stratum_sim::AccessPoint>().placement.position());

        flecs::entity second_zone = stratum_sim::zone_of(sequence_world.relocate_through(first_character,
            stratum_sim::entry_point(first_zone, staircase_type, -1)));
        CHECK_CASE("the first game is played into a second zone", second_zone && second_zone != first_zone && sequence_world.zone_count() == 2);

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Bran"});
        flecs::entity second_character = sequence_session.character().entity();

        CHECK_CASE("creating another game leaves none of the first game's regions, zones or character", !first_region.is_alive()
            && !first_zone.is_alive() && !second_zone.is_alive() && !first_character.is_alive()
            && sequence_world.entities().count<stratum_sim::Region>() == 1 && sequence_world.zone_count() == 1);
        CHECK_CASE("nor its directory, as the working directory has only the game in progress",
            !filesystem::exists(g_test_config.working_directory / "Ailis") && filesystem::is_directory(g_test_config.working_directory / "Bran"));

        stratum_sim::PersistentId::type persistent_id_max = 0;
        sequence_world.entities().each([&](flecs::entity, const stratum_sim::PersistentId& persistent_id)
        {
            persistent_id_max = max(persistent_id_max, persistent_id.id);
        });

        CHECK_CASE("and its ids count from zero again, every one of them the new game's", persistent_id_of(second_character) == 0
            && name_of(second_character) == "Bran"
            && persistent_id_max + 1 == static_cast<stratum_sim::PersistentId::type>(sequence_world.entities().count<stratum_sim::PersistentId>()));

        sequence.quit();
        sequence.menu_open();
        CHECK_CASE("opening the menu with no game in progress pauses nothing", !sequence_session.paused());

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        sequence.menu_open();
        bool paused_on_open = sequence_session.paused();
        sequence.menu_close();
        CHECK_CASE("opening the menu pauses play, and closing it resumes play", paused_on_open && !sequence_session.paused());

        sequence.menu_open();
        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        CHECK_CASE("a new game chosen from the menu is not paused", sequence_session.loaded() && !sequence_session.paused());

        // The game's end
        stratum_sim::EventQueue end_events;
        sequence_session.advance(end_events);
        CHECK_CASE("a game with its character alive is not over", !sequence_session.over());

        flecs::entity dying = sequence_session.character().entity();
        dying.add<stratum_sim::Dead>();
        long long elapsed_before = sequence_session.scheduler().elapsed();
        end_events.clear();
        id_type after_death = sequence_session.advance(end_events);
        const stratum_sim::Died* died = !end_events.events().empty() ? end_events.events().back().try_get<stratum_sim::Died>() : nullptr;

        CHECK_CASE("once the character is dead the game is over", sequence_session.over());
        CHECK_CASE("and advancing runs no actor, reports the death, and returns no actor", after_death == nullid
            && sequence_session.scheduler().elapsed() == elapsed_before && end_events.events().size() == 1 && died != nullptr
            && died->entity_id == static_cast<id_type>(dying.id()));

        {
            stratum_sim::CountingIdAllocator clock_ids(stratum_sim::PersistentId {});
            TestWorld clock_world(clock_ids);
            stratum_sim::Scheduler scheduler;
            stratum_sim::Clock clock(clock_world, scheduler);
            flecs::entity controlled = clock_world.entities().entity().add<stratum_sim::Dead>();
            flecs::entity other = clock_world.entities().entity();
            scheduler.add(other.id(), 100);
            scheduler.add(controlled.id(), 100);
            stratum_sim::EventQueue clock_events;

            CHECK_CASE("a turn-based clock stops at once when the controlled actor is dead",
                clock.advance(clock_events, static_cast<id_type>(controlled.id())) == nullid && scheduler.elapsed() == 0);
        }

        sequence.quit();
        CHECK_CASE("a game quit is not over", !sequence_session.over());

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        CHECK_CASE("and a game created after it is not over", !sequence_session.over());

        sequence.quit();
        CHECK_CASE("quitting leaves no game in progress", !sequence_session.loaded() && !sequence_session.character().entity()
            && sequence_world.entities().count<stratum_sim::Region>() == 0);

        stratum_sim::RegionSpec unregistered_entry = region_spec;
        unregistered_entry.insert(string(stratum_sim::entry_access_key), "trapdoor,1");
        bool unregistered_thrown = false;

        try
        {
            sequence.create(unregistered_entry, stratum_sim::CharacterSpec {.name = "Dai"});
        }
        catch (const runtime_error&)
        {
            unregistered_thrown = true;
        }

        CHECK_CASE("an entry access point type that is not registered is an error, which leaves no game and no directory",
            unregistered_thrown && !sequence_session.loaded() && sequence_world.entities().count<stratum_sim::Region>() == 0
            && !filesystem::exists(g_test_config.working_directory / "Dai"));

        // Saving zones, with a maximum of one zone loaded so that leaving a zone and trimming unloads it
        const stratum_sim::Config saving_config {"out/saving/saves", "out/saving/working", 1};
        filesystem::remove_all("out/saving");
        stratum_sim::Game saving(saving_config);
        saving.generation() = test_generation();
        stratum_sim::CharacterSpec ailis;
        ailis.name = "Ailis";
        saving.create(region_spec, ailis);

        stratum_sim::World& saving_world = saving.world();
        flecs::entity saving_first = saving.session().zone();
        stratum_sim::PersistentId saving_first_id = saving_first.get<stratum_sim::PersistentId>();
        filesystem::path zones_directory = saving_config.working_directory / "Ailis" / "zones";
        auto zone_file = [&](stratum_sim::PersistentId zone_id) { return zones_directory / format("{}.zone", zone_id.id); };

        auto pursuers_in = [&](flecs::entity zone)
        {
            size_t count = 0;
            saving_world.entities().each([&](flecs::entity e, const stratum_sim::Pursue&) { count += stratum_sim::zone_of(e) == zone ? 1 : 0; });
            return count;
        };

        flecs::entity saving_character = saving.character().entity();
        size_t first_pursuer_count = pursuers_in(saving_first);
        flecs::entity first_down = stratum_sim::entry_point(saving_first, staircase_type, -1);
        stratum_sim::PersistentId first_down_id = first_down.get<stratum_sim::PersistentId>();
        flecs::entity second_up = saving_world.relocate_through(saving_character, first_down);
        stratum_sim::PersistentId second_up_id = second_up.get<stratum_sim::PersistentId>();
        flecs::entity saving_second = stratum_sim::zone_of(second_up);
        CHECK_CASE("leaving a zone does not save it", saving_second && !filesystem::exists(zone_file(saving_first_id)));

        saving_world.trim_zones(saving_second);
        bool saved_back = false;

        if (filesystem::exists(zone_file(saving_first_id)))
        {
            stratum_format::Document saved_document = stratum_format::read_zone(zone_file(saving_first_id));
            saved_back = saved_document.zone_id == saving_first_id.id && saved_document.level == -1;
        }

        CHECK_CASE("a zone the world unloads is saved first, as a saved zone in the game directory's zones",
            saved_back && !saving_first.is_alive() && saving_world.zone_count() == 1);

        // Back up through the staircase bound to the first zone's, which has unloaded
        stratum_sim::PersistentId saving_second_id = saving_second.get<stratum_sim::PersistentId>();
        flecs::entity back_up = saving_world.relocate_through(saving_character, second_up);
        flecs::entity first_again = back_up ? stratum_sim::zone_of(back_up) : flecs::entity();
        flecs::entity saving_region = first_again ? stratum_sim::region_of(first_again) : flecs::entity();

        CHECK_CASE("going through a relocator bound to one in a zone that has unloaded reads the zone back and arrives there",
            first_again && first_again.get<stratum_sim::PersistentId>() == saving_first_id && back_up.get<stratum_sim::PersistentId>() == first_down_id
            && stratum_sim::zone_of(saving_character) == first_again);
        CHECK_CASE("a zone read back keeps its denizens and is not populated again, and its region lists it once", first_again
            && pursuers_in(first_again) == first_pursuer_count && saving_region.get<stratum_sim::Region>().zone_ids.size() == 2);

        saving_world.trim_zones(first_again);
        flecs::entity down_again = saving_world.relocate_through(saving_character, back_up);
        CHECK_CASE("and the zone left, unloaded in its turn, is read back going the other way", down_again
            && down_again.get<stratum_sim::PersistentId>() == second_up_id && stratum_sim::zone_of(down_again).get<stratum_sim::PersistentId>() == saving_second_id);

        // Saving the game, with the first zone unloaded and the second, which the character is in, loaded. The test case moved the
        // character through the world, so the session is started where the character is
        saving.session().start(down_again);
        filesystem::path game_directory = saving_config.working_directory / "Ailis";
        string save_name = saving.save(stratum_sim::SaveType::Explicit);
        filesystem::path save_directory = saving_config.saves_directory / save_name;

        CHECK_CASE("a save is a directory named by the character's name and the time of the save", save_name.starts_with("Ailis_")
            && save_name.size() == string("Ailis_20261002T120000").size() && filesystem::is_directory(save_directory));

        auto read_json = [](const filesystem::path& filepath)
        {
            ifstream is(filepath, ios::binary);
            return stratum_format::raw_values_type::parse(is, nullptr, false);
        };

        bool identical = true;
        for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(game_directory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            filesystem::path saved_path = save_directory / filesystem::relative(entry.path(), game_directory);
            ifstream in_game(entry.path(), ios::binary);
            ifstream in_save(saved_path, ios::binary);
            identical = identical && in_save && string(istreambuf_iterator<char>(in_game), {}) == string(istreambuf_iterator<char>(in_save), {});
        }

        CHECK_CASE("the save is a copy of the game directory, file for file", identical);

        stratum_format::raw_values_type game_json = read_json(save_directory / "game.json");
        stratum_sim::PersistentId largest_id = max({saving_character.get<stratum_sim::PersistentId>(), saving_first_id, saving_second_id,
            saving_region.get<stratum_sim::PersistentId>()});
        CHECK_CASE("game.json has the next id, past every id in the game, the seed, and the gameplay random engine's state",
            !game_json.is_discarded() && game_json["format"] == "quetzal.stratum_game" && game_json["game"]["next_id"].get<uint64_t>() > largest_id.id
            && !game_json["game"]["seed"].get<string>().empty() && !game_json["game"]["random_engines"]["gameplay"].get<string>().empty());

        stratum_format::raw_values_type character_json = read_json(save_directory / "character.json");
        const stratum_sim::placement_type& character_placement = stratum_sim::placement_of(saving_character);
        stratum_sim::discrete_point_type character_cell = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(saving_world.layout(
            stratum_sim::zone_of(saving_character))).cell_position_at(character_placement.position());
        CHECK_CASE("character.json has the character's id, components, zone and cell, and an empty inventory", !character_json.is_discarded()
            && character_json["character"]["id"] == saving_character.get<stratum_sim::PersistentId>().id
            && character_json["character"]["components"].contains("speed") && character_json["character"]["zone"] == saving_second_id.id
            && character_json["character"]["placement"]["cell_position"] == stratum_format::raw_values_type {character_cell.i(), character_cell.j()}
            && character_json["character"]["inventory"].empty());

        stratum_format::raw_values_type world_json = read_json(save_directory / "world.json");
        bool zones_listed = !world_json.is_discarded() && world_json["world"]["regions"].size() == 1;
        if (zones_listed)
        {
            const stratum_format::raw_values_type& region_json = world_json["world"]["regions"][0];
            zones_listed = region_json["id"] == saving_region.get<stratum_sim::PersistentId>().id && region_json["zones"].size() == 2
                && region_json["zones"][0]["id"] == saving_first_id.id && region_json["zones"][1]["id"] == saving_second_id.id
                && region_json["spec"].contains(string(stratum_sim::generator_selector_key));

            for (const auto& zone_json : region_json["zones"])
            {
                zones_listed = zones_listed && filesystem::exists(save_directory / zone_json["file"].get<string>());
            }
        }

        CHECK_CASE("world.json lists the region and its zones, each with its file in the save, loaded or not", zones_listed);

        auto indexed_in = [&](stratum_sim::PersistentId entity_id, stratum_sim::PersistentId zone_id)
        {
            return ranges::any_of(world_json["world"]["entity_zones"], [&](const stratum_format::raw_values_type& entry)
            {
                return entry[0] == entity_id.id && entry[1] == zone_id.id;
            });
        };

        CHECK_CASE("and the world index, with each bound staircase in its zone", !world_json.is_discarded()
            && indexed_in(first_down_id, saving_first_id) && indexed_in(second_up_id, saving_second_id));

        string second_save_name = saving.save(stratum_sim::SaveType::Explicit);
        CHECK_CASE("saving again makes another save and keeps the first, waiting for the next second if needed", second_save_name != save_name
            && filesystem::is_directory(save_directory) && filesystem::is_directory(saving_config.saves_directory / second_save_name));

        string auto_save_name = saving.save(stratum_sim::SaveType::Auto);
        string quick_save_name = saving.save(stratum_sim::SaveType::Quick);
        CHECK_CASE("an auto save and a quick save have their type appended to their names", auto_save_name.starts_with("Ailis_")
            && auto_save_name.ends_with("_autosave") && quick_save_name.starts_with("Ailis_") && quick_save_name.ends_with("_quicksave")
            && filesystem::is_directory(saving_config.saves_directory / auto_save_name)
            && filesystem::is_directory(saving_config.saves_directory / quick_save_name));

        saving_character.set<stratum_sim::DisplayName>(stratum_sim::DisplayName {"Ailis: of the \"Downs\"?"});
        string replaced_save_name = saving.save(stratum_sim::SaveType::Explicit);
        CHECK_CASE("a character in the name that cannot be in a directory name is replaced in the save's", replaced_save_name.starts_with("Ailis_ of the _Downs__")
            && filesystem::is_directory(saving_config.saves_directory / replaced_save_name));

        // Opening the first save, made with the first zone unloaded and the character in the second
        vector<stratum_sim::SavedGameDetails> saves = saving.saved_games();
        CHECK_CASE("the saved games are every save, newest first, each with its character's name as entered", saves.size() == 5
            && saves.front().save_name() == replaced_save_name && saves.front().character_name() == "Ailis: of the \"Downs\"?"
            && ranges::any_of(saves, [&](const auto& details) { return details.save_name() == save_name && details.character_name() == "Ailis"; })
            && ranges::is_sorted(saves, ranges::greater(), &stratum_sim::SavedGameDetails::time));

        auto save_type_of = [&](const string& name)
        {
            auto details = ranges::find(saves, name, &stratum_sim::SavedGameDetails::save_name);
            return details != saves.end() ? optional<stratum_sim::SaveType>(details->save_type()) : nullopt;
        };

        CHECK_CASE("each has its save type", save_type_of(save_name) == stratum_sim::SaveType::Explicit
            && save_type_of(auto_save_name) == stratum_sim::SaveType::Auto && save_type_of(quick_save_name) == stratum_sim::SaveType::Quick);

        stratum_sim::PersistentId saving_character_id = saving_character.get<stratum_sim::PersistentId>();
        stratum_sim::PersistentId region_id = saving_region.get<stratum_sim::PersistentId>();
        string saved_random_engine_state = game_json["game"]["random_engines"]["gameplay"].get<string>();
        stratum_sim::vector_type character_direction = character_placement.direction();

        saving.open(save_name);
        stratum_sim::Session& opened = saving.session();
        flecs::entity opened_character = saving.character().entity();
        flecs::entity opened_zone = opened.zone();

        CHECK_CASE("opening a save copies it into the working directory as the game directory, named by the character",
            filesystem::is_directory(game_directory) && filesystem::exists(game_directory / "world.json"));
        CHECK_CASE("and plays in the zone the character was saved in, at its cell, facing as it faced", opened.loaded()
            && opened_zone.get<stratum_sim::PersistentId>() == saving_second_id && zone_layout(saving_world, opened_zone)
                .cell_position_at(stratum_sim::placement_of(opened_character).position()) == character_cell
            && stratum_sim::placement_of(opened_character).direction() == character_direction);
        CHECK_CASE("with the character's id, name and components, controlled by the player",
            opened_character.get<stratum_sim::PersistentId>() == saving_character_id && opened_character.get<stratum_sim::DisplayName>().name == "Ailis"
            && opened_character.has<stratum_sim::Speed>() && opened_character.has<stratum_sim::Blocking>()
            && opened_character.has<stratum_sim::ControlledActor>());

        flecs::entity opened_region = saving_world.entity(region_id);
        CHECK_CASE("and the region, with its spec and its zone ids, the first zone not loaded", opened_region
            && opened_region.get<stratum_sim::Region>().zone_ids == vector {saving_first_id, saving_second_id}
            && opened_region.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && !saving_world.entity(saving_first_id) && saving_world.zone_count() == 1);
        CHECK_CASE("and ids continue past every id in the game", saving_world.next_id().id > largest_id.id);

        flecs::entity opened_second_up = saving_world.entity(second_up_id);
        flecs::entity up_again = opened_second_up ? saving_world.relocate_through(opened_character, opened_second_up) : flecs::entity();
        CHECK_CASE("going up through the staircase bound to one in the zone not loaded reads that zone back, through the world index",
            up_again && up_again.get<stratum_sim::PersistentId>() == first_down_id
            && stratum_sim::zone_of(up_again).get<stratum_sim::PersistentId>() == saving_first_id);

        string reopened_save_name = saving.save(stratum_sim::SaveType::Explicit);
        stratum_format::raw_values_type reopened_game_json = read_json(saving_config.saves_directory / reopened_save_name / "game.json");
        CHECK_CASE("the random engine's state is restored, so a save made at once has the same state",
            reopened_game_json["game"]["random_engines"]["gameplay"] == saved_random_engine_state);

        settings::store().set_value<bool>(stratum_sim::Game::reseed_on_open_key, true);
        saving.open(save_name);
        string reseeded_save_name = saving.save(stratum_sim::SaveType::Explicit);
        stratum_format::raw_values_type reseeded_game_json = read_json(saving_config.saves_directory / reseeded_save_name / "game.json");
        CHECK_CASE("or reseeded, as the settings choose", reseeded_game_json["game"]["random_engines"]["gameplay"] != saved_random_engine_state
            && reseeded_game_json["game"]["seed"] != game_json["game"]["seed"]);
        settings::store().set_value<bool>(stratum_sim::Game::reseed_on_open_key, false);

        const string detection_key = "gameplay.difficulty.detection_range_multiplier";
        settings::store().set_text("gameplay.difficulty", "hard");
        settings::store().set_value<double>(detection_key, 2);
        string settings_save_name = saving.save(stratum_sim::SaveType::Explicit);
        stratum_format::raw_values_type settings_game_json = read_json(saving_config.saves_directory / settings_save_name / "game.json");
        CHECK_CASE("a game saves its group's selected preset and its editable preset's values", settings_game_json["settings"]["gameplay.difficulty"]
            == "custom" && settings_game_json["presets"]["gameplay.difficulty"]["custom"][detection_key] == "2"
            && !settings_game_json["settings"].contains(detection_key) && !settings_game_json["settings"].contains("gameplay.reseed_on_open"));

        saving.open(save_name);
        CHECK_CASE("and a game opened without them has the game settings' defaults", settings::store().text("gameplay.difficulty") == "normal"
            && settings::store().value<double>(detection_key) == 1 && settings::store().preset_values(settings::SettingScope::Game).empty());

        saving.open(settings_save_name);
        CHECK_CASE("and opened with them has them again", settings::store().text("gameplay.difficulty") == "custom"
            && settings::store().value<double>(detection_key) == 2 && settings::store().value<double>("gameplay.difficulty.denizen_count_multiplier") == 1.5);

        saving.open(replaced_save_name);
        CHECK_CASE("a save whose character's name cannot name a directory opens into one with the characters replaced", opened.loaded()
            && filesystem::is_directory(saving_config.working_directory / "Ailis_ of the _Downs__")
            && !filesystem::exists(game_directory));

        bool absent_thrown = false;

        try
        {
            saving.open("Nobody_20000101T000000");
        }
        catch (const runtime_error&)
        {
            absent_thrown = true;
        }

        CHECK_CASE("opening a save that is not there is an error, which leaves no game in progress", absent_thrown && !opened.loaded()
            && !saving.character().entity() && saving_world.entities().count<stratum_sim::Region>() == 0);

        // Removing saves
        size_t save_count = saving.saved_games().size();
        saving.remove(second_save_name);
        CHECK_CASE("removing a save deletes its directory, and it is no longer listed", !filesystem::exists(saving_config.saves_directory
            / second_save_name) && saving.saved_games().size() == save_count - 1);

        filesystem::create_directories(saving_config.saves_directory / "stray");
        auto remove_throws = [&](string_view name)
        {
            try
            {
                saving.remove(name);
            }
            catch (const runtime_error&)
            {
                return true;
            }

            return false;
        };

        CHECK_CASE("a name that is not a save in the saves directory is not removed: a parent, a path, or none at all",
            remove_throws("..") && remove_throws(format("../{}", save_name)) && remove_throws("Nobody_20000101T000000")
            && remove_throws("") && filesystem::is_directory(saving_config.saves_directory / save_name));
        CHECK_CASE("nor a directory there with no character file", remove_throws("stray") && filesystem::is_directory(saving_config.saves_directory
            / "stray"));
        filesystem::remove_all(saving_config.saves_directory / "stray");

        saving.open(save_name);
        filesystem::remove_all(zones_directory);
        saving.quit();
        CHECK_CASE("and a game cleared saves nothing", !filesystem::exists(zones_directory));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("persistent settings")
    {
        const filesystem::path filepath = "out/user/settings.json";
        filesystem::remove_all("out/user");

        settings::Settings test_settings;
        stratum_sim::load_setting_definitions("../../assets", test_settings);

        stratum_sim::PersistentSettings user_settings(filepath, settings::SettingScope::User, test_settings);
        user_settings.load();
        CHECK_CASE("with no settings file, the settings have their defaults, and nothing has changed",
            !test_settings.value<bool>(stratum_sim::Game::reseed_on_open_key) && !user_settings.changed());

        test_settings.set_text("gameplay.difficulty", "hard");
        CHECK_CASE("a change in another scope is not a change of the file's", !user_settings.changed());

        test_settings.set_value<bool>(stratum_sim::Game::reseed_on_open_key, true);
        CHECK_CASE("and a change in its scope is, but writes nothing until it is saved", user_settings.changed() && !filesystem::exists(filepath));

        user_settings.save_if_changed();
        ifstream settings_stream(filepath, ios::binary);
        stratum_format::raw_values_type settings_json = stratum_format::raw_values_type::parse(settings_stream, nullptr, false);
        CHECK_CASE("saving it writes only the scope's values, and leaves no change", !user_settings.changed()
            && settings_json["settings"]["gameplay.reseed_on_open"] == "true" && settings_json["presets"].empty() && settings_json["settings"].size() == 1);

        settings::Settings opened_settings;
        stratum_sim::load_setting_definitions("../../assets", opened_settings);
        stratum_sim::PersistentSettings opened_user_settings(filepath, settings::SettingScope::User, opened_settings);
        opened_user_settings.load();
        CHECK_CASE("and loading it sets them again, which is no change", opened_settings.value<bool>(stratum_sim::Game::reseed_on_open_key)
            && opened_settings.text("gameplay.difficulty") == "normal" && !opened_user_settings.changed());

        {
            ofstream game_setting_stream(filepath, ios::binary);
            game_setting_stream << "{ \"format\": \"quetzal.stratum_settings\", \"version\": \"0.1\", "
                "\"settings\": { \"gameplay.difficulty.denizen_count_multiplier\": \"2\" } }\n";
        }

        string game_setting_error;

        try
        {
            opened_user_settings.load();
        }
        catch (const runtime_error& e)
        {
            game_setting_error = e.what();
        }

        CHECK_CASE("a game setting in the user settings file is an error, which names the file and the setting",
            game_setting_error == "out/user/settings.json: 'gameplay.difficulty.denizen_count_multiplier' is not a user setting"
            && opened_settings.value<double>("gameplay.difficulty.denizen_count_multiplier") == 1);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("setting definitions inserted")
    {
        settings::Settings test_settings;
        stratum_sim::insert_definitions(stratum_format::read_setting_definitions(filesystem::path("../../assets")
            / stratum_format::SettingDefinitionsDocument::filepath), test_settings);

        const string count_key = "gameplay.difficulty.denizen_count_multiplier";
        CHECK_CASE("the game's definitions insert its settings, with their menu fields and ranges", test_settings.contains(count_key)
            && test_settings.setting(count_key).scope == settings::SettingScope::Game && test_settings.setting(count_key).category == "difficulty"
            && test_settings.setting(count_key).minimum == "0.25" && !test_settings.bounded_inclusive(count_key, "5"));
        CHECK_CASE("and its difficulty group, on its default preset", test_settings.contains_group("gameplay.difficulty")
            && test_settings.text("gameplay.difficulty") == "normal" && test_settings.setting("gameplay.difficulty").display_name == "Difficulty"
            && test_settings.value<double>(count_key) == 1);

        test_settings.set_text("gameplay.difficulty", "hard");
        CHECK_CASE("whose presets set its members", test_settings.value<double>(count_key) == 1.5
            && test_settings.value<double>("gameplay.difficulty.denizen_speed_multiplier") == 1.25);
        CHECK_CASE("and whose editable preset has no values until it is selected",
            test_settings.preset("gameplay.difficulty", "custom").values.empty());

        stratum_format::SettingDefinitionsDocument document;
        document.enumerations.push_back({"mod.torches.style", {"steady", "flicker"}});
        document.settings.push_back({.key = "mod.torches.style", .type_name = "mod.torches.style", .default_value = "flicker",
            .scope = stratum_format::SettingDefinitionsDocument::Scope::User, .restart_required = true});
        stratum_sim::insert_definitions(document, test_settings);
        CHECK_CASE("an enumeration is registered before the settings that have it", test_settings.registered_type("mod.torches.style")
            && test_settings.text("mod.torches.style") == "flicker" && test_settings.setting("mod.torches.style").restart_required
            && test_settings.setting("mod.torches.style").scope == settings::SettingScope::User);

        bool inserted_twice_throws = false;
        try
        {
            stratum_sim::insert_definitions(document, test_settings);
        }
        catch (const runtime_error&)
        {
            inserted_twice_throws = true;
        }

        CHECK_CASE("and a definition that is invalid is an error", inserted_twice_throws);

        using document_type = stratum_format::SettingDefinitionsDocument;
        document_type member_document;
        member_document.settings.push_back({.key = "gameplay.size", .type_name = "integer", .default_value = "2",
            .scope = document_type::Scope::Game});
        member_document.groups.push_back({.key = "gameplay.scale", .scope = document_type::Scope::Game, .default_preset_name = "small",
            .member_keys = {"gameplay.size"}, .presets = {{.name = "small", .values = {{"gameplay.size", "1"}}}}});
        stratum_sim::insert_definitions(member_document, test_settings);
        CHECK_CASE("a member keeps its own default, and has its value in the default preset",
            test_settings.setting("gameplay.size").default_value == "2" && test_settings.value<int64_t>("gameplay.size") == 1);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("config")
    {
        // The program directory, with or without a config file
        const filesystem::path program_directory = "out/config";
        filesystem::remove_all(program_directory);
        filesystem::create_directories(program_directory);

        const filesystem::path user_directory = stratum_sim::default_user_directory();
        stratum_sim::Config config = stratum_sim::load_config(program_directory);
        CHECK_CASE("with no config file, the assets are in the program directory, and the rest in the user directory",
            config.assets_directory == program_directory / "assets" && config.user_directory == user_directory
            && config.saves_directory == user_directory / "saves" && config.working_directory == user_directory / "working"
            && config.logs_directory == user_directory / "logs" && config.cache_directory == user_directory / "cache"
            && config.mods_directory == user_directory / "mods" && config.user_settings_filepath == user_directory / "settings.json"
            && config.zone_count_max == 8 && config.log_file_count_max == 10);
        CHECK_CASE("and nothing is written in the program directory", filesystem::is_empty(program_directory));

        const filesystem::path absolute_logs = filesystem::absolute("out/config_logs");
        {
            ofstream os(program_directory / "config.json", ios_base::binary);
            os << "{ \"format\": \"quetzal.stratum_config\", \"version\": \"0.1\", \"assets_directory\": \"../../assets\", "
                << "\"user_directory\": \"user\", \"saves_directory\": \"elsewhere/saves\", \"logs_directory\": \""
                << absolute_logs.generic_string() << "\", \"zone_count_max\": 3, \"log_file_count_max\": 4 }";
        }

        config = stratum_sim::load_config(program_directory);
        CHECK_CASE("a config file overrides the values it has, the assets and user directories relative to the program directory",
            config.assets_directory == program_directory / "../../assets" && config.user_directory == program_directory / "user"
            && config.zone_count_max == 3 && config.log_file_count_max == 4);
        CHECK_CASE("and the other locations relative to the user directory, an absolute one kept",
            config.saves_directory == program_directory / "user/elsewhere/saves" && config.logs_directory == absolute_logs
            && config.working_directory == program_directory / "user/working");

        {
            ofstream os(program_directory / "config.json", ios_base::binary);
            os << "{ \"format\": \"quetzal.stratum_config\", \"version\": \"0.1\", \"save_directory\": \"saves\" }";
        }

        bool unknown_key_rejected = false;
        try
        {
            stratum_sim::load_config(program_directory);
        }
        catch (const runtime_error&)
        {
            unknown_key_rejected = true;
        }

        CHECK_CASE("a key the config file does not have is an error", unknown_key_rejected);
        CHECK_CASE("the default user directory is KML/Stratum in the platform's directory for application data",
            user_directory.generic_string().ends_with("KML/Stratum"));
        CHECK_CASE("the executable's directory has the test executable", filesystem::exists(executable_directory() / "stratum_sim_test.exe"));

        return;
    }

    TEST_SUITE_END();

} // namespace
