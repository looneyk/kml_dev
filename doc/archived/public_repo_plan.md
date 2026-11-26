# Public Repository --- Plan

Date: 2026-10-28

This document lists what is put in place before the repository is made public: the licenses of the code, the data, and
the art, the REUSE files that state them per path, a review of what the MIT license would cover, and the cleanup of the
git history.

## Order

1. Scan the history for secrets (`## History`).
2. Decide between rewriting the history and publishing a fresh one, and do it.
3. Review the files that the catch-all MIT annotation applies to (`## Review`).
4. Add `LICENSES/` and `REUSE.toml`, and run `reuse lint` until it passes (`## REUSE`).
5. Update the README's license sentence (`## README`).
6. Make the repository public.

The art notice and its credits file are added with the first asset set, since `assets/art/` does not exist yet.

## Licenses

| What | Where | License |
| --- | --- | --- |
| Code, tools, tests, documents | everything not listed below | MIT, the root `LICENSE` |
| Game data: region specs, types, setting definitions, manifests | `assets/data/` | MIT, as the code is |
| Art and audio | `assets/art/` | All rights reserved, with permission for mods |
| Third-party assets | listed in `assets/art/CREDITS.md` | Their authors' licenses |
| Third-party libraries | fetched into `library3/` at build time, not tracked | Their authors' licenses, not declared here |

- **The root `LICENSE` stays the plain MIT text,** `Copyright (c) 2025 Keith Looney`, so that GitHub detects the license.
- **MIT is compatible with every library in `library3/`.** They are under MIT, BSD, zlib-style, Apache-2.0 (Vulkan Memory
  Allocator, `vk_framework`, parts of KTX-Software and of CDT), and MPL-2.0 (Eigen, CDT), each of which may be combined with MIT code.
  A binary distribution has to include their notices, in a third-party notices file that comes with the first release.
- **The art is all rights reserved,** so that selling the game stays possible: a license can be loosened later and
  never tightened for what is already released, since a Creative Commons grant cannot be revoked. If the game will not
  be sold, the art may move to CC BY 4.0, which permits reuse with attribution, or CC BY-SA 4.0, which also requires
  derivatives to keep the license. NonCommercial licenses are avoided, since "commercial" is ill-defined and they
  exclude the assets from open distributions.
- **A mod may use the art but not redistribute it,** modified or not. A mod refers to the files a player already has.
- **Game data is under MIT with the code,** as 0 A.D. splits it: data is closer to code than to art, and mods patch and
  inherit from it.
- **Each third-party asset keeps its license.** From the first asset set onward, `assets/art/CREDITS.md` lists each
  one with its source, its author, and its license, and `REUSE.toml` has an annotation for it.

### The art notice

`assets/art/LICENSE.md`:

```
Copyright (c) 2026 Keith Looney. All rights reserved.

The files in this directory and its subdirectories are not covered by the MIT license in the
repository's root LICENSE file, except where CREDITS.md lists a file under its own license.

You may use these files unmodified to build and run this project. You may make and distribute mods
for it that use these files, provided the mods do not include or redistribute the files themselves.
Any other copying, modification, or redistribution requires written permission.
```

The text of `LICENSES/LicenseRef-AllRightsReserved.txt` is the same notice.

A public GitHub repository lets every GitHub user view and fork its content within GitHub, whatever its license, so the
notice does not prevent forks there; it prevents use and redistribution elsewhere. Art that must not be forked at all
goes in a private repository or a private submodule instead.

## REUSE

The REUSE specification, from the FSF Europe and followed by the Linux kernel and KDE, states the copyright and license
of every file in a machine-readable form.

```
LICENSE                              the MIT text, which REUSE permits beside LICENSES/
LICENSES/
  MIT.txt                            each license's text, named by its SPDX identifier
  LicenseRef-AllRightsReserved.txt   a custom license is LicenseRef-<name>
REUSE.toml                           the copyright and license of each path
```

`REUSE.toml`, where the last matching annotation takes precedence, so the catch-all comes first:

```toml
version = 1

[[annotations]]
path = "**"
SPDX-FileCopyrightText = "2025 Keith Looney"
SPDX-License-Identifier = "MIT"

[[annotations]]
path = "assets/art/**"
SPDX-FileCopyrightText = "2026 Keith Looney"
SPDX-License-Identifier = "LicenseRef-AllRightsReserved"
```

A third-party asset has an annotation of its own after these, with its author and its license, and its license's text
in `LICENSES/`. The art annotation and the custom license are added with the first asset set; until then `reuse lint`
reports an unused license.

Source files have no SPDX headers. `REUSE.toml` states their license, and a header in each of about 1,500 tracked files
adds nothing to it. A binary file that needs its own statement has a `.license` file beside it, such as
`wall.png.license`.

The tool, which checks only the files git tracks:

```
pipx install reuse
reuse download MIT
reuse lint
```

`reuse spdx` writes a software bill of materials, if one is requested.

## Review

The catch-all puts every tracked file under MIT. Each of these is checked before publishing, and given an annotation
of its own or removed if MIT should not cover it:

- **`model/`,** 884 tracked files, the geometry models included. They are all original work, so MIT applies to them.
- **`doc/`,** including the plans and the reference documents.
- **`CLAUDE.md`, `.github/copilot-instructions.md`, and `.claude/`.** These are committed on purpose, as Claude Code
  intends: `CLAUDE.md` documents the conventions, `.claude/hooks/` has the wording check, `.claude/agents/` the
  mesh_operations test agent, and `.claude/settings.json` the shared permissions and the hook. Personal permissions go
  in `.claude/settings.local.json`, which `.gitignore` excludes.
- **`.vscode/c_cpp_properties.json`,** for absolute paths or anything else local.

## README

The README's license sentence states the split and refers to `REUSE.toml`:

```
The code and the game data are under the MIT license (LICENSE). The art and audio in assets/art/ are
all rights reserved (assets/art/LICENSE.md), and third-party assets keep their own licenses
(assets/art/CREDITS.md). REUSE.toml states the license of each path.
```

## History

Making the repository public publishes every commit. The history has the earlier `.claude/settings.json`, whose
permissions included absolute paths with the user name and session scratchpad directories; it is in 2 of the 334
commits on `main`. The remote is `git@github.com:looneyk/dev.git`.

**The scan comes first,** since it decides how much has to be removed:

```
gitleaks detect
```

It reports tokens, keys, and similar secrets anywhere in the history.

### Rewriting the history

`git filter-repo`, which the git project recommends in place of `filter-branch`, removes the file from every commit,
and the clean version is committed again:

```
git clone --mirror git@github.com:looneyk/dev.git ../dev_backup.git
pipx install git-filter-repo
copy .claude\settings.json ..\settings.json.clean
git filter-repo --invert-paths --path .claude/settings.json
copy ..\settings.json.clean .claude\settings.json
git add .claude/settings.json
git commit -m "project Claude settings: shared permissions and the wording hook"
git remote add origin git@github.com:looneyk/dev.git
git push --force origin main
```

- `filter-repo` runs only on a fresh clone or a clean working tree, and `--force` overrides that; pending work is
  committed first.
- It removes `origin` on purpose, so that a rewritten history is pushed deliberately.
- Every commit from the first that touched the file onward has a new hash, so every other clone is cloned again rather
  than pulled.
- After a force push, GitHub may still serve the old commits by their hashes until it removes unreachable objects.
  That is tolerable for a user name and paths, and not for a secret.

### Publishing a fresh history

The alternative publishes one commit, and keeps the existing repository private with the whole history:

```
git checkout --orphan public
git add -A
git commit -m "initial public release"
```

The `public` branch is pushed as `main` of a new, empty GitHub repository. This removes everything the history has,
experiments included, and is the choice if the scan finds a secret.

## Open

- **Rewrite the history, or publish a fresh one?** The scan's result decides it if it finds anything.
