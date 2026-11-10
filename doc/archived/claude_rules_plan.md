# Claude Rules --- Plan

Date: 2026-11-04

Claude's local memory for this repository contains rules that every contributor and every Claude session should follow.
The split moves those rules into checked-in files under `.claude/rules/`, each loaded for the project it belongs to, and
leaves the local memory with what concerns one user and one machine.

## Loading

The behavior below is from the Claude Code memory documentation (`https://code.claude.com/docs/en/memory.md`).

- **A rule file without `paths` frontmatter loads at launch,** with the same priority as `.claude/CLAUDE.md`.
- **A rule file with `paths` loads when Claude reads, writes, or edits a matching file,** including a single-file read
  through `cat` or `head`. It does not load at launch.
- **`paths` is a YAML list of globs relative to the repository root.** `**`, `*`, brace groups such as `{hpp,cpp}`, and
  bracket expressions are supported, up to 1,000 expanded patterns for one rule.
- **Files in subdirectories of `.claude/rules/` are found recursively.**
- **A rule file loaded on demand is read up to 40,000 characters** with a 200K context, and the rest is cut off.

## Destinations

| File | Loads | Contents |
| --- | --- | --- |
| `CLAUDE.md` | At launch | Unchanged: the projects, scope, build, tests, conventions, documents, and git |
| `.claude/rules/writing.md` | At launch | Naming, wording, terminology, and comments, which apply to code, docs, and replies alike |
| `.claude/rules/code.md` | `**/*.{hpp,cpp,h,inl}`, `**/CMakeLists.txt` | C++ and library conventions |
| `.claude/rules/docs.md` | `doc/**/*.md` | Plan and reference documents |
| `.claude/rules/stratum.md` | The stratum directories, and `doc/game_system_*.md` | Stratum design rules and terms |
| `.claude/rules/model_suite.md` | `model/**`, `library/quetzal/{brep,model,geometry,triangulation,wavefront_obj,svg,xml,pdf}/**`, `utility/**` | Model suite facts |
| `doc/vulkan.md` | Read on request | The Vulkan architecture notes, as reference |
| Local memory | Per user | How Claude works with this user, and facts about this machine |

`doc/game_system_reference.md` is in the stratum paths because CLAUDE.md directs stratum work to read it first, so
documentation work on stratum loads the stratum rules too.

The stratum paths:

```yaml
paths:
  - "library/quetzal/stratum_*/**"
  - "library/quetzal/{dungen,catagen,godot}/**"
  - "application/stratum_*/**"
  - "tool/{dungen,catagen}/**"
  - "test/{stratum,dungen,catagen}*/**"
  - "doc/game_system_*.md"
```

## Assignment

| Memory | Destination |
| --- | --- |
| `feedback_comments`, `feedback_naming`, `feedback_wording`, `feedback_terminology`, `feedback_dont_document_incidental_limits` | `writing.md` |
| `feedback_auto_for_loop_variables`, `feedback_avoid_pointers_variant_any`, `feedback_binary_io`, `feedback_code_mathematically_precise`, `feedback_constrained_template_parameters`, `feedback_cross_platform_code`, `feedback_explicit_return`, `feedback_follow_oss_conventions`, `feedback_general_libraries_client_agnostic`, `feedback_general_support_from_start`, `feedback_generic_settings_no_per_setting_code`, `feedback_header_organization`, `feedback_international_datetime`, `feedback_invariants_are_asserts`, `feedback_log_every_throw`, `feedback_logging_over_aggregate_counts`, `feedback_no_small_file_splits`, `feedback_no_special_case_accumulation`, `feedback_optional_over_sentinel`, `feedback_remove_no_value_code`, `feedback_temporary_defaults_at_top` | `code.md` |
| `feedback_docs` | `docs.md` |
| `feedback_state_not_interpretation`, `feedback_stratum_facing_qualified`, `feedback_format_version_changes`, `project_modding_design_goal`, `project_procgen_beyond_dungeons`, `project_wall_notation_cell_view`, and every `project_stratum_*` except the two below | `stratum.md` |
| `feedback_derive_from_data_not_guesses`, `project_intersection_ordering_variant_a`, `project_brep_subsystem_status`, `project_filevalidator_mesh_diff_boundary`, `project_debug_asserts_invisible` | `model_suite.md` |
| `intersection_splitter` | A reference doc, `doc/intersection_splitter.md`, at 13,000 characters too long to load with every model file |
| `quetzal` | `doc/vulkan.md`, the KTX build and texture loading, and `doc/development_setup.md`, `script/build_startup_project.ps1`; the rest no longer matches the code |
| The rule in `feedback_fixing_without_asking` that an intended output change copies `out/` over `reference/` and reruns | `stratum.md` |
| The rule in `feedback_replies` that a finding names the model and the test | `model_suite.md` |
| `project_stratum_docs`, `project_mesh_operations_run_directory` | Deleted: CLAUDE.md already contains both |
| `feedback_when_to_act`, `feedback_replies`, `feedback_fixing_without_asking`, `feedback_work_units`, `feedback_tools`, `feedback_scope`, `feedback_no_false_attribution`, `feedback_dont_overgeneralize_single_decisions`, `project_stratum_map_term` | Local memory |
| `project_crawler_cat_game`, `project_dungeon_library_unreal` | Local memory |
| `reference_godot_executable` | Deleted: `doc/development_setup.md` contains the Godot path |

## Conversion

- **A rule is written as a project rule,** in the present tense or the imperative, with no mention of the user, of
  Claude, of a session, or of a date.
- **A rationale stays as one sentence** where the rule would look arbitrary without it, and an anecdote goes.
- **Content that CLAUDE.md already contains is left out.**
- **History goes:** the fixed camera bug in `quetzal`, and absolute paths, such as the `resource_path` in `toot1`.
- **Each moved memory is deleted,** and its line removed from `MEMORY.md`. `MEMORY.md` keeps the local entries and one
  line saying the project rules are in `.claude/rules/`.
- **The local project indexes, `stratum.md` and `mesh_operations.md`, are deleted.** Their two local entries, the
  user's "map" shorthand and leaving the model regression suite to the user, move into `MEMORY.md`.
- **`.gitignore` admits the rules,** with `!/.claude/rules/` and `!/.claude/rules/**` beside the entries for
  `.claude/agents/` and `.claude/hooks/`.
- **The loading is checked** with `/context all` after reading a file in each project.

## Order

1. The rule files, `.gitignore`, `doc/intersection_splitter.md`, and the additions to `doc/vulkan.md`, for review.
2. The moved memories deleted and `MEMORY.md` reduced, once the rule files are accepted.
