# 3D Scene Viewer & Object Manipulation Engine (`cg-scene-viewer-3D`)

An interactive 3D scene viewer and geometric manipulation engine developed in **C++17** and **Modern OpenGL 3.3 (Core Profile)**. 

This project was built for the **Introduction to Computer Graphics** course at the **Central University of Venezuela (UCV)**, Faculty of Sciences, School of Computer Science.

---

## Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Mathematical & Theoretical Concepts](#mathematical--theoretical-concepts)
  - [1. Homogeneous Coordinates & Affine Transformations](#1-homogeneous-coordinates--affine-transformations)
  - [2. Pivot-Centered Rotations](#2-pivot-centered-rotations)
  - [3. Object Normalization & AABB](#3-object-normalization--aabb)
  - [4. Analytical & Area-Averaged Normal Computation](#4-analytical--area-averaged-normal-computation)
  - [5. Procedural Parametric Geometry](#5-procedural-parametric-geometry)
  - [6. Color Picking via Offscreen Framebuffer (FBO)](#6-color-picking-via-offscreen-framebuffer-fbo)
  - [7. Lighting & Shading](#7-lighting--shading)
  - [8. Hidden Surface Removal (Z-Buffer & Back-Face Culling)](#8-hidden-surface-removal-z-buffer--back-face-culling)
- [Architecture & Tech Stack](#architecture--tech-stack)
- [Building & Running](#building--running)
  - [Prerequisites](#prerequisites)
  - [Compilation](#compilation)
  - [Execution](#execution)
- [Controls & Navigation](#controls--navigation)
- [File Format Specifications](#file-format-specifications)

---

## Overview

`cg-scene-viewer-3D` is a complete graphics application designed to import, render, inspect, modify, and export 3D triangular meshes and procedural geometric primitives. The system provides real-time interaction through an orbital camera, multiple rendering visualization modes, and a modern collapsible user interface built with Dear ImGui.

---

## Key Features

- **Mesh Loading**: Import Wavefront `.obj` files with automatic multi-submesh decomposition and `.mtl` / `.mlt` diffuse color extraction using `tinyobjloader`.
- **Normalization Pipeline**: Centering to the coordinate origin and isotropic scaling into the $[-1, 1]^3$ normalized unit volume.
- **Normal Generation**: Automatic smooth vertex normal reconstruction via face-normal area averaging when models lack precomputed normal data.
- **Multi-Level Selection (Color Picking)**:
  - **Global**: Selects and transforms the entire multi-part object.
  - **Local**: Selects and modifies an isolated submesh independently.
  - **Triangle**: Selects and highlights individual geometric faces.
- **Interactive Transformations**: Real-time translation, rotation about local centroids, and isotropic/anisotropic scaling.
- **Visualization Modes**:
  - **Solid (Shaded)**: Lambertian diffuse lighting with directional light.
  - **Wireframe**: Polygonal edge rasterization (`GL_LINE`).
  - **Vertices**: Point rendering with configurable point raster size.
  - **Normals**: Real-time vector projection using a specialized **Geometry Shader**.
  - **Bounding Box**: Wireframe visualization of Axis-Aligned Bounding Boxes (AABB).
- **Procedural Primitives**: On-the-fly generation of parametric **Cubes**, **Pyramids**, **UV Spheres**, and **Cylinders** with customizable dimensions and tessellation levels.
- **Lighting & Pipeline Controls**: Toggleable `GL_DEPTH_TEST` (Z-Buffer) and `GL_CULL_FACE` (Back-Face Culling with Counter-Clockwise winding).
- **Material & Transparency**: Full RGBA diffuse color customization with alpha channel modulation and alpha blending (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`).
- **Scene Persistence**:
  - **Save / Load Scene (`.scene`)**: Custom human-readable format storing objects, submesh states, procedural parameters, lighting, camera, and background settings.
  - **Export OBJ (`.obj` + `.mtl`)**: Exports the combined or procedurally generated geometry into standard Wavefront OBJ files with companion material libraries.
- **Performance**: Real-time frame time and frames-per-second (FPS) monitoring.

---

## Mathematical & Theoretical Concepts

### 1. Homogeneous Coordinates & Affine Transformations
3D positions are represented using 4D projective homogeneous vectors:

$$
\mathbf{P} = \begin{pmatrix} x \\ y \\ z \\ 1 \end{pmatrix}
$$

Affine transformations are composed via matrix multiplication in column-major order:

$$
\mathbf{M} = \mathbf{T}(t_x, t_y, t_z) \cdot \mathbf{R}_z(\theta_z) \cdot \mathbf{R}_y(\theta_y) \cdot \mathbf{R}_x(\theta_x) \cdot \mathbf{S}(s_x, s_y, s_z)
$$

- **Translation Matrix**:

  $$
  \mathbf{T}(\mathbf{t}) = \begin{pmatrix} 1 & 0 & 0 & t_x \\ 0 & 1 & 0 & t_y \\ 0 & 0 & 1 & t_z \\ 0 & 0 & 0 & 1 \end{pmatrix}
  $$

- **Scaling Matrix**:

  $$
  \mathbf{S}(\mathbf{s}) = \begin{pmatrix} s_x & 0 & 0 & 0 \\ 0 & s_y & 0 & 0 \\ 0 & 0 & s_z & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}
  $$

- **Elementary Rotation Matrices** for axes $X, Y, Z$ using standard trigonometric formulations.

The full vertex transformation pipeline maps vertices from Model Space to Clip Space:

$$
\mathbf{P}_{\text{clip}} = \mathbf{M}_{\text{projection}} \cdot \mathbf{M}_{\text{view}} \cdot \mathbf{M}_{\text{model}} \cdot \mathbf{M}_{\text{local}} \cdot \mathbf{P}_{\text{local}}
$$

---

### 2. Pivot-Centered Rotations
To prevent objects and submeshes from revolving around the global origin $(0, 0, 0)$ when rotated, rotations are executed relative to their geometric centroid $\mathbf{c} \in \mathbb{R}^3$:

$$
\mathbf{M}_{\text{pivot}} = \mathbf{T}(\mathbf{c}) \cdot \mathbf{R}(\boldsymbol{\theta}) \cdot \mathbf{S}(\mathbf{s}) \cdot \mathbf{T}(-\mathbf{c})
$$

This translates the geometry so that its center aligns with the origin, applies the angular rotation and scale, and translates it back.

---

### 3. Object Normalization & AABB
Upon importation, each model's Axis-Aligned Bounding Box (AABB) is calculated by finding the coordinate extrema:

$$
\mathbf{p}_{\min} = \min_{i}(\mathbf{v}_i), \quad \mathbf{p}_{\max} = \max_{i}(\mathbf{v}_i)
$$

The model is centered and normalized to fit within the $[-1, 1]^3$ cube:

$$
\mathbf{c} = \frac{\mathbf{p}_{\min} + \mathbf{p}_{\max}}{2}
$$

$$
d_{\max} = \max(p_{\max, x} - p_{\min, x},\; p_{\max, y} - p_{\min, y},\; p_{\max, z} - p_{\min, z})
$$

$$
s = \frac{2.0}{d_{\max}}
$$

$$
\mathbf{v}'_i = (\mathbf{v}_i - \mathbf{c}) \cdot s
$$

---

### 4. Analytical & Area-Averaged Normal Computation
When an imported model does not supply vertex normals, smooth normals are approximated by weighting the normals of all adjacent faces sharing a vertex.

For each triangle defined by vertices $(\mathbf{v}_0, \mathbf{v}_1, \mathbf{v}_2)$:

$$
\mathbf{e}_1 = \mathbf{v}_1 - \mathbf{v}_0, \quad \mathbf{e}_2 = \mathbf{v}_2 - \mathbf{v}_0
$$

$$
\mathbf{N}_{\text{face}} = \mathbf{e}_1 \times \mathbf{e}_2
$$

The face normal magnitude $\|\mathbf{N}_{\text{face}}\|$ is proportional to twice the triangle's surface area. By summing $\mathbf{N}_{\text{face}}$ directly into each vertex's accumulator, larger triangles contribute proportionally more weight:

$$
\mathbf{N}_{\text{accum}}(\mathbf{v}_i) = \sum_{k \in \text{Faces}(v_i)} \mathbf{N}_{\text{face}, k}
$$

$$
\hat{\mathbf{N}}(\mathbf{v}_i) = \frac{\mathbf{N}_{\text{accum}}(\mathbf{v}_i)}{\|\mathbf{N}_{\text{accum}}(\mathbf{v}_i)\|}
$$

---

### 5. Procedural Parametric Geometry

#### UV Sphere
Constructed using spherical coordinates with radius $r$, longitudinal sectors $j \in [0, N_s]$, and latitudinal stacks $i \in [0, N_t]$:

$$
\theta = \frac{2\pi j}{N_s}, \quad \phi = \frac{\pi}{2} - \frac{\pi i}{N_t}
$$

$$
\mathbf{p}(\theta, \phi) = \begin{pmatrix} r \cos \phi \cos \theta \\ r \sin \phi \\ r \cos \phi \sin \theta \end{pmatrix}, \quad \hat{\mathbf{n}}(\theta, \phi) = \frac{\mathbf{p}(\theta, \phi)}{r}
$$

#### Cylinder
Constructed using polar cylindrical coordinates with radius $r$, height $h$, and radial segments $j \in [0, N_s]$:

$$
\theta = \frac{2\pi j}{N_s}
$$

- **Lateral Surface**: Vertices at $(r \cos \theta, \pm \frac{h}{2}, r \sin \theta)$ with radial normals $\hat{\mathbf{n}} = (\cos \theta, 0, \sin \theta)$.
- **End Caps**: Planar triangle fans at $y = \pm \frac{h}{2}$ with axial normals $(0, \pm 1, 0)$.

#### Cube & Pyramid
Modeled face-by-face with duplicated vertices per facet to ensure sharp edges and distinct orthogonal normal vectors. All facets enforce strict Counter-Clockwise (CCW) vertex winding.

---

### 6. Color Picking via Offscreen Framebuffer (FBO)
Object selection utilizes GPU-accelerated **Color Picking**:
1. An offscreen Framebuffer Object (FBO) is bound with a dedicated `GL_RGBA8` texture and a 24-bit depth renderbuffer (`GL_DEPTH_COMPONENT24`).
2. An integer identifier is packed into the 24-bit RGB channels:
   - **Red**: `R = ID & 0xFF`
   - **Green**: `G = (ID >> 8) & 0xFF`
   - **Blue**: `B = (ID >> 16) & 0xFF`
3. The scene is drawn using an unlit flat shader (`picking.frag`).
4. Upon mouse click at screen coordinate $(x, y)$, `glReadPixels` samples the exact pixel at $(x, H - 1 - y)$:
   - **Decoded ID**: `ID = R | (G << 8) | (B << 16)`
   
   An ID of `0` denotes background (no hit).

---

### 7. Lighting & Shading
Surface lighting employs the **Lambertian Diffuse Reflection Model**:

$$
I = I_{\text{ambient}} \cdot K_a + I_{\text{diffuse}} \cdot K_d \cdot \max(\hat{\mathbf{N}} \cdot \hat{\mathbf{L}}, 0.0)
$$

Where:
- $\hat{\mathbf{N}}$ is the interpolated surface unit normal.
- $\hat{\mathbf{L}}$ is the normalized direction vector towards the light source.
- $K_d$ is the diffuse reflectance material coefficient loaded from `.mtl` or configured via the color picker.
- $I_{\text{diffuse}}$ and $I_{\text{ambient}}$ denote the incident light intensities.

---

### 8. Hidden Surface Removal (Z-Buffer & Back-Face Culling)
- **Z-Buffer Algorithm**: During rasterization, fragment depth $z \in [0, 1]$ is tested against the depth buffer. Fragments with $z < z_{\text{buffer}}[x, y]$ pass the test and overwrite the pixel color and depth value.
- **Back-Face Culling**: Polygons whose surface normal points away from the camera are culled before rasterization. Given viewing direction $\vec{D}$ and face normal $\vec{N}$:

  $$
  \vec{D} \cdot \vec{N} > 0 \implies \text{Cull Face}
  $$

  In OpenGL Core Profile, this is determined via the projected 2D winding order (CCW = front-facing, CW = back-facing).

---

## Architecture & Tech Stack

| Dependency | Purpose |
| :--- | :--- |
| **C++17** | Core programming language |
| **OpenGL 3.3 Core** | Hardware-accelerated graphics API |
| **GLFW 3** | Window management, OpenGL context creation, mouse/keyboard polling |
| **GLAD** | OpenGL function loader |
| **GLM** | Mathematics library for vectors, quaternions, and matrices |
| **Dear ImGui (1.90.x)** | Immediate-mode graphical user interface |
| **tinyobjloader** | Wavefront `.obj` file parser |
| **CMake 3.20+** | Multi-platform build system |

### Source Tree
```
cg-scene-viewer-3D/
├── CMakeLists.txt              # Build configuration and dependency fetching
├── include/
│   ├── BoundingBox.h           # Unit AABB wireframe renderer
│   ├── Camera.h                # Orbital and pan camera implementation
│   ├── Mesh.h                  # OpenGL buffers (VAO, VBO, EBO) and submesh data
│   ├── Model.h                 # Multi-mesh container, transform hierarchy, OBJ loader
│   ├── PickingFBO.h            # Offscreen framebuffer for color picking
│   ├── PrimitiveGenerator.h    # Procedural geometry generation
│   ├── SceneIO.h               # Plain-text .scene parser and OBJ exporter
│   └── Shader.h                # Shader program compiler and uniform manager
├── src/
│   ├── BoundingBox.cpp
│   ├── Camera.cpp
│   ├── Mesh.cpp
│   ├── Model.cpp
│   ├── PickingFBO.cpp
│   ├── PrimitiveGenerator.cpp
│   ├── SceneIO.cpp
│   ├── Shader.cpp
│   └── main.cpp                # Application entry point, render loop, ImGui interface
└── shaders/
    ├── base.vert / base.frag   # Lambert diffuse shading
    ├── picking.vert / frag     # ID encoding for color picking
    ├── flat.vert / flat.frag   # Solid color rendering (AABB, highlights)
    └── normals.vert / geom / frag # Surface normal vectors via Geometry Shader
```

---

## Building & Running

### Prerequisites
- **CMake** 3.20 or newer
- **C++17 compliant compiler**:
  - Windows: Visual Studio 2019/2022 (MSVC) or MinGW-w64
  - Linux: GCC 9+ or Clang 10+
  - macOS: Apple Clang 12+
- **Git** (required by CMake to fetch GLFW, GLM, ImGui, and tinyobjloader automatically)

---

### Compilation

#### Windows (PowerShell / Command Prompt with Visual Studio)
```powershell
# 1. Clone or enter project directory
cd cg-scene-viewer-3D

# 2. Configure build system
cmake -B build -S .

# 3. Build project (Debug or Release)
cmake --build build --config Release
```

#### Linux / macOS (Terminal)
```bash
# 1. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 2. Compile
cmake --build build -j$(nproc)
```

---

### Execution

#### Windows
```powershell
.\build\Release\cg-scene-viewer-3D.exe
# Or if built in Debug:
.\build\Debug\cg-scene-viewer-3D.exe
```

#### Linux / macOS
```bash
./build/cg-scene-viewer-3D
```

---

## Controls & Navigation

### Camera Navigation
| Input | Action |
| :--- | :--- |
| **Left Click + Drag** *(outside UI)* | Orbit camera (Pitch & Yaw) |
| **W / A / S / D** | Pan camera target (Forward / Left / Backward / Right) |
| **Q / E** | Pan camera target (Down / Up) |
| **Middle Click + Drag** | Pan camera target |
| **Mouse Wheel (Scroll)** | Zoom in / Zoom out |

### Object Interaction
| Input | Action |
| :--- | :--- |
| **Left Click on Mesh** | Select object, submesh, or triangle (based on active Mode) |
| **Sidebar Toggle Tab** | Click `«` / `»` tab on left edge to collapse/expand menu |
| **Transform Sliders** | Real-time translation, rotation, and scaling of selection |
| **Color Picker** | Modify diffuse RGB and Alpha transparency |
| **Display Checkboxes** | Toggle Wireframe, Vertices, Normals, and Bounding Box |

---

## File Format Specifications

### Custom Scene Format (`.scene`)
Scenes are saved in an editable plain-text format:
```
# CG Scene File
camera <targetX> <targetY> <targetZ> <distance> <yaw> <pitch>
clearColor <r> <g> <b> <a>
light <dirX> <dirY> <dirZ> <r> <g> <b>
renderOptions <depthTest> <cullFace> <wireframe> <normals> <vertices> <bbox>
model <name> <primitiveConfig> <filePath> <posX> <posY> <posZ> <rotX> <rotY> <rotZ> <scaleX> <scaleY> <scaleZ> <colR> <colG> <colB> <colA>
submesh <name> <posX> <posY> <posZ> <rotX> <rotY> <rotZ> <scaleX> <scaleY> <scaleZ> <colR> <colG> <colB> <colA>
```
