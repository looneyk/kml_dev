# Settings --- Plan

Date: 2026-10-05, revised 2026-10-19

How the game's settings are defined, stored, read and saved. There may be a hundred or more, from every subsystem of the
game. The open items are in `work_plan.md` (`### Settings`), and the work that remains in **Mods' settings**.

## Terms

| Term | What it is | What others call it |
| --- | --- | --- |
| **setting** | One value the player or the game chooses, such as a difficulty multiplier or the video mode | *option* in NetHack and DCSS; *preference* in Apple's and Java's APIs and Unity's `PlayerPrefs`; *console variable* (*cvar*) in Quake, Source and Unreal when the console can set it |
| **key** | A setting's name, the string the store maps: `"video.fullscreen"`. The store compares a key only as a whole string | *key* in Qt, ini files, .NET, Unity and Godot's `ConfigFile`; *name* in Godot's `ProjectSettings` (whose docs also call it a path), and for a cvar or a `ConVar`; *option name* in NetHack and DCSS |
| **subsystem** | A library or host with settings of its own, which registers their C++ types and reads them: the sim, a view, a host, the log | *module* in Unreal and Godot; *subsystem* in Unreal's `USubsystem` classes |
| **section** | A leading run of a key's `.`-separated segments, by naming convention only: `video.window.width` is in `video` and `video.window`. In stratum, a key's outermost section is the subsystem the setting belongs to, and a mod's settings are in `mod.<id>`. Neither the store nor a setting group interprets a section | *section* in ini files, Unreal's config, Godot's `ConfigFile` and .NET; *group* in Qt, which is unrelated to a setting group; *node* in Java's `Preferences`; a dotted prefix by convention for cvars, `r.` for rendering |
| **store** | The one map from key to setting, the `Settings` that `settings::store()` returns, and the only place a value is kept | *ProjectSettings* in Godot; *GConfig*, the config cache, in Unreal; the *console manager* for Unreal's cvars and the *cvar registry* for Source's |
| **definition** | A setting's one entry in data: its key, type, default and scope, and whether a restart is required | a `bool-setting` or `string-setting` prototype in Factorio's mod settings; `GLOBAL_DEF` in Godot; a `TAutoConsoleVariable` in Unreal and a `ConVar` in Source, each in C++; a `GameOption` in DCSS; an entry in NetHack's options table |
| **type** | A registered name for the values a setting may have: `boolean`, `integer`, `real` or `text`, which the store registers; an enumeration that a subsystem registers in C++; or an enumeration defined in data | the *setting type* of a Factorio prototype; a *simple type* in XML Schema |
| **base type** | The built-in type a registered type is based on: `text` for every enumeration. A script reads a setting as a value of its base type | the *base* of an XML Schema restriction, `<xs:restriction base="xs:string">` |
| **default** | The value a setting has until something sets it, from the setting's definition | *default* everywhere |
| **setting group** | A setting whose value is the name of one of its presets, and whose members are the settings those presets assign. A member's value is the selected preset's value for it | a *scalability group* in Unreal, `sg.ShadowQuality`, which is itself a console variable |
| **preset** | A value for each of a setting group's members, defined in data. A group's default preset is the one selected for a new game. A modifiable preset, such as Custom, has values the player sets, which the store keeps | a *level* of an Unreal scalability group, `[ShadowQuality@2]`; a *preset* in an options menu, "graphics preset: High" |
| **apply** | Putting a change into effect where the next read is not soon enough, such as switching the video mode | *apply* in Unreal's `ApplySettings`; a `ConVar`'s *change callback* in Source does it for one setting |
| **change notification** | The store calling each change listener once after an operation that changed one or more settings, with the keys it changed | *change callback* in Source (`FnChangeCallback_t`); the `onDidChangeConfiguration` event of VS Code's settings; the `settings_changed` signal of Godot's `ProjectSettings` |
| **restart required** | A setting that takes effect only when the program starts again | `restart_if_changed` in Godot; `ConfigRestartRequired` in Unreal; *requires restart* in options menus |
| **scope** | Whether a setting is a game's, saved in `game.json`, or the user's, saved in the user settings file: `SettingScope::Game` or `SettingScope::User` | NetHack's `flags` and `iflags`; Source's `FCVAR_ARCHIVE` flag; Godot's project settings and editor settings; the *layers* of Unreal's config: engine, project, platform and user |
| **user settings file** | `settings.json` in the user directory, unless the config file puts it elsewhere (`game_file_spec.md`): the file the user settings are saved in | *config.cfg* in Source; *GameUserSettings.ini* in Unreal; a `ConfigFile` in `user://` in Godot; the *options file* or *rc file*, `.nethackrc` and `init.txt`, in the roguelikes |

By convention, a key's sections are separated by `.`, as the `RegionSpec` and `ZoneSpec` keys already are (`zone.level`,
`parameters.seed`), and as Unreal's console variables (`r.ScreenPercentage`), VS Code's settings and Lua's table access
separate theirs. Godot's `ProjectSettings` and Qt's `QSettings` use `/`, .NET's configuration `:`, and Source's cvars an
`_` prefix (`sv_cheats`).

`Config` is not a setting. It is the program's fixed configuration, such as its directories, which the player does not
choose.

## Overview

- **Each subsystem has settings of its own.** Play and generation have the difficulty multipliers, the notation and
  how a save is opened; the views have video settings; the hosts have audio and controls. The sim must not depend on
  the view's or the hosts' settings.
- **A setting's scope is a game or the user.** A game setting, such as a difficulty multiplier or the notation,
  belongs to one game, is saved in its `game.json`, and is set by `Game` when the game is created or opened. A user
  setting, such as video, audio or controls, belongs to the player and the machine, is the same in every game, and is
  saved in the user settings file.
- **A new game's settings are the defaults, with each setting group's default preset selected,**
- **Any setting may change at any time,** a user setting while a game is open and a game setting during play, as the
  difficulty mode may (`difficulty_plan.md`). A game setting change is applied between advances, never partway through a turn.
- **An operation reads the settings it needs from the store,** when it begins or as it needs them, as NetHack reads
  `flags.verbose` and DCSS `Options.autopickup_on` wherever they are needed, and as code reads Godot's
  `ProjectSettings` and Unreal's console variables. The difficulty multipliers are read where each one applies: the
  denizen count when a zone is populated, the speed where a denizen is given its `Speed`, and the detection range
  where its `Detection` is set.
- **A copy changes only when a setting is read.** An operation may load settings into a structure to pass them
  around, or cache them where it reads them often, as DCSS reads its key bindings into keymaps that a key press
  consults. The settings it reads are the same either way.
- **A change that has to take effect at once is applied by a change listener.** A change notifies every change
  listener with the keys it changed, and each listener decides from the keys whether to act. Settings that take effect
  together, such as the video mode, the resolution and the UI scale, are applied together by one listener, as Unreal's
  `UGameUserSettings::ApplySettings` applies them; a listener may also refresh a cached copy.
- **The store is the one source of truth.** Saving reads the store; a copy is never written back to it.

## Typed structures or keyed values

There are two ways to hold them.

- **Typed structures for each layer.** Each library has a class of its own settings as typed members, with a getter
  and a setter for each: `SimSettings` in `stratum_sim`, `ViewSettings` in `stratum_view`, and the hosts' own. Each is
  saved by code that names each member.
- **Keyed values in one store.** One store, in a library below the sim, contains every setting's value under its key,
  such as `gameplay.difficulty.denizen_count_multiplier` or `video.fullscreen`. Each setting has one definition, and each layer reads
  only its own settings.

### Other games

- **NetHack** has typed global structures, `flags` and `iflags`, and a table that maps each option's name to its field
  and type, through which the options file is read and the options menu is drawn.
- **Dungeon Crawl Stone Soup** has one typed global, `Options`, a `game_options` with a member for each option. Each
  option is also registered as a `GameOption` object, a `BoolGameOption` or a `ColourGameOption` among them, with its
  name, its default and the member it sets, and the options file is read through that list.
- **Unreal Engine** has both. `UGameUserSettings` and each `UDeveloperSettings` subclass are typed classes whose
  `config` properties the engine saves to an ini section by reflection, and lists in the editor's Project Settings.
  Console variables are keyed: `TAutoConsoleVariable<int32> CVarExample(TEXT("r.Example"), 1, TEXT("help"))` declares
  one, once, as a typed C++ object under its name. Code reads it through that object or by name,
  `FindConsoleVariable(TEXT("r.Example"))->GetInt()`, and an ini or the console sets it by name.
- **The Source engine** declares each setting as a `ConVar`, `ConVar sv_cheats("sv_cheats", "0", flags, "help")`, a
  typed C++ object under its name. Its flags state its scope: `FCVAR_ARCHIVE` saves it to the player's `config.cfg`.
- **Godot** has `ProjectSettings`, a keyed store read with `get_setting("display/window/size/viewport_width")`.
  `GLOBAL_DEF` declares a setting with its default, and its type and hints draw it in the editor. A game's own player
  settings are usually a `ConfigFile` of sections and keys, saved to `user://`.
- **Factorio** defines a mod's settings only in data, as `bool-setting`, `int-setting`, `double-setting` and
  `string-setting` prototypes, each with a name, a default and a setting type, startup or runtime, and a
  `string-setting` may list its allowed values. Lua reads `settings.startup["name"].value` as a value of the setting's
  type.
- **Qt** has `QSettings`, keyed by `group/key`, each value a `QVariant` read with a default at the call. **Unity** has
  `PlayerPrefs`, keyed by name, with `GetInt`, `GetFloat` and `GetString`, and no declaration at all.

The structured games, NetHack, DCSS and Unreal's settings classes, each keep a table of names beside their fields,
since a file, a menu and a console all reach a setting by name. Unreal's console variables and Source's `ConVar`s
start from the name, and declare each setting as a typed C++ object. Factorio's mod settings start from the name and
are defined only in data.

### Pros and cons

| | Typed structures for each layer | Keyed values in one store |
| --- | --- | --- |
| **Adding a setting** | A member, a getter, a setter, and its entry in the code that saves it, in C++ | One definition, in data |
| **A mod's settings** | Impossible without C++ | Defined in data like any other (`modding_plan.md`) |
| **Saving and loading** | Code for each member, or a name table beside the members, as NetHack and DCSS keep | One loop over the store, by key |
| **An options menu** | Written by hand, or drawn through the same name table | Drawn from the definitions, as Godot's and Unreal's editors draw theirs |
| **Scripts** | Each setting exposed by hand | Read by key, as a value of the setting's base type |
| **A wrong name or type** | A compile error | An assert at the read |
| **Reading cost** | A member read | A map lookup and a conversion from text at each read, which a copy avoids where reads are frequent |
| **Tools** | Completion and refactoring see every setting | They see a key in C++ only as a string |
| **Ownership by layer** | Each structure in its own library; nothing shared | One store below the sim; each layer reads only its own settings |
| **Scope** | Which structure a setting is in, or a member's annotation | A field of each definition, as a `ConVar`'s flags and a Factorio prototype's setting type state it |

The typed structures check at compile time, but each setting is C++ in three places, mods cannot add any, and a name
table has to be built beside them anyway, for the file and the menu. The keyed store makes a setting data, which is
the design goal (`modding_plan.md`). A misspelled key or a wrong type asserts at the read, so a test that runs the
operation finds it.

### Recommendation

**Keyed values in one store, each setting defined once, in data,** as Factorio defines a mod's settings, and read by
key, as Unreal's console manager, Source's cvar registry, Godot's `ProjectSettings` and Qt's `QSettings` are read. C++
defines no setting: it registers the types its settings have, and reads a setting by its key and its C++ type. The
store keeps every setting's value, and an operation reads the settings it needs when it needs them.

```json
{ "key": "video.fullscreen", "type": "boolean", "default": "false", "scope": "user", "restart_required": true }
```

```cpp
// The view reads it: the store asserts that the setting's type is a bool
if (settings::store().value<bool>("video.fullscreen"))

// The options menu changes it
settings::store().set_value<bool>("video.fullscreen", true);
```

The user settings file has only the value, under its key:

```json
{ "settings": { "video.fullscreen": "true" } }
```

A mod's setting is defined the same way. A script reads it by key as a value of its base type:
`settings.value("mod.torches.flicker")` returns a boolean.

```json
{ "key": "mod.torches.flicker", "type": "boolean", "default": "true", "scope": "user" }
```

A definition's type is a registered type name. The store registers `boolean`, `integer`, `real` and `text`; a subsystem
registers each enumeration its settings have, with the enumeration's values; and the loader registers each enumeration
defined in data. An enumeration defined in data is text with a list of allowed values, and C++ reads it as a
`std::string`:

```json
"types": { "mod.torches.style": ["steady", "flicker", "gutter"] }
```

A patch can add values to an enumeration defined in data, but not to one registered in C++, whose values are fixed when
it is compiled.

C++ states the type at each read and write. `set_value`'s value parameter does not deduce `T`, since an integer literal
would deduce `int` rather than `std::int64_t`, which `integer` is, and a string literal `const char*`.

### The store

The library is `library/quetzal/settings/`, the target `quetzal_settings` in the namespace `quetzal::settings`, with
`settings_test`. It links only `quetzal_common`.

```cpp
namespace quetzal::settings
{

    enum class SettingScope { User, Game };
    enum class SettingBaseType { Boolean, Integer, Real, Text };

    // A C++ type a setting may have: one that std::format writes, and that quetzal::try_type reads back through
    // from_chars. An enumeration needs a std::formatter specialization and a from_chars overload in its namespace
    template<typename T>
    concept SettingValue = std::formattable<T, char> && (std::same_as<T, std::string> || /* from_chars(first, last, value) */);

    // A registered type
    struct SettingType
    {
        std::type_index cpp_type;                  // std::string for an enumeration defined in data
        SettingBaseType base_type;
        std::vector<std::string> values;           // An enumeration's values; empty for a built-in type
        std::function<bool(std::string_view text)> accepts;
        bool group = false;                        // An enumeration of a setting group's preset names
    };

    struct SettingGroup
    {
        std::string key;
        std::vector<std::string> member_keys;
    };

    struct Preset
    {
        std::string display_name;
        bool modifiable = false;
        KeyValueStore values;                      // <member key, value>; empty for a modifiable preset not yet defined
    };

    // What the store keeps for every setting
    struct Setting
    {
        std::string type_name;
        std::string default_value;
        SettingScope scope;
        bool restart_required;
        bool modifiable;                           // A game setting's: true if it can change after the game is created
        std::optional<std::string> initializer_setting; // A game setting's: the user setting a new game takes its value from
        std::string display_name;
        std::string description;
        std::string category;                      // The menu page that shows it
        std::optional<std::string> minimum;        // An integer's or a real's
        std::optional<std::string> maximum;
        std::optional<std::string> value;          // If empty, has its default value
    };

    class Settings
    {
    public:

        using listener_id_type = std::size_t;
        using preset_key_type = std::pair<std::string, std::string>;                                  // {group key, preset name}
        using preset_values_type = std::map<preset_key_type, KeyValueStore>;                          // <{group key, preset name}, values>

        template<SettingValue T>
        void register_type(std::string_view type_name, std::vector<std::string> values = {});        // from C++
        void register_enumeration(std::string_view type_name, std::vector<std::string> values);       // from data
        bool registered_type(std::string_view type_name) const;
        const SettingType& type(std::string_view type_name) const;

        void insert(std::string_view key, Setting setting);                                          // a setting with no value
        bool contains(std::string_view key) const;
        const Setting& setting(std::string_view key) const;
        bool bounded_inclusive(std::string_view key, std::string_view text) const;

        // Registers a group type under the key, whose values are the presets' names in order, and inserts the group's
        // setting of that type, whose default is the default preset. Requires the members to be inserted already
        void insert_group(std::string_view key, SettingScope scope, std::string_view default_preset_name, std::vector<std::string> member_keys,
            std::vector<std::pair<std::string, Preset>> presets, std::string_view display_name = {}, std::string_view category = {});
        bool contains_group(std::string_view key) const;
        const SettingGroup& group(std::string_view key) const;
        std::optional<std::string> member_group_key(std::string_view key) const;                     // a member's group
        const Preset& preset(std::string_view group_key, std::string_view preset_name) const;

        // Each set returns false, setting nothing, if a value is out of its setting's range
        template<SettingValue T> T value(std::string_view key) const;                                // reading
        template<SettingValue T> bool set_value(std::string_view key, const std::type_identity_t<T>& value);

        std::string text(std::string_view key) const;                                                // by key, as text
        bool set_text(std::string_view key, std::string_view text);

        KeyValueStore values(SettingScope scope) const;                                              // saving
        bool set_values(const KeyValueStore& values);                                                // loading
        bool set_values(const KeyValueStore& values, const preset_values_type& preset_values);       // modifiable presets' first
        void reset(SettingScope scope);                                                              // a game cleared

        // The defined values of each modifiable preset of the scope's groups
        preset_values_type preset_values(SettingScope scope) const;

        listener_id_type add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener);
        void remove_listener(listener_id_type listener_id);

        void clear();                                                                                // keeps the listeners
    };

    // The process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings
```

`SettingGroup` and `Preset` are in `SettingGroup.hpp`, and `SettingsSnapshot`, one scope's values as a file has them,
in `SettingsSnapshot.hpp`:

```cpp
namespace quetzal::settings
{

    struct SettingsSnapshot
    {
        KeyValueStore settings;                                       // <key, value>, groups' selections included
        Settings::preset_values_type presets;                         // <{group key, preset name}, values>

        static SettingsSnapshot capture(SettingScope scope, const Settings& store);

        // Checks every key, preset and value before changing any. Sets the presets' values first, then the settings, with one
        // set_values, so loading a file notifies once
        void restore(SettingScope scope, Settings& store) const;
    };

} // namespace quetzal::settings
```

**Usage:**

- **At startup, in each host's `main`,** the config is loaded, any C++ enumeration a subsystem's settings have is
  registered with `settings::store().register_type<T>(name, values)`, and the loader reads the definitions, the game's
  and then each mod's, registering their enumerations, inserting their settings into the store, and then inserting
  their groups with `insert_group`. The user's values are loaded then; a game's are set when it is created or opened.
- **Loading sets the values:** from the user settings file at startup, and from `game.json` when a game is opened,
  through `SettingsSnapshot::restore`. A key that is not a setting's of the file's scope, text that is no value of its
  setting's type, a value out of range, or a preset's values that are not exactly its group's members, is an error, and
  nothing is set.
- **An operation reads its settings** with `value<T>(key)`, when it begins or as it needs them. A subsystem that has to
  act on a change at once, such as the view switching the video mode, adds a change listener that acts when any of its
  keys is among the keys changed.
- **Saving writes the values set in each scope,** `values(SettingScope::Game)` into `game.json`, and
  `values(SettingScope::User)` into the user settings file, each value as text, in the file's `settings`, and the
  defined values of the scope's modifiable presets, `preset_values`, in its `presets`. A setting never set is not
  written, so it keeps its default, even if the default changes. A member never has a value of its own, so saving
  writes none for it in `settings`; a member's value that a file's `settings` has is set as setting the member sets it.
- **A game cleared** returns the game scope's settings to their defaults with `reset(SettingScope::Game)`, which
  selects each group's default preset and leaves the values of its modifiable presets undefined.

**The store's decisions:**

- **A value is kept as text, in the store and in a file, and converted on each read,** as ini files, Source's
  `config.cfg`, Minecraft's `options.txt` and Qt's `QSettings` keep theirs. The store needs no `std::variant` or
  `std::any`. Each read converts the text, which a copy avoids where an operation reads a setting often.
- **A C++ type is described only by its formatting and parsing.** `quetzal::to_string` writes a value through
  `std::format` and `quetzal::try_type` reads it through `from_chars`, as `KeyValueStore` reads its values, so an
  enumeration that is a setting works with `std::format`, the log and `KeyValueStore` too. Its formatter's `format` is a
  template over the context, since `std::formattable` checks it with the standard.
- **An enumeration registered in C++ lists its values,** so the menu can offer them. Registering it checks that each
  value parses.
- **A setting has its type's C++ type,** so reading a setting with the wrong type asserts, whatever registered it.
- **An integer or a real may have a minimum and a maximum,** which the store enforces. `insert` throws if the default is
  out of range. A set returns false, setting nothing, if any value is out of range, so the menu can tell the player;
  `bounded_inclusive` checks a value before it is set.
- **The store keeps a setting's menu fields** --- its display name, description and category, the menu page that shows
  it --- which only the menu reads.
- **A value whose key is not a setting is an error,** reported when its file is loaded, so a misspelled key in a file
  or a mod is found at once.
- **A key is opaque.** The store compares only whole keys, and requires only that a key is not empty.
- **The store resolves groups itself** (`### Setting groups`), so a selection and the members it changes are one
  operation and one notification.
- **A change is notified once for each operation,** after every value it changes has changed, with all of their keys,
  so loading a file or resetting a scope notifies once. Setting a value it already has notifies nothing. Which keys
  matter is the listener's to decide.
- **Listeners last until they are removed.** Each add returns an id, which `remove_listener` takes, and `clear` keeps
  them, so the listeners of subsystems that last as long as the store survive a test's `clear`.

### The definition file

Each mod has one definition file, and the game's own is the first: `assets/data/setting_definitions.json` in the
program directory, and `data/setting_definitions.json` in each mod's directory. It defines settings of both scopes.

```json
{
  "format": "quetzal.stratum_setting_definitions",
  "version": "0.1",
  "enumerations": [
    { "name": "mod.torches.style", "values": ["steady", "flicker", "gutter"] }
  ],
  "settings": [
    { "key": "gameplay.difficulty.denizen_count_multiplier", "type": "real", "default": "1", "scope": "game",
      "minimum": "0.25", "maximum": "4", "display_name": "Denizen count", "category": "difficulty" },
    { "key": "video.fullscreen", "type": "boolean", "default": "false", "scope": "user", "restart_required": true,
      "display_name": "Fullscreen", "category": "video" }
  ],
  "groups": [
    { "key": "gameplay.difficulty", "scope": "game", "default_preset": "normal", "display_name": "Difficulty", "category": "difficulty",
      "members": ["gameplay.difficulty.denizen_count_multiplier"],
      "presets": [
        { "name": "easy", "display_name": "Easy", "values": { "gameplay.difficulty.denizen_count_multiplier": "0.5" } },
        { "name": "normal", "display_name": "Normal", "values": { "gameplay.difficulty.denizen_count_multiplier": "1" } },
        { "name": "custom", "display_name": "Custom", "modifiable": true }
      ] }
  ]
}
```

- **Each section is an array of entries with a key or a name,** so a duplicate reaches `insert` and is reported, and the
  menu shows entries in the file's order.
- **The sections are inserted in order:** enumerations, then settings, then groups, each of which may refer to those
  before it. The files are inserted in mod order, the game's first.
- **Every value is text,** as in the store and the saved files.
- **A setting's entry has what `Setting` has:** `type`, `default`, `scope`, `restart_required`, which is false if
  absent, and the menu fields `display_name`, `description`, `category`, `minimum` and `maximum`. Every setting has a
  `default`, a group's member included.
- **A game setting's entry may have an `initializer`,** which sets its value when a game is created. The first form
  refers to a user setting, `"initializer": { "setting": "world.world_size_default.x" }`, whose value the game setting
  takes; a script is a later form (`work_plan.md`, `## Revisit later`). A game setting without one, or whose user
  setting is not defined, takes its own default.
- **A game setting's entry may have `modifiable`,** true if the setting can change after the game is created, as the
  difficulty settings can, and false if absent, so a game setting without it is fixed for the game's life.
- **A group's entry has what `insert_group` takes:** `key`, `scope`, `default_preset`, `members`, `presets`,
  `display_name` and `category`. A preset's entry has `name`, `display_name`, `modifiable`, which is false if absent, and
  `values`, which a modifiable preset does not have.
- **`stratum_format` reads the file** into a `SettingDefinitionsDocument`, and `stratum_sim`'s `insert_definitions`
  inserts it into a store, so the settings library has no JSON.

The values are saved apart from the definitions: the user's in the user settings file, `settings.json` in the user
directory, and a game's in its `game.json`. Each file has the settings' values in `settings`, groups' selections
among them, and the defined values of its modifiable presets in `presets`, by group key and preset name. The values of a
mod's settings and presets are under that mod's entry in each file's `mods`, so the loader can keep them while the mod
is not loaded:

```json
{ "format": "quetzal.stratum_settings", "version": "0.1",
  "settings": { "video.fullscreen": "true", "video.quality": "custom" },
  "presets": { "video.quality": { "custom": { "video.shadow_resolution": "2048", "video.texture_resolution": "4096" } } },
  "mods": [ { "id": "torches", "settings": { "mod.torches.flicker": "false" }, "presets": { } } ] }
```

### A new game's settings

Lookups name no scope: a game reads every setting from the one store by its key, and a setting's scope decides only
where its value is saved. When a game is created, each game setting is reset to its default and then set by its
`initializer`, if it has one. An opened game sets its game settings from its `game.json` alone. During the game, a
game setting changes only if it is `modifiable`; the store rejects a change to one that is not, and the menu shows it as
read-only.

The world's dimensions are game settings initialized this way, so each world keeps the dimensions it was created with:
`world.world_size.x` and `.y`, the number of regions the world has room for in each direction, 1000000 by default, and
`world.region_size.x`, `.y` and `.z`, the number of zones a region has room for in each direction, 1000 by default.
Each is an `integer` of at least 1, not `modifiable`, whose `initializer` refers to the user setting of the same key with
`_default` after `world_size` or `region_size`. A region's or zone's coordinate along a size `s` is signed, from `-(s / 2)` to `s - s / 2 - 1`, so the
starting region is at the origin of the world and the starting zone at the origin of its region.

### Setting groups

A setting group is itself a setting, as Unreal's `sg.ShadowQuality` is a console variable. Its value is the name of
its selected preset, and its type is an enumeration of its presets' names that is marked as a group's. The difficulty
mode is one (`difficulty_plan.md`), and a video quality preset another. The store resolves groups itself, so there is
one store, one key space and one change notification, for settings and groups alike.

- **A group has a key, a scope, its members, and its presets, one of which is its default.** Its `SettingGroup` has the
  key and the member keys; everything else a group has is its setting's, its type's and its presets'. Its selection
  is saved under its key in a file's `settings`, like any setting's value: `"gameplay.difficulty": "hard"`.
- **The presets are one table,** from a group key and a preset name to the `Preset`, which only a member's lookup and a
  selection consult. The group's type lists the preset names in order, for the menu.
- **A member's value is the selected preset's value for it.** A member never has a value of its own, so reading a
  member reads the selected preset's row of the table, and a rebalanced preset reaches every game that selected it.
- **Every preset that is not modifiable sets each of its group's members.** Setting them is the preset's purpose, so a
  member's value always comes from a preset. The default preset is not modifiable. A member's own default, in its
  definition, is independent of the presets: it is the value that resetting the member in the modifiable preset sets.
- **Each member has the group's scope, a setting is a member of at most one group,** as each Unreal console variable is
  in at most one scalability group, **and a member is not a group.** `insert_group` checks each. Nested groups, such as
  an overall video quality that selects the shadow and texture qualities, are not supported; the member lookup and
  `insert_group`'s check are where they would be added.
- **A group has at most one modifiable preset, such as Custom.** Its values are kept in the table like any preset's, and
  last as long as the scope's values: they are saved, and restored when the file is loaded. For a new game, and once
  the scope is reset, they are undefined.
- **Selecting a preset** sets the group's value, and notifies the group's key and each member whose value changed, in
  one notification. Selecting the modifiable preset while its values are undefined defines them from the preset
  selected before it, so Custom starts from that mode; once defined, selecting it again restores them, so switching
  from Custom to Hard and back keeps the player's values.
- **Setting a member writes it into the modifiable preset,** which is selected if it is not. If a preset that is not
  modifiable was selected, the modifiable preset's values are first set to that preset's, replacing any it had, so the
  change is that preset with one value changed: setting a member under Hard is selecting Custom with Hard's values,
  then setting the member. Only an explicit selection of the modifiable preset restores its own values. In a group with no modifiable preset, setting a member is an error, and nothing is set. The menu edits a
  member only while the modifiable preset is selected; a script or the console may set one at any time.
- **A game that does not allow customization defines no modifiable preset.**
- **The table determines what is saved.** A selection is the group's value in `settings`, and a modifiable preset's
  defined values are in `presets`, whichever preset is selected. A member is a setting like any other, but has no value
  of its own to save.
- **Only the menu and the save list read the selected preset.** Other code reads the members: permadeath is read from its
  own setting, not from the mode.
- **Presets are data,** in the game's definitions.

### Mods' settings

- **A mod's settings are in the section `mod.<id>`.** The loader checks that each key a mod defines begins with it;
  the store does not interpret it.
- **A mod changes any other setting by a patch addressed by its full key.** A patch may change a setting's default,
  its menu fields and a data enumeration's values. It cannot change a type, a scope, whether a restart is required, or
  a group's members.
- **A patch addressed to a mod that is not loaded is skipped with a warning.**
- **The saved values of a mod that is not loaded are kept by the loader,** not the store. `game.json` keeps each mod's
  values under its entry in the game's mods, so the loader sets only those of loaded mods, keeps the others as text,
  and writes them back on save. A mod's group selections and modifiable preset values are kept the same way
  (`modding_plan.md`, `## A missing mod`).

## Open

The open items are tracked in `work_plan.md`, under `### Settings`.
