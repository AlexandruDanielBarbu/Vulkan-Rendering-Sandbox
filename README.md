# Vulkan-Rendering-Sandbox

## Index

- [Intro to the project](#intro-to-the-project)
  - [Overview of the branches](#overview-of-the-branches)
- [How to run - step 1](#how-to-run---step-1)
  - [Prerequesites](#prerequesites)
    - [Visual Studio 2022](#visual-studio-2022)
    - [CMake](#cmake)
    - [VULKAN](#vulkan)
    - [Linux requirements](#linux-requirements)
- [How to run - step 2](#how-to-run---step-2)
  - [Download the libraries](#download-the-libraries)
  - [In VS 2022](#in-vs-2022)
  - [Alternative: Using your default generator](#alternative-using-your-default-generator)
- [How to run - step 3](#how-to-run---step-3)
- [Project Structure - relevant folders](#project-structure---relevant-folders)
- [Errors and FAQ](#errors-and-faq)
- [Controls](#controls)
- [Features](#features)
- [Challenges](#challenges)
  - [Structuring the code](#structuring-the-code)
  - [Procedural meshes](#procedural-meshes)
  - [Normals](#normals)
- [Helpful tutorials that I used](#helpful-tutorials-that-i-used)

# Intro to the project

This Rendering Engine was developed as part of an assignment at **Technische Universität Wien** for the course **Grundlagen der Computergraphik** ([Fundamentals of Computer Graphics](https://www.cg.tuwien.ac.at/courses/GCG/VU/2025W)).

For this project, I was provided access to the course-specific framework (functions starting with `gcg`) and the [Vulkan Launchpad framework](https://github.com/cg-tuwien/VulkanLaunchpad) (functions starting with `vkl`).

Assets such as images and `.ini` files are the property of TU Wien. Please obtain explicit permission before using these assets outside the scope of this project.

With that said, I would like to thank the GCG team for organizing and maintaining such an engaging course. My best regards to you all, and I hope you will hear good things about my work in the future.

## Overview of the branches

From branch 0 to 6 (the latter being the final version of the engine, which is integrated into `main`), you can inspect various development milestones. The code is intended to run at each milestone, so if you're curious when a feature was introduced or how the engine looked earlier, you can simply:

```bash
git checkout task1 # for example

# Now run from Visual Studio and play around.
```

# How to run - step 1

## Prerequesites

### Visual Studio 2022

**Download and install `Visual Studio Community 2022`** from [https://visualstudio.microsoft.com/vs/](https://visualstudio.microsoft.com/vs/) with the C++ Development Tools selected.

### CMake

**Download and install `CMake 3.24` or higher** from [https://cmake.org/download/](https://cmake.org/download/).

### VULKAN

**Download and install the `Vulkan SDK 1.3.216.0` or newer** from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/).

> [!WARNING]
> The project was developed on Windows 11. For best results I strongly recommend you test the project on a Windows 11 PC.

### Linux requirements

If you open this framework on Linux, please first run the script `./scripts/linux/install_dependencies.sh` which will install most necessary dependencies except for VS Code and its extensions.


# How to run - step 2

## Download the libraries

Execute the `download_assets_and_libs`script, depending on your operating system. This will download the necessary shared library files.

> [!WARNING]
> For developing this project I used Windows 11. I strongly recommend you also use Visual Studio 2022 for the best experience.
> Please note that you might have some problems if you use CLion.

## In VS 2022

1. Open the Framework as folder in VS 2022 (right-click in the folder containing the `CMakeLists.txt` and select "Open with Visual Studio").
2. The contained `CMakeLists.txt` will automatically be used by Visual Studio to load the project and the CMake configuration process is started immediately and the CMake output should be visible automatically. If not, please right-click on `CMakeLists.txt` and select "Configure project_name".

## Alternative: Using your default generator

We supplied you with a make.bat file for windows and a makefile for MacOS/Linux. You can double click make.bat or execute `make debug` or `make release` to build with your default cmake generator (for example Visual Studio 2022 or Xcode). Project files can be found in `_project` afterwards. You can edit the files to change the generator for example to Xcode by adding a `-G "Xcode"` to the cmake generation command (first line). You can also use CLion or VS Code as an IDE, but on Windows it will require the installation of Visual Studio 2022 nonetheless, as you need the accompanying MSVC compiler.

# How to run - step 3

With Visual Studio 2022 open, press the green triangle at the top (make sure `GCGProject_VK.exe` is selected) to run the engine in **Debug mode**.

# Project Structure - relevant folders

Shader Code is located in the `assets/shaders/` folder, and the application will try to find any shaders inside this folder.

Source Code is located in the `src` folder.

# Errors and FAQ

Please follow the instructions of this readme carefully if something does not work.
If everything was done correctly, please look if a new checkout of the Repo in a different location helps.
Sometimes CMake caches can interfere for example. Sometimes project caches can also lead to problems.
Windows path length is a major problem often, it is restricted to 260 characters and you should place the repository into a short path

# Controls:

| Key | Action |
|-----|--------|
| esc | close the application |
|-----|--------|
| left mouse btn  | arcball camera |
| right mouse btn | strafe camera |
| scroll wheel    | zoom in / out |
|-----|--------|
| Z | reset camera position |
| N | view normals |
| F | toggle Fresnel effect |
| T | view UV coordinates on models |
|-----|--------|
| F1 | wireframe on / off |
| F2 | cycle: cull modes (none, front, back) |
| F5 | reload shaders |

# Features:

- **Bézier curve geometry**: Supports rendering cylindrical geometry defined by Bézier curves.
- **Procedural geometry with custom level of detail**: Generates meshes for **spheres**, **cylinders**, and **tori** (see branch `boni4`) with user defined number of subdivisions.
- **Mipmapping**: Uses mipmaps to improve texture filtering and reduce aliasing at distance.
- **Texturing**: Procedurally adjusts UV coordinates to a 2D textures to rendered geometry.
- **Phong / Gouraud shading**:
  - **Phong shading** (per-fragment lighting) implemented in the **fragment shader**
  - **Gouraud shading** (per-vertex lighting) implemented in the **vertex shader**
- **Fresnel effect**: Implements a Fresnel term using Schlick's approximation for view-dependent reflectance.

# Challenges

## Structuring the code

When starting this assignment, I quickly realized I needed to organize my code early on. Even though I could have done more to reduce the complexity of `main.cpp`, I found that using `#region`s and classes worked well for the scope of this engine.

## Procedural meshes

I ran into several issues while generating meshes. At times I used the wrong winding order for faces, specified incorrect indices for a triangle, or inserted the wrong vertices (or forgot to insert some) into the vertex buffer. With enough patience and a long trial-and-error process, I managed to fix most of these issues. If you enable wireframe mode, you can see that the small cube has a different orientation for the left face - a great learning exercise nonetheless.

## Normals

Normals were fairly straightforward, however computing the correct normal relative to the camera view was tricky.

I performed all lighting calculations in view space, with the intent of saving development time, but it did not, since I had to adjust the uniforms anyway. As a result, the normal-visualization mode always displays normals as if you are looking directly at them.

The lighting looks good overall, but if you compare my engine side-by-side with the course-approved solution, you may notice slightly less "shininess" on the objects. I plan to investigate this further in my upcoming project, _RexCore_.

# Helpful tutorials that I used

- https://www.scratchapixel.com/lessons/geometry/bezier-curve-rendering-utah-teapot/curves-as-geometry.html
- https://grokipedia.com/page/De_Casteljau's_algorithm
- https://en.wikipedia.org/wiki/Phong_reflection_model
- https://en.wikipedia.org/wiki/Schlick%27s_approximation
- https://en.wikipedia.org/wiki/Fresnel_equations
- https://learnopengl.com/Lighting/Basic-Lighting