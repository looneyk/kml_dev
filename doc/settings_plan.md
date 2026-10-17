# Settings --- Plan

Date: 2026-10-05, revised 2026-10-16

How the game's settings are defined, stored, read and saved. There may be a hundred or more, from every subsystem of the
game. The open items are in `work_plan.md` (`### Settings`), and the work in **Keyed settings**.

## Terms

| Term | What it is | What others call it |
| --- | --- | --- |
| **setting** | One value the player or the game chooses, such as a difficulty multiplier or the video mode | *option* in NetHack and DCSS; *preference* in Apple's and Java's APIs and Unity's `PlayerPrefs`; *console variable* (*cvar*) in Quake, Source and Unreal when the console can set it |
| **key** | A setting's name, the string the store maps: `"gameplay.random_seeding_on_open"`. The store compares a key only as a whole string | *key* in Qt, ini files, .NET, Unity and Godot's `ConfigFile`; *name* in Godot's `ProjectSettings` (whose docs also call it a path), and for a cvar or a `ConVar`; *option name* in NetHack and DCSS |
| **subsystem** | A library or host with settings of its own, which registers their C++ types and applies them: the sim, a view, a host, the log | *module* in Unreal and Godot; *subsystem* in Unreal's `USubsystem` classes |
| **section** | A leading run of a key's `.`-separated segments, by naming convention only: `video.window.width` is in `video` and `video.window`. In stratum, a key's outermost section is the subsystem the setting belongs to, and a mod's settings are in `mod.<id>`. Neither the store nor a settings group interprets a section | *section* in ini files, Unreal's config, Godot's `ConfigFile` and .NET; *group* in Qt, which is unrelated to a settings group; *node* in Java's `Preferences`; a dotted prefix by convention for cvars, `r.` for rendering |
| **store** | The one map from key to setting, the `Settings` that `settings::store()` returns, and the only place a value is kept | *ProjectSettings* in Godot; *GConfig*, the config cache, in Unreal; the *console manager* for Unreal's cvars and the *cvar registry* for Source's |
| **definition** | A setting's one entry in data: its key, type, default and scope, and whether a restart is required | a `bool-setting` or `string-setting` prototype in Factorio's mod settings; `GLOBAL_DEF` in Godot; a `TAutoConsoleVariable` in Unreal and a `ConVar` in Source, each in C++; a `GameOption` in DCSS; an entry in NetHack's options table |
| **type** | A registered name for the values a setting may have: `boolean`, `integer`, `real` or `text`, which the store registers; an enumeration that a subsystem registers in C++; or an enumeration defined in data | the *setting type* of a Factorio prototype; a *simple type* in XML Schema |
| **base type** | The built-in type a registered type is based on: `text` for every enumeration. A script reads a setting as a value of its base type | the *base* of an XML Schema restriction, `<xs:restriction base="xs:string">` |
| **default** | The value a setting has until something sets it, from the setting's definition | *default* everywhere |
| **settings group** | A set of settings that its presets assign together, with one preset chosen at a time. Its members are the settings its presets set | a *scalability group* in Unreal, `sg.ShadowQuality` |
| **preset** | A set of values for a settings group's settings, defined in data. A group's default preset is the one chosen for a new game. An editable preset, such as Custom, has values the player sets | a *level* of an Unreal scalability group, `[ShadowQuality@2]`; a *preset* in an options menu, "graphics preset: High" |
| **apply** | A subsystem reading its settings from the store into structures of its own | *apply* in Unreal's `ApplySettings`; a `ConVar`'s *change callback* in Source does it for one setting |
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

## Where it is now

- **`stratum_sim::Settings` is a class with a getter and a setter for each setting,** and `stratum_sim::settings()`
  returns the one process-wide instance, constructed on first use. Its only setting is `random_seeding_on_open`.
- **Nothing saves the settings.** `game.json` has a `settings` section, which `Game` neither writes nor reads.

## What the settings are

- **Each subsystem has settings of its own.** Play and generation have the difficulty multipliers, the notation and
  how a save is opened; the views have video settings; the hosts have audio and controls. The sim must not depend on
  the view's or the hosts' settings.
- **A setting's scope is a game or the user.** A game setting, such as a difficulty multiplier or the notation,
  belongs to one game, is saved in its `game.json`, and is set by `Game` when the game is created or opened. A user
  setting, such as video, audio or controls, belongs to the player and the machine, is the same in every game, and is
  saved in the user settings file.
- **A new game's settings are the defaults, with each settings group's default preset chosen,** whoever plays it and
  whatever an earlier game chose.
- **Any setting may change at any time,** a user setting while a game is open and a game setting during play, as the
  difficulty mode may (`difficulty_plan.md`). A game setting changes between advances, never partway through a turn.
- **Each subsystem applies its settings into structures of its own,** and works from those, never from the
  store. The sim builds the difficulty multipliers it applies on every hit, a host its key map from the bindings, the
  log its levels, and the view its video mode. A subsystem reads the store only when it applies its settings: when it
  starts, when a game is created or opened, and when one of its settings changes. So reading a setting is never
  frequent, and what a read costs does not matter.
- **This is how the engines treat their settings.** Unreal's `UGameUserSettings::ApplySettings` pushes the stored
  settings into the renderer and audio, which work from their own state. Godot's servers read `ProjectSettings` when
  they start. DCSS reads its key bindings into keymaps, which a key press consults. A Source `ConVar` can have a change
  callback, which rebuilds what depends on it. The roguelikes' gameplay options are the exception: NetHack reads
  `flags.verbose` and DCSS `Options.autopickup_on` wherever they are needed. A subsystem's structure may be a plain struct
  of the values it applied, which costs little more.
- **A change notifies every change listener** with the keys it changed, and each listener decides from the keys
  whether to act. Settings that take effect together, such as the video mode, the resolution and the UI scale, are
  applied together by one listener, as Unreal's `ApplySettings` applies them; another listener may watch one setting.
- **The store is the one source of truth.** Saving reads the store; a subsystem's structures are never written back to it.

## Typed structures or keyed values

There are two ways to hold them.

- **Typed structures for each layer.** Each library has a class of its own settings as typed members, with a getter
  and a setter for each: `SimSettings` in `stratum_sim`, `ViewSettings` in `stratum_view`, and the hosts' own. Each is
  saved by code that names each member.
- **Keyed values in one store.** One store, in a library below the sim, contains every setting's value under its key,
  such as `gameplay.random_seeding_on_open` or `video.fullscreen`. Each setting has one definition, and each layer reads
  only its own settings.

### What other games do

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
| **A wrong name or type** | A compile error | An assert at the read, which runs when the subsystem applies its settings at startup |
| **Reading cost** | A member read | A map lookup, made only when a subsystem applies its settings |
| **Tools** | Completion and refactoring see every setting | They see a key in C++ only as a string |
| **Ownership by layer** | Each structure in its own library; nothing shared | One store below the sim; each layer reads only its own settings |
| **Scope** | Which structure a setting is in, or a member's annotation | A field of each definition, as a `ConVar`'s flags and a Factorio prototype's setting type state it |

The typed structures check at compile time, but each setting is C++ in three places, mods cannot add any, and a name
table has to be built beside them anyway, for the file and the menu. The keyed store makes a setting data, which is
the design goal (`modding_plan.md`). Every read in C++ runs when its subsystem applies its settings at startup, so a
misspelled key or a wrong type is found then.

### Recommendation

**Keyed values in one store, each setting defined once, in data,** as Factorio defines a mod's settings, and read by
key, as Unreal's console manager, Source's cvar registry, Godot's `ProjectSettings` and Qt's `QSettings` are read. C++
defines no setting: it registers the types its settings have, and reads a setting by its key and its C++ type. The
store keeps every setting's value; each subsystem applies its own into structures of its own, and applies them again
when its change listener is called with any of their keys.

```json
{ "key": "gameplay.random_seeding_on_open", "type": "random_seeding_on_open", "default": "restore_saved_state", "scope": "user" }
```

```cpp
// stratum_sim registers the type of its setting, with the type's values, when it starts
settings::store().register_type<RandomSeedingOnOpen>("random_seeding_on_open", {"restore_saved_state", "reseed"});

// Game applies it, when a save is opened: the store asserts that the setting's type is a RandomSeedingOnOpen
if (settings::store().value<RandomSeedingOnOpen>("gameplay.random_seeding_on_open") == RandomSeedingOnOpen::Reseed)

// The options menu changes it
settings::store().set_value<RandomSeedingOnOpen>("gameplay.random_seeding_on_open", RandomSeedingOnOpen::Reseed);
```

The user settings file has only the value, under its key:

```json
{ "gameplay.random_seeding_on_open": "reseed" }
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
    };

    // What the store keeps for every setting
    struct Setting
    {
        std::string type_name;
        std::string default_value;
        SettingScope scope;
        bool restart_required;
        std::optional<std::string> value;          // If empty, has its default value
    };

    struct Preset
    {
        std::string name;
        std::string display_name;
        bool editable = false;
        KeyValueStore values;                      // Empty for an editable preset
    };

    struct SettingsGroup
    {
        SettingScope scope;
        std::string default_preset_name;
        std::string preset_name;                   // The chosen preset
        std::vector<Preset> presets;
    };

    class Settings
    {
    public:

        template<SettingValue T>
        void register_type(std::string_view type_name, std::vector<std::string> values = {});        // from C++
        void register_enumeration(std::string_view type_name, std::vector<std::string> values);       // from data
        bool registered_type(std::string_view type_name) const;
        const SettingType& type(std::string_view type_name) const;

        void insert(std::string_view key, std::string_view type_name, std::string_view default_value, SettingScope scope,
            bool restart_required);
        bool contains(std::string_view key) const;
        const Setting& setting(std::string_view key) const;

        template<SettingValue T> T value(std::string_view key) const;                                // applying
        template<SettingValue T> void set_value(std::string_view key, const std::type_identity_t<T>& value);

        std::string text(std::string_view key) const;                                                // by key, as text
        void set_text(std::string_view key, std::string_view text);

        void insert_group(std::string_view key, SettingScope scope, std::string_view default_preset_name);
        void insert_preset(std::string_view group_key, const Preset& preset);
        const SettingsGroup& group(std::string_view group_key) const;
        void choose_preset(std::string_view group_key, std::string_view preset_name);

        KeyValueStore values(SettingScope scope) const;                                              // saving
        void set_values(const KeyValueStore& values);                                                // loading
        void reset(SettingScope scope);                                                              // a game cleared

        void add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener);
    };

    // The process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings
```

**How it is used:**

- **At startup, each subsystem registers its types,** calling `settings::store().register_type<T>(name, values)` for
  each enumeration its settings have. Then the loader reads the definitions, the game's and then each mod's, and
  registers their enumerations and inserts their settings, groups and presets.
- **Loading sets the values,** with `set_values`: from the user settings file at startup, and from `game.json`'s
  `settings` when a game is opened. A key that is neither a setting nor a group, or text that is no value of its
  setting's type, is an error, and none of the values is set.
- **Each subsystem applies its settings** with `value<T>(key)`, into structures of its own, and adds a change listener
  that applies them again when any of their keys is among the keys changed.
- **Saving writes the values set in each scope,** `values(SettingScope::Game)` into `game.json`, and
  `values(SettingScope::User)` into the user settings file, each value as text. A setting never set is not written, so
  it keeps its default, even if the default changes. A group is written as its chosen preset's name under the group's
  key, and its members' values are written only when that preset is editable.
- **A game cleared** returns the game scope's settings to their defaults, and then chooses its groups' default
  presets, with `reset(SettingScope::Game)`, in one operation.

**What the store decides:**

- **A value is kept as text, in the store and in a file, and converted on each read,** as ini files, Source's
  `config.cfg`, Minecraft's `options.txt` and Qt's `QSettings` keep theirs. The store needs no `std::variant` or
  `std::any`, and a setting is read only when it is applied, so the conversion costs nothing that matters.
- **A C++ type is described only by its formatting and parsing.** `quetzal::to_string` writes a value through
  `std::format` and `quetzal::try_type` reads it through `from_chars`, as `KeyValueStore` reads its values, so an
  enumeration that is a setting works with `std::format`, the log and `KeyValueStore` too. Its formatter's `format` is a
  template over the context, since `std::formattable` checks it with the standard.
- **An enumeration registered in C++ lists its values,** so the menu can offer them. Registering it checks that each
  value parses.
- **A setting has its type's C++ type,** so reading a setting with the wrong type asserts, whatever registered it.
- **A value whose key is not a setting or a group is an error,** reported when its file is loaded, so a misspelled key
  in a file or a mod is found at once.
- **A key is opaque.** The store compares only whole keys, and requires only that a key is not empty.
- **A change is notified once for each operation,** after every value it changes has changed, with all of their keys,
  so loading a file, resetting a scope or choosing a preset notifies once. Setting a value it already has notifies
  nothing. Which keys matter is the listener's to decide.

### Settings groups

A settings group is a concept of its own, apart from the settings it sets, as Unreal's scalability groups are. The
difficulty mode is one (`difficulty_plan.md`), and a video quality preset another.

- **A group has a key, a scope and its presets, one of which is its default.** The group's key is in the settings' key
  space, and its chosen preset's name is saved under it: `"gameplay.difficulty": "hard"`.
- **The group's members are the settings its presets set.** Each member has its own default, in its definition, like
  any other setting. The default preset is a set of values like any other, and is the preset chosen for a new game and
  when the group's scope is reset; it has no effect on the members' defaults.
- **Each member has the group's scope, and a setting is a member of at most one group,** as each Unreal console
  variable is in at most one scalability group. Inserting a group or a preset checks both.
- **Choosing a preset** sets every member to the preset's value and the group's chosen preset to it, in one operation.
  A member the preset leaves out returns to its default. The keys notified include the group's key.
- **A group has at most one editable preset, such as Custom, which has no values in data.** Choosing it keeps the
  members' current values, which the player then edits. Choosing another preset writes over them, and choosing the
  editable preset again starts from that preset's values.
- **Setting a member while a preset that is not editable is chosen switches the group to its editable preset,** in
  the same operation, so a chosen preset that is not editable always matches its members. The menu edits a member
  only while the editable preset is chosen; a script or the console may set one at any time. In a group with no
  editable preset, setting a member is an error.
- **A game that does not allow customization defines no editable preset.**
- **What is saved depends on the preset.** For a preset that is not editable, only its name is saved, so a rebalanced
  Hard reaches old Hard games. For an editable preset, its name and every member's value are saved, and a member
  added since the save has its default.
- **Only the menu and the save list read the chosen preset.** Other code reads the members: permadeath is read from its
  own setting, not from the mode.
- **Presets are data,** in the game's definitions and mods'.

### Mods' settings

- **A mod's settings are in the section `mod.<id>`.** The loader checks that each key a mod defines begins with it;
  the store does not interpret it.
- **A mod changes any other setting, group or preset by a patch addressed by its full key.** A patch may change a
  setting's default, its menu fields and a data enumeration's values, a preset's values, and a group's presets. It
  cannot change a type, a scope or whether a restart is required.
- **A patch addressed to a mod that is not loaded is skipped with a warning.**
- **The saved values of a mod that is not loaded are kept by the loader,** not the store. `game.json` keeps each mod's
  values under its entry in the game's mods, so the loader sets only those of loaded mods, keeps the others as text,
  and writes them back on save.

## Open

The open items are tracked in `work_plan.md`, under `### Settings`.
