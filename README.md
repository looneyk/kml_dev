# kml_dev

A set of several largely independent projects that share the libraries in
[`library/quetzal/`](library/quetzal/). The libraries are heavily template-based, written in C++26, and built with CMake and
MSVC. It is released under the [MIT License](LICENSE), and the project documents are in [`doc/`](doc/).
The third-party works it includes are listed with their licenses in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

All of it is work in progress, and its interfaces change without notice.

## 3D Modeling

Procedurally generated 3D models: a set of [model programs](model/) utilizing
[boundary-representation](library/quetzal/brep/) solid modeling.

## Vulkan Rendering

Real-time 3D rendering using Vulkan, an engine and a scene graph. [`doc/vulkan.md`](doc/vulkan.md) describes the engine.

## Stratum

A dungeon-crawler, roguelike game prototype.
[`doc/game_system_reference.md`](doc/game_system_reference.md) describes the system.

## Building

The build requires Visual Studio 2022 or later, CMake 3.25 or later, and Git for Windows. The Vulkan applications also
require the Vulkan SDK, and the stratum Godot host requires Godot 4.7.

    pwsh script/clone_library3.ps1
    cmake -S . -B build
    cmake --build build --config Debug
    ctest --test-dir build -C Debug -j 8

The tests are written in doctest and registered with CTest.

[`doc/development_setup.md`](doc/development_setup.md) describes the full setup.
