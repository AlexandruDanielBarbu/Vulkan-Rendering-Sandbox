# Vulkan Rendering Sandbox

A real-time 3D rendering engine written from scratch in C++ and Vulkan. It renders procedurally generated geometry with physically-inspired lighting, written directly against the Vulkan API for explicit control over the GPU pipeline — without a high-level engine like Unity or Unreal.

Developed for the **Fundamentals of Computer Graphics** course at **TU Wien**.

## Table of Contents

- [Features](#features)
- [Tech Stack](#tech-stack)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Build and Run](#build-and-run)
- [Controls](#controls)
- [Project Structure](#project-structure)
- [Development Milestones](#development-milestones)
- [Engineering Notes](#engineering-notes)
- [Troubleshooting](#troubleshooting)
- [Assets and Licensing](#assets-and-licensing)
- [References](#references)

## Features

- **Procedural geometry** — generates spheres, cylinders, and tori at runtime with a configurable subdivision count for level-of-detail control.
- **Bézier curve geometry** — renders cylindrical surfaces defined by Bézier curves via De Casteljau's algorithm.
- **Phong shading** — per-fragment lighting computed in the fragment shader.
- **Gouraud shading** — per-vertex lighting computed in the vertex shader, for direct comparison.
- **Fresnel effect** — view-dependent reflectance using Schlick's approximation.
- **Mipmapping** — improves texture filtering and reduces aliasing on distant surfaces.
- **Texturing** — procedurally maps UV coordinates onto 2D textures across the geometry.

## Tech Stack

| Category       | Technology                                                                 |
|----------------|----------------------------------------------------------------------------|
| Language       | C++17                                                                       |
| Graphics API   | Vulkan 1.3                                                                  |
| Build System   | CMake 3.24+                                                                 |
| Shaders        | GLSL (compiled to SPIR-V)                                                   |
| Frameworks     | [Vulkan Launchpad](https://github.com/cg-tuwien/VulkanLaunchpad) (`vkl`), TU Wien GCG framework (`gcg`) |

## Getting Started

### Prerequisites

| Tool               | Version                                  | Link                                          |
|--------------------|------------------------------------------|-----------------------------------------------|
| Visual Studio 2022 | Community+, with C++ Development Tools    | https://visualstudio.microsoft.com/vs/        |
| CMake              | 3.24 or higher                           | https://cmake.org/download/                   |
| Vulkan SDK         | 1.3.216.0 or newer                       | https://vulkan.lunarg.com/                    |

Developed and tested on **Windows 11 with Visual Studio 2022**. This is the recommended setup. CLion is known to cause build issues.

On Linux, run the dependency script first to install most prerequisites (excludes VS Code and its extensions):

```bash
./scripts/linux/install_dependencies.sh
```

### Build and Run

**1. Download assets and shared libraries.** Run the `download_assets_and_libs` script matching your operating system.

**2. Configure and build.** Choose one of the methods below.

Visual Studio 2022 (recommended):

1. Right-click the folder containing `CMakeLists.txt` and select *Open with Visual Studio*.
2. Visual Studio detects `CMakeLists.txt` and starts CMake configuration automatically. If it doesn't, right-click `CMakeLists.txt` and select *Configure*.

Command line:

```bash
# Configure
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build (debug)
cmake --build build --config Debug --parallel

# Build (release)
cmake --build build --config Release --parallel
```

Default generator (make):

Double-click `make.bat` (Windows) or run `make debug` / `make release` (macOS/Linux). Project files are generated in `_project/`. To switch generators (for example, Xcode), add `-G "Xcode"` to the cmake command in the makefile. On Windows you still need Visual Studio 2022 installed for the MSVC compiler, even when using VS Code or CLion.

**3. Run** from the project root:

```bash
.\build\Release\GCGProject_VK.exe
```

## Controls

| Key / Input       | Action                                  |
|-------------------|-----------------------------------------|
| `Esc`             | Close the application                   |
| Left mouse        | Arcball camera rotation                 |
| Right mouse       | Strafe camera                           |
| Scroll wheel      | Zoom in / out                           |
| `Z`               | Reset camera position                   |
| `N`               | Visualize normals                       |
| `F`               | Toggle Fresnel effect                   |
| `T`               | View UV coordinates on models           |
| `F1`              | Toggle wireframe                        |
| `F2`              | Cycle cull modes (none, front, back)    |
| `F5`              | Reload shaders                          |

## Project Structure

```
.
├── assets/
│   └── shaders/        # GLSL shader source (auto-discovered at runtime)
├── src/                # Engine source code
├── scripts/            # Setup and dependency scripts
└── CMakeLists.txt
```

Shaders are loaded from `assets/shaders/` at runtime — the application searches this folder automatically. Source code lives in `src/`.

## Development Milestones

The repository is organized as a progression of branches, from `task0` through `task6` (the final version, merged into `main`). Each milestone is runnable on its own, so you can trace how the engine evolved feature by feature:

```bash
git checkout task1   # inspect an earlier milestone, then build and run
```

## Engineering Notes

**Code architecture.** An unstructured `main.cpp` becomes unmaintainable quickly, so the engine is organized using classes and `#region` blocks to separate responsibilities and keep the entry point readable — a pragmatic structure suited to the project's scope.

**Procedural mesh generation.** Generating meshes surfaced a cluster of subtle bugs: incorrect winding order, malformed triangle indices, and missing or duplicated vertices in the vertex buffer. These were debugged through wireframe inspection and methodical trial-and-error. One known artifact remains on a single face of the small cube, kept as a documented learning case rather than silently patched.

**Normals and view-space lighting.** All lighting is computed in view space to simplify the math. This introduces a tradeoff: the normal-visualization mode renders normals relative to the camera, so they always appear to face the viewer. Compared side-by-side with the reference solution, the engine shows slightly reduced specular intensity — an open thread being carried into a follow-up project, _RexCore_.

## Troubleshooting

| Problem                          | Fix                                                                                          |
|----------------------------------|----------------------------------------------------------------------------------------------|
| Build fails inexplicably         | Try a fresh checkout in a new location. Stale CMake or project caches are a common cause.    |
| Path-related errors on Windows   | Windows caps paths at 260 characters. Place the repo in a short directory (e.g. `C:\dev\vrs`). |
| Shaders not found                | Ensure they are in `assets/shaders/` — the app searches this folder at runtime.             |

## Assets and Licensing

This engine was developed as coursework for [Fundamentals of Computer Graphics](https://www.cg.tuwien.ac.at/courses/GCG/VU/2025W) at TU Wien, using the course's `gcg` framework and the [Vulkan Launchpad](https://github.com/cg-tuwien/VulkanLaunchpad) (`vkl`) framework.

Assets (images, `.ini` files) are the property of TU Wien. Obtain explicit permission before reusing them outside this project.

## References

- [Bézier curves as geometry](https://www.scratchapixel.com/lessons/geometry/bezier-curve-rendering-utah-teapot/curves-as-geometry.html) — Scratchapixel
- [De Casteljau's algorithm](https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm)
- [Phong reflection model](https://en.wikipedia.org/wiki/Phong_reflection_model)
- [Schlick's approximation](https://en.wikipedia.org/wiki/Schlick%27s_approximation)
- [Fresnel equations](https://en.wikipedia.org/wiki/Fresnel_equations)
- [Basic Lighting](https://learnopengl.com/Lighting/Basic-Lighting) — LearnOpenGL
