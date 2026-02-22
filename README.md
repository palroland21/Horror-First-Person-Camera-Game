# Horror First-Person Camera Game 🏥🔦👻 (OpenGL)

A first-person **horror exploration** scene rendered in **OpenGL**, built as a mini-game set in an abandoned hospital.  
You can freely walk through the environment (W/A/S/D + mouse look), interact with doors, toggle lighting, and experience a horror atmosphere enhanced with **flashlight + shadows (shadow mapping)** and **distance-based monster audio**.

> Project documentation/report: see `PG__Horror_Game.pdf`.

---

## 🎯 Project Overview

The main goal of this project was to create a **realistic-looking interactive 3D horror scene** using:
- OpenGL rendering pipeline + GLSL shaders
- First-person camera controls
- Collisions (so the player cannot walk through walls/objects)
- Gravity + floors (so the player stays grounded and can walk stairs)
- Door animations + auto-close logic
- Flashlight spotlight attached to the camera + **shadow mapping**
- Monster ambient sound that becomes louder as you approach

---

## ✅ Features

### 🎮 First-person controls (keyboard + mouse)
- Movement with **W / A / S / D**
- Mouse look (yaw/pitch) with pitch clamping (prevents camera flipping)
- Smooth continuous movement using a `pressedKeys[1024]` array for held keys

### 🧱 Collision system (AABB + player cylinder)
- Scene obstacles (walls/objects) are represented as **AABB colliders**
- Player is treated as a simplified **cylinder** (radius on XZ + height)
- Movement uses “try move → collision check → rollback if needed”

### 🌍 Gravity + floor patches (no floating)
- The player is “glued” to the floor and falls if no floor exists underneath
- Floors are defined as rectangular **FloorPatch** zones with a known `topY`
- The correct floor is selected under the player even when multiple levels exist (stairs / different floors)

### 🚪 Door interaction (animation + auto-close + door collisions)
- Doors open/close on **left click**
- Opening/closing is animated gradually (angle increases/decreases)
- Optional **auto-close timer** (closes after a few seconds)
- Door collider blocks the player only when the door is considered closed

### 🔦 Lighting (flashlight + lamp)
- Flashlight is a **spotlight** attached to the camera (with a small right/down offset to feel “hand-held”)
- A lamp exists in the scene and can be toggled and moved to demonstrate object transforms

### 🌑 Shadows (shadow mapping from the flashlight)
- Two-pass rendering:
  1) Depth pass from the flashlight into a depth texture (shadow map)
  2) Normal render pass + shadow test in fragment shader
- Uses **bias** to reduce shadow acne and **PCF** (3x3) for softer edges

### 🔊 Audio (monster sound with distance-based volume)
- Ambient monster sound implemented using **miniaudio**
- Volume depends on distance to the monster:
  - Far away → volume ~ 0
  - Close → volume increases up to a max value
- Smooth volume transitions using deltaTime-based fading (independent of FPS)

### 🎥 Presentation / Tour mode
- Press **P** to toggle an automatic camera tour (moves in a circle around a center)
- Exiting tour mode resets the camera to the start position

### 🖼️ Render modes + shading
- Render mode toggle:
  - `1` = solid
  - `2` = wireframe
  - `3` = points
- `4` toggles **smooth vs flat shading**

---

## 🧭 Controls

| Action | Key / Input | Notes |
|---|---|---|
| Move | **W / A / S / D** | Continuous while held |
| Look around | **Mouse** | FPS camera (yaw/pitch) |
| Door interact | **Left click** | Open/close (some auto-close) |
| Tour mode ON/OFF | **P** | Auto camera movement + reset on exit |
| Render modes | **1 / 2 / 3** | Solid / Wireframe / Points |
| Smooth / Flat shading | **4** | Lighting appearance |
| Lamp ON/OFF | **H** | Toggle scene lamp |
| Move lamp (X/Z) | **I / J / K / L** | Forward/back + left/right |
| Move lamp (Y) | **U / O** | Up/down |
| Scale / rotate lamp | **N / M**, **Z / X** | Scale + rotate around Y |

---

## 🗺️ How to play (mini guide)

- Walk around with **W/A/S/D** and look around with the **mouse**
- You cannot pass through walls or objects (collision system)
- Approach doors and **left click** to open/close them
- Toggle **tour mode (P)** to view the scene automatically

---

## 🧩 Technical Implementation Highlights

### Models, textures, materials
- Scene uses `.obj` models + `.mtl` materials
- Textures are loaded using **stb_image**, uploaded to GPU with `glTexImage2D`, and bound per mesh via texture units
- Rendering pipeline: OBJ/MTL → load geometry/materials → create OpenGL textures → shader samples texture using UV coordinates

### Core data structures
- `AABB` for collisions (walls/objects)
- `Door` for doors (hinge point, angle animation, auto-close timer, collider)
- `FloorPatch` for floor zones (min/max + topY)
- `std::vector` containers for colliders, floors, doors

---

## 🗂️ Project Structure (typical)

### Header files (interfaces)
- `Window.h` – window creation, OpenGL context, GLFW callbacks
- `Shader.hpp` – shader loading/compilation
- `Camera.hpp` – camera position/orientation, movement, view matrix
- `Model3D.hpp` – OBJ/MTL loading + rendering
- `Mesh.hpp` – mesh data (VAO/VBO, vertices/normals/UV/indices)
- `stb_image.h` – image loading for textures
- `tiny_obj_loader.h` – OBJ/MTL parsing
- `miniaudio.h` – audio playback

### Source files (implementations)
- `main.cpp` – game loop, input, collisions, animations, updates, rendering
- `Window.cpp` – GLFW/OpenGL initialization
- `Shader.cpp` – shader implementation
- `Camera.cpp` – camera movement/rotation implementation
- `Model3D.cpp` – model/material loading implementation
- `Mesh.cpp` – GPU buffer setup and draw calls
- `stb_image.cpp`, `tiny_obj_loader.cpp` – external library sources

---

## ▶️ How to Run

- Run from an IDE (Visual Studio / CLion) and press **Run**
- If you run an executable directly, keep it next to all required resource folders (models, textures, shaders, sounds), otherwise the scene will not load correctly

---

## 🔮 Possible future improvements
- Add a clear gameplay objective (e.g., find a key, find the exit, activate a generator)
- Optimize rendering by not drawing objects outside the camera view (culling)

---

## 👤 Author
**Pal Roland** – Technical University of Cluj-Napoca (UTCN), Computer Science
