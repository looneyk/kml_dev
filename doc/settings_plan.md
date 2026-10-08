# Settings --- Plan

Date: 2026-10-05, revised 2026-10-06

How the game's settings are declared, stored, read and saved. There may be a hundred or more, from every subsystem of the
game. The open items are in `work_plan.md` (`### Settings`), and the work in **Keyed settings**.

## Terms

| Term | What it is | What others call it |
| --- | --- | --- |
| **setting** | One value the player or the game chooses, such as the difficulty mode or the video mode | *option* in NetHack and DCSS; *preference* in Apple's and Java's APIs and Unity's `PlayerPrefs`; *console variable* (*cvar*) in Quake, Source and Unreal when the console can set it |
| **key** | A setting's name, the string the store maps: `"gameplay.random_seeding_on_open"` | *key* in Qt, ini files, .NET, Unity and Godot's `ConfigFile`; *name* in Godot's `ProjectSettings` (whose docs also call it a path), and for a cvar or a `ConVar`; *option name* in NetHack and DCSS |
| **subsystem** | A library or host with settings of its own, which declares and applies them: the sim, a view, a host, the log | *module* in Unreal and Godot; *subsystem* in Unreal's `USubsystem` classes |
| **section** | A leading run of a key's segments, before its last `.`: `video.window.width` is in `video` and `video.window`. In stratum, a key's outermost section is the subsystem the setting belongs to: `gameplay`, `video` | *section* in ini files, Unreal's config, Godot's `ConfigFile` and .NET; *group* in Qt; *node* in Java's `Preferences`; a dotted prefix by convention for cvars, `r.` for rendering |
| **store** | The one map from key to value, the `Settings` that `settings::store()` returns, and the only place a value is kept | *ProjectSettings* in Godot; *GConfig*, the config cache, in Unreal; the *console manager* for Unreal's cvars and the *cvar registry* for Source's |
| **declaration** | What defines a setting once: its key, type, default and scope | `GLOBAL_DEF` in Godot; a `TAutoConsoleVariable` in Unreal and a `ConVar` in Source; a `GameOption` in DCSS; an entry in NetHack's options table |
| **default** | The value a setting has until something sets it | *default* everywhere |
| **preset** | One of the values a setting allows, keyed by the setting and the value, with the settings that choosing it sets | a *level* of an Unreal scalability group, `[ShadowQuality@2]`; a *preset* in an options menu, "graphics preset: High" |
| **apply** | A subsystem reading its settings from the store into structures of its own | *apply* in Unreal's `ApplySettings`; a `ConVar`'s *change callback* in Source does it for one setting |
| **change notification** | The store telling the subsystem that declared a setting that it changed, so the subsystem applies its section again | *change callback* in Source (`FnChangeCallback_t`); the `settings_changed` signal of Godot's `ProjectSettings` |
| **restart required** | A setting that takes effect only when the program starts again | `restart_if_changed` in Godot; `ConfigRestartRequired` in Unreal; *requires restart* in options menus |
| **scope** | Whether a setting is a game's, saved in `game.json`, or the user's, saved in the user settings file: `SettingScope::Game` or `SettingScope::User` | NetHack's `flags` and `iflags`; Source's `FCVAR_ARCHIVE` flag; Godot's project settings and editor settings; the *layers* of Unreal's config: engine, project, platform and user |
| **user settings file** | `settings.json` in the user directory, unless the config file puts it elsewhere (`game_file_spec.md`): the file the user settings are saved in | *config.cfg* in Source; *GameUserSettings.ini* in Unreal; a `ConfigFile` in `user://` in Godot; the *options file* or *rc file*, `.nethackrc` and `init.txt`, in the roguelikes |

A key's sections are separated by `.`, as the `RegionSpec` and `ZoneSpec` keys already are (`zone.level`,
`parameters.seed`), and as Unreal's console variables (`r.ScreenPercentage`), VS Code's settings and Lua's table access
separate theirs. Godot's `ProjectSettings` and Qt's `QSettings` use `/`, .NET's configuration `:`, and Source's cvars an
`_` prefix (`sv_cheats`).

`Config` is not a setting. It is the program's fixed configuration, such as its directories, which the player does not
choose.

## Where it is now

- **The store is built** (`### The store`), and nothing declares a setting on it yet.
- **`stratum_sim::Settings` is a class with a getter and a setter for each setting,** and `stratum_sim::settings()`
  returns the one process-wide instance, constructed on first use. Its only setting is `random_seeding_on_open`.
- **Nothing saves the settings.** `game.json` has a `settings` section, which `Game` neither writes nor reads.

## What the settings are

- **Each subsystem has settings of its own.** Play and generation have the difficulty mode, the notation and
  how a save is opened; the views have video settings; the hosts have audio and controls. The sim must not depend on
  the view's or the hosts' settings.
- **A setting's scope is a game or the user.** A game setting, such as the difficulty mode or the notation,
  belongs to one game, is saved in its `game.json`, and is set by `Game` when the game is created or opened. A user
  setting, such as video, audio or controls, belongs to the player and the machine, is the same in every game, and is
  saved in the user settings file.
- **A new game's settings are the declared defaults,** whoever plays it and whatever an earlier game chose.
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
- **A change reaches the subsystem that declared the setting,** which applies its whole section again, since settings such
  as the video mode, the resolution and the UI scale take effect together, as Unreal's `ApplySettings` applies them.
- **The store is the one source of truth.** Saving reads the store; a subsystem's structures are never written back to it.

## Typed structures or keyed values

There are two ways to hold them.

- **Typed structures for each layer.** Each library has a class of its own settings as typed members, with a getter
  and a setter for each: `SimSettings` in `stratum_sim`, `ViewSettings` in `stratum_view`, and the hosts' own. Each is
  saved by code that names each member.
- **Keyed values in one store.** One store, in a library below the sim, holds every setting's value under its key,
  such as `gameplay.random_seeding_on_open` or `video.fullscreen`. Each setting has one declaration, in data or in
  C++, and each layer reads only the settings of its own sections.

### What other games do

- **NetHack** has typed global structures, `flags` and `iflags`, and a table that maps each option's name to its field
  and type, through which the options file is read and the options menu is drawn.
- **Dungeon Crawl Stone Soup** has one typed global, `Options`, a `game_options` with a member for each option. Each
  option is also registered as a `GameOption` object, a `BoolGameOption` or a `ColourGameOption` among them, with its
  name, its default and the member it sets, and the options file is read through that list.
- **Unreal Engine** has both. `UGameUserSettings` and each `UDeveloperSettings` subclass are typed classes whose
  `config` properties the engine saves to an ini section by reflection, and lists in the editor's Project Settings.
  Console variables are keyed: `TAutoConsoleVariable<int32> CVarExample(TEXT("r.Example"), 1, TEXT("help"))` declares
  one, once, as a typed C++ object under its name. Code reads it through that object, and an ini or the console sets it
  by name.
- **The Source engine** declares each setting as a `ConVar`, `ConVar sv_cheats("sv_cheats", "0", flags, "help")`, a
  typed C++ object under its name. Its flags give its scope: `FCVAR_ARCHIVE` saves it to the player's `config.cfg`.
- **Godot** has `ProjectSettings`, a keyed store read with `get_setting("display/window/size/viewport_width")`.
  `GLOBAL_DEF` declares a setting with its default, and its type and hints draw it in the editor. A game's own player
  settings are usually a `ConfigFile` of sections and keys, saved to `user://`.
- **Qt** has `QSettings`, keyed by `group/key`, each value a `QVariant` read with a default at the call. **Unity** has
  `PlayerPrefs`, keyed by name, with `GetInt`, `GetFloat` and `GetString`, and no declaration at all.

The structured games, NetHack, DCSS and Unreal's settings classes, each keep a table of names beside their fields,
since a file, a menu and a console all reach a setting by name. Unreal's console variables and Source's `ConVar`s
start from the name, and declare each setting as a typed C++ object for the code that reads it.

### Pros and cons

| | Typed structures for each layer | Keyed values in one store |
| --- | --- | --- |
| **Adding a setting** | A member, a getter, a setter, and its entry in the code that saves it, in C++ | One declaration, in data or one line of C++ |
| **A mod's settings** | Impossible without C++ | Declared in data like any other (`modding_plan.md`) |
| **Saving and loading** | Code for each member, or a name table beside the members, as NetHack and DCSS keep | One loop over the store, by key |
| **An options menu** | Written by hand, or drawn through the same name table | Drawn from the declarations, as Godot's and Unreal's editors draw theirs |
| **Scripts** | Each setting exposed by hand | Read by key, as data |
| **A wrong name or type** | A compile error | A compile error when C++ reads through a declaration; a runtime error, which an assert catches, when it reads by a key string |
| **Reading cost** | A member read | A map lookup, made only when a subsystem applies its settings |
| **Tools** | Completion and refactoring see every setting | They see each declaration in C++, and not those in data |
| **Ownership by layer** | Each structure in its own library; nothing shared | One store below the sim; each layer declares and reads only its own sections |
| **Scope** | Which structure a setting is in, or a member's annotation | Part of each declaration, as a `ConVar`'s flags give it |

The typed structures check at compile time, but each setting is C++ in three places, mods cannot add any, and a name
table has to be built beside them anyway, for the file and the menu. The keyed store makes a setting data, which is
the design goal (`modding_plan.md`), and a declaration in C++ gives back the compile-time check for the settings C++
reads.

### Recommendation

**Keyed values in one store, each setting with one declaration,** as Unreal's console variables and Source's `ConVar`s
have them. A setting that C++ reads is declared in C++, in the library of the layer it belongs to; one that only data
and scripts read is declared in data. The store keeps every setting's value; each subsystem applies its own
into structures of its own, and applies them again on a change notification.

```cpp
// stratum_sim: the setting's one declaration, with its key, default and scope
inline const Setting<RandomSeedingOnOpen> random_seeding_on_open
{
    "gameplay.random_seeding_on_open", RandomSeedingOnOpen::RestoreSavedState, SettingScope::User
};

// Game applies it, when a save is opened: the store looks up the key, and returns the value as a RandomSeedingOnOpen
if (settings::store().value(random_seeding_on_open) == RandomSeedingOnOpen::Reseed)

// The options menu changes it
settings::store().set_value(random_seeding_on_open, RandomSeedingOnOpen::Reseed);
```

The user settings file has only the value, under its key:

```json
{ "gameplay.random_seeding_on_open": "reseed" }
```

A mod's setting, which no C++ reads, is declared in data, and a script reads it by key, `settings.value("torches.flicker")`:

```json
{ "key": "torches.flicker", "type": "bool", "default": "true", "scope": "user" }
```

A declaration's type is a registered type name: `bool`, `integer`, `real` and `text` are registered by the store, and a
library registers the name of each enumeration its settings have, such as `difficulty_mode`, so a mod can declare a
setting of it.

C++ can read any setting by its key too, as text, `settings::store().text("gameplay.random_seeding_on_open")`, but then the
key is written and the text converted at every read, and a misspelling is found only when it runs. Reading through the
declaration writes the key and the type once, so a misspelled name is a compile error.

### The store

The library is `library/quetzal/settings/`, the target `quetzal_settings` in the namespace `quetzal::settings`, with
`settings_test`. It links only `quetzal_common`.

```cpp
namespace quetzal::settings
{

    enum class SettingScope { User, Game };

    // A type a setting may have: one that std::format writes, and that quetzal::try_type reads back through from_chars.
    // An enumeration's setting needs a std::formatter specialization and a from_chars overload in its namespace
    template<typename T>
    concept SettingValue = std::formattable<T, char> && (std::same_as<T, std::string> || /* from_chars(first, last, value) */);

    // A setting's one declaration in C++, which holds no value
    template<SettingValue T>
    struct Setting
    {
        std::string_view key;      // Its sections and its name, separated by '.': "gameplay.random_seeding_on_open"
        T default_value {};
        SettingScope scope = SettingScope::User;
        bool restart_required = false;
    };

    // Each section the key is in, from the outermost: "video.window.width" is in "video" and "video.window"
    std::vector<std::string_view> sections_of(std::string_view key);
    bool is_setting_key(std::string_view key);

    // What the store keeps for every declared setting, whether declared in C++ or in data
    struct SettingDeclaration
    {
        std::type_index type;      // The C++ type of the setting's value
        std::string default_text;
        SettingScope scope;
        bool restart_required;
        std::function<bool(std::string_view text)> accepts;
    };

    class Settings
    {
    public:

        template<SettingValue T> void register_type(std::string_view type_name);   // for declarations in data
        template<SettingValue T> void declare(const Setting<T>& setting);
        void declare(std::string_view key, std::string_view type_name, std::string_view default_text, SettingScope scope,
            bool restart_required);                                                 // from data

        template<SettingValue T> T value(const Setting<T>& setting) const;           // applying
        template<SettingValue T> void set_value(const Setting<T>& setting, const T& value);

        std::string text(std::string_view key) const;                               // by key, for scripts
        void set_text(std::string_view key, std::string_view text);

        KeyValueStore values(SettingScope scope) const;                             // saving
        void set_values(const KeyValueStore& values);                               // loading
        void reset(SettingScope scope);                                             // a game cleared

        void add_section_listener(std::string_view section, std::function<void()> apply);
    };

    // The process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings
```

**How it is used:**

- **At startup, each subsystem registers its types and declares its settings,** calling
  `settings::store().register_type<T>(name)` for each enumeration its settings have, and `declare(...)` with its own
  `Setting<T>` constants. Then the settings declared in data are loaded, which may give any registered type name.
- **Loading sets the values,** with `set_values`: from the user settings file at startup, and from `game.json`'s
  `settings` when a game is opened. A key that is not declared, or text that is no value of its setting's type, is an
  error, and none of the values is set.
- **Each subsystem applies its section** with `value(...)`, into structures of its own, and registers the same function
  with `add_section_listener`. A change notifies each section the changed key is in, the outer sections as well as the
  inner, once for each change, after the values have changed; setting a value it already has notifies nothing.
- **Saving writes the values set in each scope,** `values(SettingScope::Game)` into `game.json`, and
  `values(SettingScope::User)` into the user settings file, each value as text. A setting never set is not written, so
  it keeps its declared default, even if the default changes.
- **A game cleared** returns the game scope to its defaults with `reset(SettingScope::Game)`.

**What the store decides:**

- **A value is kept as text, in the store and in a file, and converted on each read,** as ini files, Source's
  `config.cfg`, Minecraft's `options.txt` and Qt's `QSettings` keep theirs. The store needs no `std::variant` or
  `std::any`, and a setting is read only when it is applied, so the conversion costs nothing that matters.
- **A type is described only by its formatting and parsing.** `quetzal::to_string` writes a value through
  `std::format` and `quetzal::try_type` reads it through `from_chars`, as `KeyValueStore` reads its values, so an
  enumeration that is a setting works with `std::format`, the log and `KeyValueStore` too. Its formatter's `format` is a
  template over the context, since `std::formattable` checks it with the standard's own.
- **A declaration has the C++ type of its value,** as a `std::type_index`, so reading a setting with the wrong type
  asserts, whether it was declared in C++ or in data.
- **A value whose key is not declared is an error,** reported when its file is loaded, so a misspelled key in a file
  or a mod is found at once.
- **A change is notified for a section, not for each setting,** so a subsystem applies all of its settings again, as
  Unreal's `ApplySettings` applies them together. Sections nest, as Qt's groups and .NET's sections do, so a listener
  can be as broad as `video` or as narrow as `video.window`.

### Presets

Presets are not designed yet. This section states the problem; the model is chosen once its questions are answered.

**What is needed:**

1. **Settings:** named, typed values with a default and a scope, which the store keeps and subsystems apply.
2. **A named choice that sets a group of other settings.** Choosing Hard sets several multipliers at once, and a video
   quality preset sets several video settings.
3. **A record of which choice was made,** so the menu can show "Hard" and the game can save it.
4. **Custom:** no choice applies, and the group's settings are edited one at a time, starting from the last choice's
   values.
5. **Display:** the menu may show what each choice sets.

**Where the first design tangled:**

- **It made the choice a setting, and the presets its values.** A preset is not a value of a type, as `"1.5"` is; it is
  a bundle of other settings' values. Making it a value put presets into the setting's declaration, which raised the
  questions of default presets and of defaults taken from presets.
- **The defaults of the grouped settings,** the multipliers, had two candidate homes: their own declarations, or the
  default preset.
- **Which of the choice and the grouped settings is the state was unclear.** The sim reads only the multipliers; the
  mode exists for the menu and the save.

**The questions that define it:**

1. **What is saved: the choice's name, the grouped settings' values, or both?** It matters when an update rebalances
   Hard: a saved `"hard"` takes the new values in an old game, and saved values keep the old ones. Custom has to save
   the values whichever is chosen.
2. **Is the choice something code reads, or only the menu and the save?** If only those, it is metadata of the group
   rather than a setting a subsystem applies.
3. **Is a group always a set of settings chosen together, and can a setting belong to two groups?** Unreal forbids it:
   each console variable is in at most one scalability group.
4. **Where do the grouped settings' defaults come from:** the default preset, or each setting's own?
5. **Are presets only data,** in the game's file and mods', or can C++ define them too?

**How Unreal frames it:** a scalability group, `sg.ShadowQuality`, is a concept of its own, apart from the console
variables it sets. The group has a current level, and each level, an ini section such as `[ShadowQuality@2]`, lists
values for the group's member variables. The group is not one of its members, and its levels are not values of any
member. Separating the group from the settings it sets untangles most of the above.

## Open

The open items are tracked in `work_plan.md`, under `### Settings`.
