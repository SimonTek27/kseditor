---
title: "Assetto Corsa MOD Creation Guide — Enhanced Edition"
description: "Comprehensive, cross-referenced guide to creating cars and tracks for Assetto Corsa with Custom Shaders Patch (CSP)"
version: "1.16"
date: "2026-07-15"
author: "Community"
---

# Assetto Corsa MOD Creation Guide — Enhanced Edition

> **Document Scope:** This guide covers the two primary content categories in Assetto Corsa modding: **Cars** and **Tracks**. It is designed for intermediate to advanced modders who understand basic 3D modeling and want to produce CSP-compatible, release-quality content.
> 
> **Prerequisites:** Content Manager (CM) and Custom Shaders Patch (CSP) installed. Basic familiarity with `.ini` files, 3D modeling, and DDS textures.

---

## 📑 Quick Navigation

| Section | Description | Page |
|---------|-------------|------|
| [Introduction](#introduction) | Philosophy, tools, and prerequisites | Top |
| [Category 1: Cars](#category-1--cars) | Complete car modding pipeline | Below |
| [1.7 Driver Release](#17-driver-release-standards) | Driver model, cockpit & animation release criteria | Below |
| [Category 2: Tracks](#category-2--tracks) | Complete track modding pipeline | Further below |
| [Essential Tools](#essential-tools-summary) | Software and utility matrix | Bottom |
| [Recommended Workflows](#recommended-workflow) | Step-by-step car/track pipelines | Bottom |
| [Appendix A: File Reference](#appendix-a-file-extension-quick-reference) | Every extension explained | Bottom |
| [Appendix B: Glossary](#appendix-b-glossary-of-ac-modding-terms) | Terminology decoded | Bottom |
| [Appendix C: Troubleshooting](#appendix-c-troubleshooting-matrix) | Common issues and fixes | Bottom |
| [Appendix D: Checklists](#appendix-d-release-checklists) | Pre-release validation | Bottom |

---

## Introduction

Assetto Corsa (AC), developed by Kunos Simulazioni, is one of the most mod-friendly racing simulators on the market. Because the game exposes a large portion of its internal data through plain-text `.ini` configuration files and a documented 3D format (KN5), the community has produced tens of thousands of user-created modifications.

A **"MOD"** in Assetto Corsa can be anything from a simple liveried skin, to a fully scratch-built car with custom physics and audio, to a laser-scanned-grade circuit with dynamic weather and night lighting.

This guide is divided into the two primary content categories that make up the bulk of AC modding:

1. **Cars** — adding new vehicles, from road cars to prototypes and open-wheelers.
2. **Tracks** — adding new circuits, layouts, and environments.

Both categories are installed into the `content/` folder of your Assetto Corsa installation. Cars live in `content/cars/<car_name>/` and tracks live in `content/tracks/<track_name>/`. The game reads these folders automatically; no recompilation is needed.

> **⚠️ Critical:** Before you begin, you should install **Content Manager (CM)** and **Custom Shaders Patch (CSP)**, both of which provide powerful modding, testing, and configuration tools that make the workflow dramatically easier than working with the bare game launcher.

---

## Category 1 — Cars

A car mod in Assetto Corsa is a self-contained directory describing the vehicle's geometry, appearance, behavior, and sound. Below we break it down into its essential subsections: **3D Model**, **Textures**, **Physics**, **Sound**, and **Driver / Cockpit**.

### 1.1 Folder Structure

A complete modern CSP-scripted car folder (based on a real high-end mod such as the VRC Ferrenzo LMH) follows this hierarchy:

```
content/cars/my_car/
├── data/                          # Physics & configuration core
│   ├── car.ini                    # Master config (mass, CoG, inertia, dimensions, camera)
│   ├── engine.ini                 # Engine power/torque curves, fuel, limiter, turbo
│   ├── drivetrain.ini             # Clutch, differentials
│   ├── gearbox.ini                # Gear ratios, final drive, shift behavior
│   ├── suspension.ini             # Geometry, springs, dampers, ARB, wheels
│   ├── tyres.ini                  # Tyre model, grip curves, wear, temperatures
│   ├── aero.ini                   # Drag, downforce, wings, DRS
│   ├── brakes.ini                 # Brake bias, torque, cooling, pad properties
│   ├── steering.ini               # Steering ratio, lock, feedback
│   ├── dashboard.ini              # Digital dashboard layout
│   ├── electronics.ini            # ABS, TC, ESC, launch control (also named electronic_drivers.ini)
│   ├── damage.ini                 # Deformation and failure thresholds
│   ├── lights.ini                 # Headlights, taillights, indicators, brake lights
│   ├── ff_effects.ini             # Force feedback tuning
│   ├── setup.ini                  # Setup menu definitions (adjustable parameters)
│   ├── example_setup.ini          # A named, loadable default setup preset
│   ├── hybrid.ini                 # Hybrid / ERS / KERS systems (MGU-K/H)
│   ├── thermal.ini                # Thermal model: tyre/brake/engine heat (CSP)
│   ├── drs.ini                    # DRS actuator zones and behavior
│   ├── fuel.ini                   # Fuel tank geometry and sloshing properties
│   ├── driver3d.ini               # Driver model placement (seat, head, hands)
│   ├── cameras.ini                # Onboard camera definitions (alt to car.ini)
│   ├── power.lut                  # Optional external power curve lookup table
│   └── torque.lut                 # Optional external torque curve lookup table
│
│   ├── ── Additional real-world data files (see section 1.4) ──
│   ├── ai.ini, ai_tyres.ini           # AI behavior & AI tyre mapping
│   ├── ambient_shadows.ini            # Baked ambient occlusion shadows
│   ├── bumpstops.ini / dampers.ini    # Damper curves split from suspension.ini
│   ├── colliders.ini                  # Detailed collision mesh descriptors
│   ├── lods.ini / blurred_objects.ini # LOD distances, rotating-blur props
│   ├── mirrors.ini                    # Mirror placement/type
│   ├── digital_instruments.ini        # Custom digital dash (CSP)
│   ├── dash_cam.ini                   # Dashcam definition
│   ├── flames.ini / flame_presets.ini # Exhaust flame FX (CSP)
│   ├── fuel_cons.ini                  # Fuel consumption mapping
│   ├── er*.ini / ers.ini / *ers*.lut  # Hybrid/ERS control & torque maps
│   ├── ctrl_*.ini                     # Control maps (brake, turbo, ERS, ARB)
│   ├── extra_animations.ini / wing_animations.ini / proview_nodes.ini
│   ├── *.lut          # Lookup tables (see 1.4.1) — hundreds in big mods
│   ├── *.rto          # Gear ratio files (see 1.4.2)
│   └── script*.lua    # CSP Lua scripting (see 1.4.3)
│
├── 3d/
│   ├── car.kn5            # Exterior + visible interior mesh
│   ├── cockpit.kn5        # Cockpit/inside mesh (optional separate)
│   └── *.ksanim           # Animations (steering wheel, pedals, shifter)
├── sfx/
│   ├── <name>.bank        # FMOD sound bank (engine, exhaust, etc.)
│   ├── GUIDs.txt          # GUID mapping for events
│   └── sounds.ini         # CSP sound event mapping
├── texture/               # Misc textures (flames, decals, etc.)
│   └── *.png / *.dds
├── extension/             # Custom Shaders Patch extras (see 1.8)
│   ├── ext_config.ini     # Main CSP toggle/config
│   ├── ext_car_controls.ini
│   ├── <mod>_config/      # Mod-specific config folder
│   │   ├── csp/           #   CSP scripts, lighting, display
│   │   ├── animations.ini
│   │   ├── generic.ini / lighting.ini / materials.ini / tyres.ini
│   ├── data_override/     # Lua script overrides (physics, force, lights, display)
│   │   ├── *.lua          #   script.lua, force.lua, lights.lua, display.lua...
│   │   └── display/       #   HTML/JS custom dashboard
│   ├── hr_textures.zip / tyre_textures.zip  # High-res/tyre texture packs
│   └── Texture*.dds       # Override albedo/normal/specular maps
├── skins/
│   └── <livery_name>/
│       ├── skin.ini       # Skin author, name, template info
│       ├── preview.jpg    # Showroom preview image
│       ├── body.dds       # Body paint texture
│       ├── windows.dds    # Glass texture
│       ├── rims.dds       # Wheel rim texture
│       ├── calipers.dds   # Brake caliper texture
│       ├── interior.dds   # Interior texture
│       └── *.dds          # Additional decals, numbers, sponsors, helmets
├── ui/
│   ├── ui_car.json        # Menu display (name, tags, specs, power/torque graphs)
│   ├── badge.png          # Small icon shown in car list
│   ├── brand.png          # Manufacturer logo
│   ├── camera_trajectory.json  # Replay/chase camera path (CSP)
│   ├── cm_paintshop.json  # Content Manager paintshop customization
│   └── cm_textures.json   # Content Manager texture mapping
├── driver/                # Driver helmet & suit (see section 1.5)
│   ├── driver.kn5
│   └── driver.ini
└── description.ini        # Long description shown in CM
```

> **💡 Pro Tip:** Keep your folder structure clean. Many modders separate WIP files into a `_dev/` or `_source/` subfolder that is removed before release. This prevents shipping bloated archives.

---

### 1.2 3D Model

The 3D model is the visual representation of the car. Assetto Corsa uses the proprietary **KN5** format. You cannot directly edit a KN5; you build the model in a 3D application and export it through the official Kunos KN5 exporter.

#### Software
| Application | Cost | Notes |
|-------------|------|-------|
| **Blender** | Free | Most popular choice; community KN5 importer/exporter add-on (`io_import_ksp_cars` / `Blender KN5` scripts) |
| **Autodesk 3ds Max** | Paid | Kunos shipped an official exporter; common in professional modding |
| **Maya / Cinema 4D** | Paid | Possible via scripts but less common |

#### Modeling Pipeline
1. **Reference gathering** — collect blueprints, photos (every angle), dimensions, wheelbase, track width, ride height.
2. **High-poly model** — sculpt the exterior with full detail (panel gaps, vents, badges).
3. **Retopology / game-ready mesh** — reduce to 30k–120k triangles for the exterior depending on class. Keep it clean and quad-based where possible.
4. **UV unwrapping** — lay out UVs for body, windows, rims, calipers, interior. Non-overlapping, texel-density-consistent maps (e.g., 2048×2048 or 4096×4096).
5. **LODs (Levels of Detail)** — create 2–4 progressively simplified versions (`car_lod1`, `car_lod2`) so distant cars render cheaply. AC switches LODs by distance automatically.
6. **Collision mesh** — a very low-poly box or shell used only for crash physics; does not need to be visible.

> **📌 Note:** LODs are not optional for release-quality mods. Poor LOD management is the #1 cause of FPS drops in multiplayer races with 30+ car grids.

#### Naming Conventions (Critical)
AC relies on exact node names to bind animations and physics. Common required names:

| Node Name | Purpose |
|-----------|---------|
| `WHEEL_LF`, `WHEEL_RF`, `WHEEL_LR`, `WHEEL_RR` | Wheel hubs (must rotate with wheel spin and steer) |
| `STEER_L`, `STEER_R` | Steering column/arms |
| `PEDAL_THROTTLE`, `PEDAL_BRAKE`, `PEDAL_CLUTCH` | Pedal travel |
| `GEARCHANGE` / `SHIFT_STICK` | Gear lever for H-pattern |
| `DOOR_L`, `DOOR_R` | Door animations |
| `WIPER_L`, `WIPER_R` | Windshield wipers |
| `WING` | Adjustable aerodynamic element |
| `DRIVER_HANS`, `DRIVER_HELMET` | Driver safety gear |

> **⚠️ Critical:** Use `_N` suffix or separate normal map channel per material. Node names are case-sensitive.

#### Materials / Shaders
Materials are assigned via Kunos shaders in the exporter. Key shaders:

| Shader | Use Case |
|--------|----------|
| `ksCarPaint` | Metallic flake car paint with clearcoat |
| `ksPerPixel` / `ksPerPixelAT` | Generic surfaces |
| `ksGlass` | Transparent glass with reflections |
| `ksChrome` | Mirrors, trim |
| `ksTyre` | Rubber with tread |
| `ksLight` / `ksEmissive` | Lights and emissive parts |
| `ksVegetation` | Rare on cars |
| `ksWindscreen` | Rain effects on windshield |

Each material references textures (diffuse, normal, specular/AO, detail, clearcoat). Texture resolution should be power-of-two (1024, 2048, 4096).

#### Export Settings
- Scale must be **1 unit = 1 meter** (AC is metric).
- "Hide from Reflection" toggles on interior-only parts.
- Set LOD distances.
- Verify in CM's 3D viewer before testing in-game.

---

### 1.3 Textures

Textures define the surface appearance and the paint scheme (skins). They are stored as **DDS** (DirectDraw Surface) files with BC/DXT compression.

#### Texture Channels per Material
| Channel | Purpose |
|---------|---------|
| **Diffuse (RGB)** | Base color/albedo |
| **Normal map** | Surface relief (panel lines, rivets) |
| **Specular / Reflection** | Shininess and reflectivity mask |
| **Ambient Occlusion (AO)** | Contact shadows baked in |
| **Detail map** | Tileable micro-detail (for close-up paint scratches, dirt) |

#### Skin System
Each folder under `skins/` is one livery. The `skin.ini` contains:
```ini
[SKIN]
NAME=My Team Livery
AUTHOR=YourName
TEMPLATE=template.psd
```

Skin textures are merged over the car's base material at runtime. Standard names:
- `body.dds` — main paint (the file you repaint).
- `windows.dds` — window tints and reflections.
- `rims.dds` — wheel face paint.
- `calipers.dds` — brake caliper color.
- `interior.dds` — seats, dashboard, roll cage.

> **⚠️ Critical:** Use **DXT5** (BC3) if the texture has alpha (transparency), **DXT1** (BC1) otherwise. Tools: NVIDIA Texture Tools, Paint.NET, Photoshop with Intel DDS plugin, GIMP with DDS plugin.

#### Creating a Template
Modders often ship a `template.psd` — a layered Photoshop file with the UV layout on a base layer so users can paint easily. Many cars also use a `mapping.ini` for multi-layer liveries (number panels, sponsor zones).

#### Preview
Each skin needs a `preview.jpg` rendered from the showroom (use CM's "Render Skin" function) at 512×512 or 1024×1024.

---

### 1.4 Physics

Physics in Assetto Corsa is entirely data-driven through `.ini` files in the `data/` folder. There is no visual scripting; you describe the car numerically and the engine simulates it. This is where realism is won or lost.

The complete set of `data/*.ini` files that Assetto Corsa reads for a car:

| File | Purpose | Cross-Reference |
|------|---------|-----------------|
| `car.ini` | Master config: mass, CoG, inertia, dimensions, camera, loaded sub-files | All files |
| `engine.ini` | Power/torque curves, RPM limiter, idle, fuel burn, turbo params | `power.lut`, `torque.lut` |
| `drivetrain.ini` | Clutch capacity, differential type/bias/preload | `gearbox.ini` |
| `gearbox.ini` | Gear ratios, final drive, shift type (seq/H-pattern) | `drivetrain.ini`, `*.rto` |
| `suspension.ini` | Per-wheel springs/dampers/ARB, geometry, ride height, wheels | `dampers.ini`, `bumpstops.ini` |
| `tyres.ini` | Tyre model, slip curves, compounds, wear, temperature | `*.lut` (Pacejka curves) |
| `aero.ini` | Cd/Cl, wings, diffuser, ground effect, DRS | `*.2Dlut` |
| `brakes.ini` | Brake torque, bias, cooling, fade, ABS interaction | `electronics.ini` |
| `steering.ini` | Steering ratio, max lock, speed-sensitive assist | `car.ini` |
| `dashboard.ini` | Digital/analog gauge and shift-light layout | `digital_instruments.ini` |
| `electronics.ini` | ABS, TC, ESC, launch control maps | `setup.ini` |
| `damage.ini` | Deformable nodes, part failure thresholds | — |
| `lights.ini` | Head/tail/indicator/brake light behavior | `flames.ini` |
| `ff_effects.ini` | Force feedback gain, damping, min/max force | `car.ini` |
| `setup.ini` | Which parameters appear in the setup menu + ranges | `ctrl_*.ini` |
| `example_setup.ini` | A named loadable preset the player can select | `setup.ini` |
| `hybrid.ini` | Hybrid/ERS/KERS deployment and recovery | `ers.ini`, `script_*.lua` |
| `thermal.ini` | Tyre/brake/engine thermal model (CSP) | `tyres.ini` |
| `drs.ini` | DRS actuator logic and zones | `aero.ini` |
| `fuel.ini` | Fuel tank position, capacity, sloshing | `car.ini` |
| `driver3d.ini` | Driver model seat/head/hand placement | `driver/` |
| `cameras.ini` | Onboard camera definitions | `car.ini` `[CAMERA]` |
| `power.lut` / `torque.lut` | Optional external lookup tables for power/torque | `engine.ini` |

> **💡 Pro Tip:** Start physics development by copying a similar Kunos car (e.g., `ks_ferrari_458` for a mid-engine GT) and modifying values incrementally. Never build from scratch until you understand the baseline.

---

#### 1.4.1 Lookup Tables (`.lut`)

High-fidelity cars do not hard-code every curve inside an `.ini`; instead they reference external **lookup tables** (`.lut`) that map an input variable to an output value. These are plain text files with space- or comma-separated `input output` pairs. A single top-tier mod can contain 200+ `.lut` files.

Common categories:

| Category | Example Filename | Description |
|----------|----------------|-------------|
| **Damper curves** | `fast_frebound1.lut`, `slow_rbump3.lut` | Front/rear, fast/slow shaft velocity, bump/rebound, per wheel |
| **Tyre curves** | `F_DX_S.lut`, `R_DY_H.lut` | Front/rear, longitudinal/lateral force, Soft/Medium/Hard/Wet compound |
| **Aero maps** | `BodyCL.lut`, `BodyCD.lut`, `map0_cl.2Dlut` | 2D lookups of downforce/drag vs speed, steer, roll, yaw, ride height |
| **Wing/device maps** | `FrontAdjuDeviceCL.lut`, `RearAdjuDeviceCD.lut` | Movable aero coefficients |
| **Engine/ERS** | `turbo0.lut`, `ers_torque.lut`, `engine_map0.lut` | Boost, hybrid torque, throttle maps |
| **ARB / brake / steering** | `arb_f.lut`, `awd_share.lut`, `setup_steering_ratio.lut` | Setup-linked controllers |
| **Camber/load** | `F_camber_S.lut`, `WET_camber_rsr.lut` | Camber sensitivity per compound |

> **📌 Note:** `.2Dlut` and `.2dlut` files are two-dimensional tables (input1 → input2 → output), used for maps that depend on two variables (e.g., throttle + RPM).

---

#### 1.4.2 Gear Ratio Files (`.rto`)

Each gear can be defined in its own **`.rto`** file (`first.rto`, `second.rto`, … `seventh.rto`, `final.rto`, `Neutral.lut`). This lets the gearbox use external ratio tables and per-gear final-drive or dog-box behaviors, referenced from `gearbox.ini`. `Neutral.lut` maps the neutral/idle state.

---

#### 1.4.3 CSP Lua Scripting (`script*.lua`)

Modern CSP cars extend the stock physics with **Lua scripts** that run inside Custom Shaders Patch. These are referenced from `data/script.lua` (and split helpers like `script__connection.lua`, `script__utils.lua`, `script__constants.lua`).

Typical scripts:

| Script | Purpose |
|--------|---------|
| `script_ecu.lua` | Engine control unit (throttle response, engine maps, limiter) |
| `script_traction_control.lua` | Driver aid: torque cut based on slip |
| `script_ers_kinetic.lua` | Hybrid/4WD torque management |
| `script_awd.lua` | All-wheel-drive torque split logic |
| `script_throttle_body.lua` | Throttle plate behavior |
| `script_engine_brake.lua` | Coast-down torque |
| `script_antiroll_bar_front/rear.lua` | Active ARBs |
| `script_brakes.lua` | Brake-by-wire, migration |
| `script_sensors.lua` | Physics sensor fusion |
| `script_antistall.lua` | Anti-stall logic |
| `script_cb_lights.lua` | Custom light behavior |

These override/augment the `.ini` values at runtime, enabling features the base engine cannot express (active aero, hybrid deployment strategies, custom TC). The `extension/data_override/` folder holds further Lua files (`force.lua`, `lights.lua`, `display.lua`) plus an HTML/JS `display/` for fully custom digital dashes.

---

#### car.ini — Global Parameters
```ini
[HEADER]
VERSION=1
[INFO]
NAME=My Car
[DATA]
STEER_RATIO=15.0
WHEELBASE=2.7
TRACKWIDTH=1.6
MASS=1250
CG_HEIGHT=0.45
INERTIA=850,1200,300   ; Pitch, yaw, roll
```

Defines: total mass, center of gravity height, weight distribution, body dimensions, inertial moments, and which sub-files are loaded.

> **⚠️ Critical:** `INERTIA` values are in kg·m² and control how the car rotates in pitch, yaw, and roll. These are often the most misunderstood parameters by new modders. Use a similar real car's published polar moments as a starting point.

---

#### engine.ini — Powertrain
- `POWER_CURVE` / `TORQUE_CURVE` — RPM-indexed tables (e.g., `360, 520.0` meaning 360 RPM produces 520 Nm).
- `LIMITER` — rev cut RPM.
- `MINIMUM_LIMITER_RPM` — idle floor.
- `IDLE_RPM`, `MAX_RPM`.
- `ENGINE_BRAKE` — coast-down behavior.
- `FUEL_CONSUMPTION` — liters per hour at full load.
- `TURBO` parameters (if applicable): threshold, lag, wastegate, pressure.

---

#### gearbox.ini / drivetrain.ini
- Gear ratios (1st through 6th/7th + reverse).
- Final drive ratio.
- Sequential vs H-pattern (`SHIFT_LIGHTS`, `HSHIFTER`).
- Clutch torque capacity, lockup.
- Differential(s): open, viscous, clutch-pack LSD, with power/torque bias and preload.
- Four-wheel-drive splits if AWD.

---

#### suspension.ini — Chassis Dynamics
- Per-wheel: spring rate, damper bump/rebound, bump stop, travel.
- Geometry: camber, toe, caster, KPI, scrub radius.
- Anti-roll bars (front/rear stiffness).
- Ride height, droop, bump.
- Wheel/tyre radius and width.

> **💡 Pro Tip:** This file is the most important for handling feel. Start from a similar Kunos car and adjust. The relationship between `suspension.ini` and `tyres.ini` is symbiotic — changing one usually requires re-tuning the other.

---

#### tyres.ini — Tyre Model
AC uses a Pacejka-style or simplified combined-slip tyre model. Parameters:
- `RADIUS`, `WIDTH`, `PRESSURE`.
- `TEMPERATURE_OPTIMAL`, `CORE`, `WARMUP`.
- Slip-curve coefficients (B, C, D, E) for lateral and longitudinal grip.
- `WEAR`, `DIRT`, `ROLLING_RESISTANCE`.
- Compound definitions (soft/medium/hard) with `PERFORMANCE` curves.

> **📌 Note:** A good tyre file is what separates an "arcade" mod from a realistic one. The Pacejka coefficients are non-intuitive; use AC's Car Tuner app to visualize slip curves while editing.

---

#### aero.ini — Aerodynamics
- `Cd` (drag coefficient) and `Cl` (lift/downforce).
- Front/rear wing efficiency and AoA (`WING` sections).
- Diffuser and floor effects.
- DRS zone reduction.
- Ground effect (modern cars).

---

#### brakes.ini — Braking
- `TORQUE` per corner, `BRAKE_BIAS` (front %), `DISTRIBUTION`.
- `COOLING` (duct efficiency), `MAX_TEMP`, `FADE`.
- `RELEASE_POWER` for ABS interaction.

---

#### electronics / driver aids
- `electronic_drivers.ini`: ABS modes, Traction Control (cut/retard), ESC, launch control, adjustable maps via CM.
- TC and ABS are simulated as real systems affecting torque, not scripted grip.

---

#### damage.ini
- Defines deformable nodes, collision thresholds, part failure (suspension, wheel, radiator), and how damage affects performance.

---

#### Validation
Use the in-game **Car Tuner** app or CM's "Physics Checker" to spot NaNs, impossible values, and force-feedback spikes. Test in Hotlap and with AI.

> **⚠️ Critical:** Always test with a full AI grid before release. Physics errors that are invisible in solo hotlapping often cause AI crashes or infinite loops.

---

### 1.5 Sound

Assetto Corsa audio is built on **FMOD Studio** (Firelight Technologies middleware). The sound bank file `sfx.bank` (and its GUID mapping `sfx_guids.txt`) contains all audio events; the game triggers them based on physics state (RPM, load, speed, gear, throttle, brake, slip, suspension travel, surface type). CSP extends the system with additional parameters, convolution reverb, and per-car DSP chains.

#### FMOD Project Structure

**Event Hierarchy** (recommended):
```
Car/
├── Engine/
│   ├── Intake_Int          # Internal induction
│   ├── Combustion_Int      # Internal combustion (main loop)
│   ├── Exhaust_Int         # Internal exhaust resonance
│   ├── Engine_Ext          # External mix (combustion + exhaust + intake)
│   ├── Turbo_Int / Turbo_Ext
│   ├── Wastegate_Int / Wastegate_Ext
│   ├── Blowoff_Int / Blowoff_Ext
│   ├── Starter_Int / Starter_Ext
│   └── Antistall_Int / Antistall_Ext
├── Transmission/
│   ├── Gearbox_Whine_Int / Gearbox_Whine_Ext
│   ├── Gear_Shift_Int / Gear_Shift_Ext      # Sequential clunk / H-pattern gate
│   ├── Clutch_Slip_Int / Clutch_Slip_Ext
│   ├── Diff_Whine_Int / Diff_Whine_Ext
│   └── Driveline_Clunk_Ext
├── Tyres/
│   ├── Scrub_Ext              # Rolling scrub (speed + load dependent)
│   ├── Slip_Ext               # Lateral slip (slip angle dependent)
│   ├── Lockup_Ext             # Brake lockup screech
│   ├── Flatspot_Ext           # Flatspot thump (per-wheel)
│   ├── Gravel_Ext / Grass_Ext / Kerb_Ext  # Surface-specific
│   └── Wet_Ext                # Aquaplane / wet scrub
├── Suspension/
│   ├── Bump_Ext               # Fast bump (kerbs)
│   ├── Rebound_Ext
│   ├── Bottomout_Ext          # Hard bottoming
│   └── Rattles_Int            # Interior panel rattles
├── Brakes/
│   ├── Brake_Squeal_Ext       # Pad vibration (temp + pressure)
│   ├── Brake_Cooling_Ext      # Duct whistle
│   └── Brake_Release_Ext
├── Aero/
│   ├── Wind_Int / Wind_Ext    # Wind noise (speed dependent)
│   ├── DRS_Open_Ext / DRS_Close_Ext
│   └── Wing_Flutter_Ext
├── Body/
│   ├── Impact_Light_Ext / Impact_Heavy_Ext
│   ├── Scrape_Ext             # Undertray / splitter scraping
│   ├── Door_Close_Int / Door_Close_Ext
│   └── Hood_Open_Ext
└── Ambience/
    ├── Cockpit_Amb_Int        # HVAC, electrical hum
    └── Garage_Amb_Ext
```

#### Parameters (must match AC/CSP expected names exactly)

| Parameter | Range | Description |
|-----------|-------|-------------|
| `RPM` | 0–12000 | Engine speed (primary modulator for engine events) |
| `Load` | 0–1 | Engine load (throttle × volumetric efficiency) |
| `Throttle` | 0–1 | Pedal position |
| `Speed` | 0–400+ | Vehicle speed km/h |
| `Gear` | -1–7 | Current gear (-1=R, 0=N) |
| `TurboBoost` | 0–2+ bar | Absolute boost pressure |
| `SlipFL/FR/RL/RR` | 0–2 | Per-wheel slip ratio |
| `SlipAngleFL/FR/RL/RR` | -40–40° | Per-wheel slip angle |
| `SuspensionTravelFL/FR/RL/RR` | -0.3–0.3 m | Per-wheel damper displacement |
| `SuspensionVelocityFL/FR/RL/RR` | -10–10 m/s | Per-wheel damper shaft speed |
| `BrakePressure` | 0–1 | Normalised brake pedal |
| `BrakeTempFL/FR/RL/RR` | 0–1200°C | Per-disc temperature |
| `SurfaceFL/FR/RL/RR` | 0–N | Surface type index (from `surfaces.ini`) |
| `RainIntensity` | 0–1 | CSP rain level |
| `IsInterior` | 0/1 | Listener inside cockpit (for EQ/occlusion) |
| `IsAI` | 0/1 | AI-controlled car (simpler mix) |

> **⚠️ Critical:** Parameter names are case-sensitive and must match exactly. A mismatch of even one character will cause the parameter to be ignored by the game, resulting in silent or incorrectly modulated events.

---

#### Core Events — Detailed Spec

##### Engine_Int (Main Internal Loop)
- **Type**: Scatterer Instrument or Multi-Sound with playlist
- **Layers**: 3–5 RPM-synchronous layers (idle, low, mid, high, limiter)
- **Modulation**: `RPM` → pitch (1:1), `Load` → volume crossfade between layers
- **Granular/Loop**: Seamless loops at each RPM band; crossfade width ~300–500 RPM
- **EQ**: Low-pass at high RPM for mechanical harshness; high-pass at idle for cabin filter
- **DSP**: Subtle saturation (tube/tape) driven by `Load`; convolution IR for intake resonance

##### Engine_Ext (Main External Loop)
- **Type**: Same layer structure as internal but different recordings
- **Volume**: +10 to +15 dB vs internal at 1 m
- **Directivity**: Use FMOD *Cone* attenuation: narrow forward lobe (exhaust), wider rear (intake)
- **Doppler**: Enabled (default)
- **Occlusion**: Car body occlusion via geometry (CSP raycasts) — low-pass + volume duck when behind obstacle

##### Turbo_Int / Turbo_Ext
- **Source**: Compressor whistle (synthetic or recorded)
- **Modulation**: `TurboBoost` → pitch + volume; `RPM` → spool lag (FMOD *AHDSR* on volume)
- **Wastegate_Ext**: Transient one-shot triggered when `TurboBoost` > `WastegateThreshold` (parameter from `engine.ini`)
- **Blowoff_Ext**: One-shot on throttle lift with `TurboBoost` > threshold; pitch varies with boost level

##### Transmission Events
- **Gearbox_Whine_Int/Ext**: `RPM` × `Gear` ratio → pitch; `Load` → volume; different sample per gear or pitch-shifted
- **Gear_Shift_Ext**: Multi-sound with variants (upshift/downshift, sequential/H-pattern); triggered by `Gear` change + `Clutch` disengage
- **Clutch_Slip_Ext**: `ClutchSlip` parameter (0–1) → volume; friction material texture

##### Tyre Events (Critical for Immersion)
- **Scrub_Ext**: Continuous. `Speed` × `Load` → volume; `Surface` → sample selector (asphalt/concrete/gravel/grass/kerb). Use *Scatterer* with grain size ~50 ms for texture.
- **Slip_Ext**: Lateral. `SlipAngle` → volume (threshold ~2°, peak ~8–12°); `Load` → pitch (heavier = lower). Separate *wet* layer crossfaded by `RainIntensity` × `SurfaceWetness`.
- **Lockup_Ext**: Longitudinal. `SlipRatio` > 0.15 → volume; `BrakeTemp` → pitch (hot = higher squeal). One-shot retrigger at ~20 Hz while locked.
- **Flatspot_Ext**: Per-wheel thump. Triggered when `FlatspotSeverity` > 0; period = `Speed` / (π × `Radius`); volume ∝ severity.

##### Suspension & Body
- **Bump_Ext / Rebound_Ext**: `SuspensionVelocity` → volume (fast bump > 2 m/s); `Surface` → sample (kerb = metallic, pothole = dull).
- **Bottomout_Ext**: `SuspensionTravel` ≤ `BumpStop` + 0.005 m → one-shot; `Load` → volume.
- **Impact_Ext**: `CollisionImpulse` parameter (from physics) → volume/pitch tiers (light/medium/heavy).
- **Scrape_Ext**: Continuous while `RideHeight` < `SplitterHeight` + 0.01 m; `Speed` → pitch.

##### Brake Events
- **Brake_Squeal_Ext**: `BrakePressure` × `BrakeTemp` (peak 300–600°C) → volume; `Speed` → pitch (slower = lower). Pad material variants (carbon/ceramic/steel).
- **Brake_Cooling_Ext**: `Speed` > 80 km/h + `BrakeTemp` > 200°C → duct whistle; `BrakeTemp` → volume.

##### Aero & Wind
- **Wind_Int**: `Speed` → volume + high-shelf EQ; `IsInterior` = 1 only. *Convolution* with cabin IR.
- **Wind_Ext**: `Speed` → volume; no EQ. Mirrors, A-pillars, roof racks add turbulence layers.
- **DRS_Open/Close_Ext**: One-shots triggered by `DRS` parameter (0/1); hydraulic actuator + flap rustle.

---

#### Building the Bank — Step-by-Step

1. **Create FMOD Studio project** (version matching CSP requirement — currently FMOD 2.02.xx).
2. **Import WAV assets** (48 kHz / 24-bit recommended; 96 kHz for high-res source). Organise into folders matching event hierarchy.
3. **Create Events** as above. Use *Multi-Sound* for layered loops, *Scatterer* for tyre/granular, *Single* for one-shots.
4. **Add Parameters** (right-click parameter deck → "Add Parameter"). Name **exactly** as in the table above.
5. **Modulation**: Drag parameter onto volume/pitch/effect knobs. Draw curves (right-click knob → "Add Modulation").
6. **Snapshot Mixing** (optional): Create snapshots `Interior`, `Exterior`, `Replay`, `LowSpeed`, `HighSpeed`; automate via `IsInterior`, `Speed`.
7. **Build**: *File → Build* → Desktop platform. Output: `sfx.bank`, `sfx_guids.txt`, `sfx.strings.bank` (optional).
8. **Copy** `sfx.bank` + `sfx_guids.txt` to `content/cars/<car_name>/sfx/`.
9. **Test in-game**: CM → Car → Sound tab → "Reload FMOD". Use `Ctrl+Shift+R` to hot-reload bank while driving.

---

#### Sample Sourcing & Production Tips

- **Engine**: Record at dyno (steady-state RPM holds) + on-track (load transitions). Minimum 8–10 RPM points per layer. Use *Izotope RX* to remove electrical noise, isolate orders.
- **Exhaust**: Place mic at 45° rear, 1 m; also close-mic (<0.2 m) for transient detail. Blend.
- **Turbo**: Record compressor housing (whistle) separate from wastegate/blowoff. Synthesize whistle with FM synthesis (operator ratio 2:1, index modulated by boost).
- **Transmission**: Drain gearbox oil, spin on bench for clean whine. Record shifts with load cell.
- **Tyres**: Skidpad at controlled slip angles (0–15°) and speeds (20–100 km/h). Multiple surfaces. Use *contact mic* on carcass for structure-borne thump.
- **Brakes**: Heat discs to 400–600°C; apply controlled pressure. Capture squeal harmonics (typically 1–6 kHz).
- **Loops**: Edit at zero-crossings; use *Reaper* or *RX* loop-tuning. Crossfade 50–100 ms. Verify in FMOD *Loop Region* audition.
- **Dynamic Range**: Leave 6–12 dB headroom in bank; master bus limiter at -1 dBFS. AC normalises to -18 LUFS; don't over-compress.

---

#### CSP Extensions

- **Convolution Reverb**: Assign per-event *Send* to `CSP_Reverb` return; CSP loads `reverb_<track>.ir` (track-provided IR) or default cabin/garage/tunnel IRs.
- **Positional Audio**: CSP enables *Geometry Occlusion* (raycast against track mesh + car body). Set event *Occlusion* = *Automatic*.
- **Per-Car DSP**: `ext_config.ini` `[AUDIO]` block can define `DSP_CHAIN=eq,compressor,saturator` with parameters driven by `RPM`, `Speed`, `Load`.
- **Rain/Aquaplane Layers**: Events with `RainIntensity` parameter get automatic wet-layer crossfade; CSP also triggers `Wet_Ext` tyre event when puddle depth > 1 mm.
- **Replay/TV Mix**: CSP reads `REPLAY_MIX` snapshot; attenuate engine, boost tyre/brake for broadcast feel.
- **Multiplayer Sync**: Bank must be identical client/server; `sfx_guids.txt` ensures event GUID match. No per-client audio settings in MP.

---

#### Common Pitfalls & Fixes

| Symptom | Cause | Fix |
|---------|-------|-----|
| Engine pitch "steps" | Too few RPM layers / granular grain size too large | Add layers; reduce grain to 20–40 ms; use *Pitch Shifter* DSP instead of sample rate |
| External too quiet inside car | Missing `IsInterior` attenuation | Add `IsInterior` → volume -12 dB + low-pass 800 Hz on external events |
| Tyre scrub repeats obviously | Scatterer grain too long / too few grains | Grain 30–80 ms; min 8 simultaneous grains; randomise pitch ±3% |
| Gear shift plays twice | Trigger on both `Gear` change and `Clutch` | Use *Trigger Condition*: `Gear` changed AND `Clutch` < 0.1 |
| Bank fails to load | FMOD version mismatch / missing `sfx_guids.txt` | Rebuild with exact FMOD version CSP uses; copy both files |
| Clipping at high RPM | Master bus over 0 dBFS | Add *Limiter* -1 dB ceiling; reduce layer gains |
| AI cars silent | `IsAI` parameter not handled | Add `IsAI` → volume 0 on interior events; external events at 50% volume for AI |

#### `sounds.ini` (Legacy)

Pre-FMOD config (AC 1.0–1.10). Still read as fallback if `sfx.bank` missing. Contains `[ENGINE]`, `[EXHAUST]`, `[TURBO]`, `[GEARBOX]`, `[TYRES]`, `[BRAKES]`, `[WIND]`, `[IMPACT]` sections with `FILE=sample.wav`, `VOLUME=`, `PITCH=`, `RPM_MIN/MAX=`. **Not recommended** for new mods — use FMOD.

---

### 1.6 Driver / Cockpit

The driver and cockpit are what the player sees in first-person and what AI opponents show externally. This is split into two parts: the **driver model** and the **cockpit/dashboard**.

#### Driver Model
- A separate `driver.kn5` (helmet, HANS device, torso, arms) placed in the `driver/` folder.
- `driver.ini` controls whether the driver is visible, head position, and animation.
- A simple mannequin is enough; detail can go into the helmet (team livery, sponsor).
- Animated hands on the wheel (`DRIVER_ARM_L/R`) sync with steering.
- For open-wheelers, the driver is fully visible; for GT, only helmet + arms above the dashboard.

#### Cockpit / Interior
- Part of `car.kn5` or separate `cockpit.kn5`. Contains dashboard, steering wheel, seats, paddles, switches.
- The 3D steering wheel should be named correctly so it rotates with `STEER_*` nodes.
- **dashboard.ini** defines the digital/analog gauge layout (RPM lights, speed, gear, fuel, temps). You can use a custom texture or the built-in hybrid display.
- **Mirrors**: `ksMirror` shader or CSP virtual mirrors; configure in `car.ini` mirror parameters.
- **Interior lights**: `lights.ini` for dashboard backlight and button illumination at night.

#### First-Person Camera
Defined in `car.ini` `[CAMERA]` sections: `ON_BOARD`, `HELMET`, `COCKPIT_HOOD`. Coordinates are relative to the driver's head. Good camera placement is essential for immersion.

#### Seat / Pedals Animation
`*.ksanim` files drive pedal travel and shifter movement linked to input. These are exported from the 3D app as keyframe animations named to match the bound node.

---

### 1.7 Driver Release Standards

> **Scope:** This section defines the release-quality criteria for the driver model, cockpit interior, animations, and first-person experience. A car with excellent physics and sound can still fail review if the driver/cockpit presentation is broken or immersion-breaking.

#### Driver Model Release Criteria

A release-quality driver model must satisfy the following standards:

**Geometry & LODs**
- **Primary driver mesh** (`driver.kn5`) must include: helmet (with visor), HANS device, race suit, gloves, and boots. Open-wheeler mods require full torso, arms, and legs visible from the cockpit.
- **Polygon budget:** Helmet ≤ 8k tris; full driver ≤ 25k tris for the highest LOD. Lower LODs should halve count at each step.
- **LODs:** Provide at least two levels: `driver_lod_a.kn5` (close-up, <15 m) and `driver_lod_b.kn5` (distant, >15 m). Distant LOD may omit gloves/boots and use simplified helmet geometry.
- **Visor:** Must use `ksGlass` or `ksCarPaint` shader with dynamic reflection. Visor tint should respect `lights.ini` day/night states (darker tint at night is acceptable if headlight glare is simulated).

**Materials & Textures**
- **Helmet livery:** Must ship with a default helmet texture (`helmet.dds`, 2048×2048 minimum) that matches the default skin theme. Include a `helmet_template.psd` in the `_dev/` or documentation folder for skinners.
- **Suit & gloves:** Use `ksPerPixel` or `ksPerPixelAT` with normal maps for fabric weave. Gloves should have separate material for grip texture on palms.
- **HANS:** `ksRubber` or `ksPerPixel` with subtle normal detail. Must not clip through the suit collar at any steering angle.
- **Skin integration:** `driver.ini` must support `SKIN_OVERRIDE` so skinners can replace `helmet.dds`, `suit.dds`, and `gloves.dds` via the skin folder without editing the base mod.

**Animation Standards**
- **Steering animation:** `steer.ksanim` must map 1:1 to the car's actual steering lock (`STEER_LOCK` in `driver3d.ini`). Hands must remain on the wheel rim at all lock angles; no "floating hands" or wrist clipping.
- **Shifting animation:** `shift.ksanim` (or `GEARCHANGE` node animation) must trigger within 80 ms of the gear-change event. For H-pattern cars, the hand must move to the correct gate position per gear. For sequential/paddle cars, left/right paddle animations must be distinct.
- **Pedal animation:** `PEDAL_THROTTLE`, `PEDAL_BRAKE`, `PEDAL_CLUTCH` nodes must travel the full physical range defined in the 3D model. Heel-toe technique should be visually plausible for road cars with H-pattern gearboxes.
- **Head animation:** `DRIVER_HEAD` must follow physics-based G-force movement via `driver3d.ini` `[HEAD_MOVEMENT]` with plausible filter values (0.10–0.25). Helmet should pitch/roll/yaw within the limits defined; excessive movement causes VR nausea.

**Driver Visibility Configuration**
- `driver3d.ini` must correctly set `[HIDE_OBJECT_*]` to hide the helmet and face in cockpit view while keeping arms and gloves visible. External views must show the full driver.
- For closed-cockpit GT cars: hide helmet/face in `CAMERA_0` (helmet cam), show only arms/gloves/steering wheel.
- For open-wheelers: full driver visible in all views except optional helmet-cam (which may hide the helmet interior to prevent clipping).

---

#### Cockpit Interior Release Criteria

The cockpit is the player's primary visual anchor during driving. Release-quality cockpits must meet these standards:

**Geometry & Optimization**
- **Cockpit mesh** (`cockpit.kn5` or interior nodes in `car.kn5`) must include: dashboard, steering wheel, seat, harness, pedals, shifter, switch panels, and major structural elements (roll cage, A-pillars, roof).
- **Polygon budget:** Interior ≤ 80k tris for the highest LOD. `cockpit_lod_a` (player car only) may use higher detail; `cockpit_lod_b` (AI/opponents) should drop to ≤ 30k tris.
- **Culling:** Interior parts not visible from the driver's seat should be deleted or moved to a separate "exterior-only" mesh to save GPU cost.

**Steering Wheel**
- Must be a separate node (`STEER_L` / `STEER_R` or `STEERING_WHEEL`) with correct pivot alignment to the steering column.
- Include functional buttons, dials, and LEDs mapped to `digital_instruments.ini` or `ext_config.ini` CSP display nodes.
- Wheel material should use `ksLeather` or `ksAlcantara` with normal map for grip texture. Carbon-fiber wheels should use `ksCarbonFiber` with weave direction aligned to the UVs.
- Shift-light LEDs (if present) must sync to `digital_instruments.ini` `LED_N` RPM switches and blink at the defined `BLINK_HZ`.

**Dashboard & Displays**
- **Analog gauges:** Needle pivots must be at the geometric center of the dial. Needle rotation must be smooth (no snapping) across the RPM/speed range.
- **Digital displays:** `dashboard.ini` or `digital_instruments.ini` must render correctly in all lighting conditions (day, dusk, night). Minimum brightness should be readable at noon; maximum should not bloom excessively in darkness.
- **CSP custom displays:** If using `extension/data_override/display/` HTML/JS dashboards, verify they load in CM's "Custom Shaders Patch" mode and fail gracefully to the default `dashboard.ini` if CSP is disabled.

**Mirrors**
- **Virtual mirrors (CSP):** `mirrors.ini` must define `MIRROR_0` (left), `MIRROR_1` (center/rearview), `MIRROR_2` (right). Node names must exist in the 3D model. CSP planar reflections must update at ≥ 30 Hz without ghosting.
- **Physical mirrors:** If using `ksMirror` shader on mirror geometry, verify the reflection plane normal faces the driver. Side mirrors should not reflect the car's own bodywork unless physically accurate.

**Interior Lighting**
- `lights.ini` must define cockpit illumination for night racing: dashboard backlight, button glow, and ambient footwell light.
- Emissive materials (`ksEmissive`) for buttons/switches should use low intensity (0.2–0.8) to avoid blowing out the exposure in night conditions.
- Interior light color temperature should match the era: modern cars = cool white/blue (5500–6500 K); vintage cars = warm white (3000–4000 K).

**Seat & Harness**
- Seat must use `ksFabric` or `ksLeather` with wrinkle normal maps. Harness straps must be geometry (not alpha planes) to cast correct shadows in CSP ray-traced interiors.
- Harness clip/buckle should be a distinct material with metallic specular.

---

#### First-Person Camera Release Criteria

- **Helmet cam (`DRIVEREYES` in `car.ini`):** Position must align with the driver's eye level in the 3D model. Height should be realistic (e.g., ~1.05–1.15 m above ground for a seated GT driver).
- **FOV:** Default 45–55° for realism; never exceed 70° as default (distorts gauges). Include a note in `description.ini` if the car is optimized for a specific FOV.
- **Headshake:** `SHAKE_MUL` in `car.ini` should be subtle (0.3–0.8). Excessive shake causes motion sickness in VR.
- **Exposure:** `ON_BOARD_EXPOSURE` must balance interior shadow detail with exterior visibility through windows. Avoid "black cockpit" syndrome where the driver cannot see switches in daylight.
- **Triple-screen / VR:** Verify that the steering wheel and dashboard are centered and that side mirrors are visible without extreme head rotation. Use CM's "Onboard Settings" to test IPD offsets.

---

#### Driver / Cockpit Packaging Standards

- `driver/` folder must contain: `driver.kn5`, `driver.ini`, and a `README_DRIVER.txt` explaining LOD distances and animation naming.
- Include `driver_template.psd` (or `_driver_template/`) with helmet, suit, and glove UV layouts for skinners.
- `ui_car.json` should list `driverModel: true` if the driver is custom (not a Kunos default).
- If using CSP-specific driver features (custom shaders, animated visor tear-offs, etc.), note the minimum CSP version in `description.ini`.

---

#### Driver Release Checklist (Quick Reference)

| # | Item | Pass/Fail | Notes |
|---|------|-----------|-------|
| 1 | Driver visible in external views | ☐ | All LODs show helmet + suit |
| 2 | Helmet/face hidden in cockpit view | ☐ | `HIDE_OBJECT_*` configured |
| 3 | Hands grip wheel at full lock | ☐ | No clipping or floating |
| 4 | Shifting animation synced to gear change | ☐ | ≤ 100 ms latency |
| 5 | Pedals travel full range | ☐ | Match suspension geometry |
| 6 | Head movement filtered naturally | ☐ | No VR nausea at 0.15–0.25 filter |
| 7 | Helmet visor reflects environment | ☐ | `ksGlass` or `ksCarPaint` shader |
| 8 | Default helmet matches default skin | ☐ | Thematic consistency |
| 9 | Steering wheel buttons/LEDs functional | ☐ | `digital_instruments.ini` verified |
| 10 | Dashboard readable day & night | ☐ | Backlight + emissive tuned |
| 11 | Mirrors render without ghosting | ☐ | CSP planar or `ksMirror` OK |
| 12 | Interior LODs switch correctly | ☐ | AI cars use low interior LOD |
| 13 | Seat & harness cast shadows | ☐ | CSP ray-tracing compatible |
| 14 | Cockpit camera position realistic | ☐ | Eye height ~1.05–1.15 m |
| 15 | Driver skin override works | ☐ | Skin folder replaces helmet/suit |



### 1.8 Detailed INI File Reference — Line-by-Line Parameter Guide

> **📌 Note:** This section provides exhaustive parameter documentation for the core `.ini` files. Use this as a reference when tuning, not as a first-read tutorial.

#### car.ini
The master vehicle descriptor: `[HEADER] VERSION=extended-2` enables CSP-extended physics; `[_EXTENSION] REQUIRED_VERSION=3281` mandates CSP 0.1.78+ and `SCRIPT_INPUTS_REPLAY` maps 11 custom script channels for replay recording. `[INFO] SCREEN_NAME` / `SHORT_NAME` set the UI labels. `[BASIC] GRAPHICS_OFFSET` (X,Y,Z) and `GRAPHICS_PITCH_ROTATION` position the visual model relative to the physics origin; `TOTALMASS=1110` kg and `INERTIA=1.055,0.52,3.68` (pitch, yaw, roll kg·m²) define the rigid-body mass properties. `[GRAPHICS]` places cameras: `DRIVEREYES` for helmet view, `ON_BOARD_EXPOSURE`/`OUTBOARD_EXPOSURE` for HDR tuning, `BONNET_CAMERA_POS/PITCH`, `BUMPER_CAMERA_POS/PITCH`, `CHASE_CAMERA_PITCH`, `MIRROR_POSITION`; `SHAKE_MUL` scales cockpit shake. `[CONTROLS] FFMULT` force-feedback gain, `STEER_ASSIST` enables speed-sensitive power steering, `STEER_LOCK=270°` max wheel rotation, `STEER_RATIO=13.6`, `LINEAR_STEER_ROD_RATIO` and `RACK_TRAVEL_LUT` link rack displacement to steer angle. `[FUEL] CONSUMPTION` (L/s at full load), `FUEL` start load, `MAX_FUEL` capacity; `[FUEL_EXT] KG_PER_LITER=0.76` fuel density. `[FUELTANK] POSITION` locates the tank CG for weight transfer. `[RIDE] PICKUP_FRONT/REAR_HEIGHT` set the aerodynamic reference plane height. `[PIT_STOP]` times in seconds for tyre change, fuel per litre, body/engine/suspension repair. `[RULES] MIN_HEIGHT=0` disables ride-height scrutineering.

#### engine.ini
Powertrain definition: `[HEADER] VERSION=1`, `POWER_CURVE=torque.lut` points to the main torque lookup, `COAST_CURVE=FROM_COAST_REF` uses the `[COAST_REF]` block, `POWER_CURVE_STANDARD=2` selects the LUT format, `HUMIDITY_REF=0`. `[ENGINE_DATA] AMBIENT_EFFECT=0` disables ambient-power correction, `ALTITUDE_SENSITIVITY=0.2`, `INERTIA=0.078` kg·m² crank inertia, `LIMITER=8000` RPM hard cut, `LIMITER_HZ=100` cut frequency, `MINIMUM=-1800` RPM idle floor, `DEFAULT_TURBO_ADJUSTMENT=1`. `[COAST_REF] RPM=8000 TORQUE=75 NON_LINEARITY=0` defines engine-brake torque at redline. `[COAST_DATA] COAST0/1/COAST` shape the coast curve. `[COAST_SETTINGS] LUT` array maps 7 gear-dependent coast multipliers, `DEFAULT=3` selects 4th gear map, `ACTIVATION_RPM=4000`. `[OVERLAP]` all zeros (no anti-lag). `[DAMAGE] TURBO_BOOST_THRESHOLD=1.1` bar over-boost damage trigger, `TURBO_DAMAGE_K=100`, `RPM_THRESHOLD=13000`, `RPM_DAMAGE_K=0.1`. `[TURBO_0]` & `[TURBO_1]` identical twin turbos: `LAG_DN/UP` spool rates, `MAX_BOOST=0.5` bar, `WASTEGATE=0.5`, `DISPLAY_MAX_BOOST=0.5`, `REFERENCE_RPM=3000`, `GAMMA=0.2` boost curve shape, `COCKPIT_ADJUSTABLE=0`. `[BOV] PRESSURE_THRESHOLD=0.75` bar blow-off valve. `[THROTTLE_RESPONSE] RPM_REFERENCE=8500`, `LUT=throttleref.lut` maps pedal-to-throttle by RPM. `[MAP] DEFAULT=2` selects `MAP_2=engine_map2.lut` (three power maps via `engine_map0/1/2.lut`).

#### suspension.ini
(not present in this car; superseded by `dampers.ini`, `bumpstops.ini`, `ctrl_*.ini`, and `setup.ini` script-driven suspension)

#### tyres.ini
Pacejka-MF6.1 tyre model with CSP thermal extension: `[HEADER] VERSION=10`, `[_EXTENSION] EXTENDED_RAYTRACING=0`, `LATERAL/LONGITUDINAL_RAYS=6`, `MAX_RAY_ANGLE=50`, `MAX_RAY_WIDTH_RATIO=1`, `DISABLE_RAY_DOUBLING=0`, `PRACTICAL_TEMP_RATIO=0.4`, `USE_NEW_RR_SLIP=1`. `[COMPOUND_DEFAULT] INDEX=1` selects Soft Cold as default. `[VIRTUALKM] USE_LOAD=1` enables virtual kilometre wear. `[EXPLOSION] TEMPERATURE=1000°C`. `[ADDITIONAL1] CAMBER_TEMP_SPREAD_K=1.3`, `PRESSURE_TEMPERATURE_GAIN=0.1125`, `BLANKETS_TEMP=45°C`. Each compound block (`[FRONT]`, `[REAR]`, `[FRONT_1]`, `[REAR_1]`, `[FRONT_2]`, `[REAR_2]`, `[FRONT_3]`, `[REAR_3]` for Soft Cold / Soft Hot / Medium / Wet) declares: `NAME/SHORT_NAME`, `WIDTH=0.29/0.34 m`, `RADIUS=0.355 m`, `RIM_RADIUS=0.2413 m`, `ANGULAR_INERTIA`, `DAMP`, `RATE` (spring rate N/m), `DY_CURVE/DX_CURVE` = `.lut` files for lateral/longitudinal Pacejka DY/DX coefficients, `DY0/1 DX0/1` offsets, `WEAR_CURVE`, `SPEED_SENSITIVITY`, `RELAXATION_LENGTH=0.1`, rolling resistance coefficients `ROLLING_RESISTANCE_0/1/SLIP`, `FLEX=0`, `CAMBER_GAIN`, `DCAMBER_0/1` linear camber stiffness, `DCAMBER_LUT` camber-sensitivity LUT, `DCAMBER_LUT_SMOOTH=1`, `FRICTION_LIMIT_ANGLE` peak-slip angle (deg), `XMU=0.2` friction fallback, pressure model: `PRESSURE_STATIC=22 psi`, `PRESSURE_SPRING_GAIN`, `PRESSURE_FLEX_GAIN`, `PRESSURE_RR_GAIN`, `PRESSURE_D_GAIN`, `PRESSURE_IDEAL=29 psi`, `FZ0=3000 N` nominal load, `LS_EXPY/EXPX` load sensitivity exponents, `DY_REF/DX_REF` reference friction, `FLEX_GAIN`, `FALLOFF_LEVEL/SPEED` grip drop-off, `CX_MULT` drag multiplier, `COMBINED_FACTOR=2.22` combined-slip weighting, `RADIUS_ANGULAR_K/K_2` centrifugal growth, `BRAKE_DX_MOD=0`, `DX_CAMBER_REF=-4°`, `DX_CAMBER_MULT=0.98`, `ROLLING_RADIUS_MULT=0.05`, `CAMBER_SPRING_MULT` = per-compound LUT, `FALLOFF_LEVEL_CURVE/SPEED_CURVE` = `*_flc.lut` / `*_fsc.lut`, `SIDEWALL_K_MULT/X`, `COMBINED_FACTOR_1/BRAKE_MULT/LOAD_K`, `DROPOFF_FACTOR_0/1`, `TYPE_HINT=SLICK`. Thermal blocks (`[THERMAL_FRONT/REAR]`, `[THERMAL2_FRONT/REAR]` and `_1/_2/_3` variants) define: `SURFACE/PATCH/CORE/INTERNAL_CORE_TRANSFER` heat-flow coefficients, `FRICTION_K` grip-temperature sensitivity, `SURFACE_ROLLING_K`, `ROLLING_K`, `PERFORMANCE_CURVE` = `*_T_*.lut` grip vs temp, `GRAIN_GAMMA/GAIN`, `BLISTER_GAMMA/GAIN`, `COOL_FACTOR`. `THERMAL2` adds carcass model: `CARCASS_ROLLING_K`, `BRAKE_TO_CORE`, `SURFACE_TO_AMBIENT/CARCASS`, `CARCASS_TO_SURFACE/CORE`, `CORE_TO_CARCASS/AMBIENT`, `FLEX_SPEED_COEFF`, `FRICTION_TEMP_K`. Wet compounds add `TREAD_COVER=0.7`, `TREAD_DEPTH=0.008 m`.

#### aero.ini
CSP aero map with 5 wing elements and 2D ride-height/roll/yaw/steer/speed maps: `[HEADER] VERSION=3`. `[WING_0] NAME=THR` tiny throat reference wing at `POSITION=0,-0.28,0` with `LUT_AOA_CL/CD=THRCL/CD.lut`. `[WING_1] NAME=RearAdjuDevice` main rear wing `CHORD=0.25 SPAN=1.7 POSITION=0,0.62,-2.188632`, `LUT_AOA_CL/CD=RearAdjuDeviceCL/CD.lut`, `ANGLE=50°` max flap, rear zone weights `ZONE_REAR_CL/CD=0.08`. `[WING_2] NAME=RearAdjuDevice_G_` gurney flap `CHORD=0.02`, same span/pos, `ANGLE=0`. `[WING_3] NAME=Body` chassis underbody `CHORD=0.5 SPAN=0.5 POSITION=0,0.12,0`, `LUT_AOA_CL/CD=BodyCL/CD.lut`, side zones `ZONE_LEFT/RIGHT_CL/CD=0.08`. `[WING_4] NAME=Body_G_` underbody gurney `CHORD=0.2`, `ANGLE=50°`. `[MAP_0] NAME=DEFAULT` references twelve `.2Dlut`/`.lut` files for `MAP_CL_RH/CD/BALANCE_RH`, `MAP_YAW_CL/CD/BALANCE`, `MAP_ROLL_CL/CD/BALANCE`, `MAP_STEER_CL/CD/BALANCE`, `MAP_SPEED_CL/CD/BALANCE`; `FRONT/REAR_RH_OFFSET=0`, `CL/CD_MULT=1`, `BALANCE_OFFSET=0`, `INTERPOLATION=LINEAR`, `DRAG_OFFSET=0,0`, `FRONT/REAR/DRAG_FORCE_DELAY=0.05` s. `[DYNAMIC_CONTROLLER_0]` drives `WING=0` (THR) from `INPUT=GAS` via `driver3d_anim.lut` with `FILTER=0.9`; `[DYNAMIC_CONTROLLER_1]` drives `WING=2` (gurney) from `INPUT=BRAKE` same LUT.

#### brakes.ini
`[HEADER] VERSION=2`, `[_EXTENSION] ENABLE=1 USE_ADVANCED_SYSTEM=0`. `[DATA] MAX_TORQUE=4680.28 Nm` total, `FRONT_SHARE=0.59` bias, `HANDBRAKE_TORQUE=0`, `COCKPIT_ADJUSTABLE=1`, `ADJUST_STEP=0.5%`. `[TEMPS_FRONT/REAR] TRANSFER=0.4` heat-in, `COOL_TRANSFER=0.002` air cooling, `TORQUE_K=0.4` fade coefficient, `PERF_CURVE` piecewise temp vs torque multiplier (0=0.34, 400=1.0, 900=0.966, 1200=0.59, 2000=0.34), `COOL_SPEED_FACTOR=0.06` / `0.04`, `COOL_SPEED_FACTOR_0=0.05`, `CORE_TRANSFER_IN=0.16`, `CORE_TRANSFER_OUT=0.5`, `CORE_TRANSFER_AIR=0.0008/0.0005`, `DEFAULT_BLANKING=0`. `[DISCS_GRAPHICS]` maps 3D disc nodes `GEO_Disc_LF/RF/LR/RR`, `FRONT/REAR_MAX_GLOW` temperature for full glow, `LAG_HOT=0.997`, `LAG_COOL=0.99` visual thermal inertia.

#### drivetrain.ini
`[HEADER] VERSION=3`. `[TRACTION] TYPE=AWD2` (CSP AWD with centre diff). `[GEARS] COUNT=7`, ratios `GEAR_R=-3.134`, `GEAR_1..7 = 2.617, 2.094, 1.718, 1.457, 1.264, 1.106, 1.015`, `FINAL=3.196`. `[DIFFERENTIAL] POWER=0 COAST=0 PRELOAD=0` (rear diff, overridden by AWD2). `[GEARBOX] CHANGE_UP_TIME=20 ms`, `CHANGE_DN_TIME=40 ms`, `AUTO_CUTOFF_TIME=30 ms`, `SUPPORTS_SHIFTER=0`, `VALID_SHIFT_RPM_WINDOW=1000`, `CONTROLS_WINDOW_GAIN=0.5`, `INERTIA=0.01 kg·m²`. `[CLUTCH] MAX_TORQUE=700 Nm`. `[AUTOCLUTCH]` all `NONE` profiles, `USE_ON_CHANGES=1`, `MIN/MAX_RPM=1000/5800`, `FORCED_ON=0`. `[UPSHIFT_PROFILE]` / `[DOWNSHIFT_PROFILE]` `POINT_0/1/2` clutch bite points. `[AUTOBLIP] ELECTRONIC=1`, `POINT_0/1/2` blip profile, `LEVEL=0.9`. `[AUTO_SHIFTER] UP=7750 DOWN=5800 RPM`, `SLIP_THRESHOLD=1.05`, `GAS_CUTOFF_TIME=0.04 s`. `[DOWNSHIFT_PROTECTION] ACTIVE=1`, `DEBUG=0`, `OVERREV=1200 RPM`, `LOCK_N=1`. `[AWD2]` centre/rear split: `FRONT_DIFF_POWER=0.925 COAST=0 PRELOAD=0`, `CENTRE_RAMP_TORQUE=800 Nm`, `CENTRE_MAX_TORQUE=0`, `REAR_DIFF_POWER=0.17 COAST=0.58 PRELOAD=55 Nm`.

#### electronics.ini
ABS/TC/EDL configuration: `[ABS_V2] SLIP_RATIO_LIMIT=0.12`, `CURVE=` empty, `PRESENT=0 ACTIVE=0` (disabled by default), `RATE_HZ=100`, `CHANNELS=4`. `[TRACTION_CONTROL] SLIP_RATIO_LIMIT=0.08`, `CURVE=setup_tc_target_slip.lut` (setup-adjustable), `PRESENT=0 ACTIVE=0`, `RATE_HZ=85`, `MIN_SPEED_KMH=5`. `[EDL] PRESENT=0 ACTIVE=0`, `MAX_SPIN_POWER=0.55 COAST=0.4`, `BRAKE_TORQUE_POWER=140 COAST=420 Nm`, `DEAD_ZONE_POWER=0.12 COAST=0.12`.

#### damage.ini
Visual damage thresholds: `[SCRATCHES] MAX_SPEED=50 MIN_SPEED=0 km/h`. `[OSCILLATIONS] ENABLED=1`. Glass breakage: `[DAMAGE_GLASS_FRONT] MIN_SPEED=30 FULL_SPEED=100`, `[DAMAGE_GLASS_REAR] MIN_SPEED=50 FULL_SPEED=200`, `[DAMAGE_GLASS_LEFT/RIGHT] MIN_SPEED=80 FULL_SPEED=200`, `[DAMAGE_GLASS_CENTER] MIN_SPEED=50 FULL_SPEED=200`. `[DAMAGE] INITIAL_LEVEL=20%` pre-damage for replay continuity.

#### lights.ini
`[HEADER] VERSION=3`, `FLASHING_BLINK_TIME=0.15 s`, `FLASHING_REPEAT=8`. Each `[BRAKE_0]` / `[LIGHT_0..17]` block: `NAME=` 3D node, `COLOR=R,G,B` intensity (linear), `OFF_COLOR=` idle emission, `FLASH=0/1` indicator/headlight flash, `PITLINE=0/1` pit-limiter active, `SPECIAL=0/1` rain light. Covers: rear brake/tail/indicator/rain lights (`GEO_REARLIGHTS_0/1/2`), 7 headlight elements (`GEO_HEADLIGHTS_0_OUTER/INNER/UPPER/MID`, `GEO_HEADLIGHTS_1/2`), steering-wheel display LEDs (`DISPLAY_WHEEL`, `DISPLAY_MONITOR`, `GEO_INT_SteeringWheel_Red/Green/Blue/LightBlue/Yellow/Blue2`), generic cockpit (`GEO_GEN_INTLIGHT`, `GEO_CPIT_GEN_PANEL`).

#### ers.ini
Hybrid system (MGU-K + MGU-H): `[HEADER] VERSION=1`. `[KINETIC] CHARGE_K=1e-7` regen efficiency, `TORQUE_CURVE=ers_torque.lut`, `COAST_CURVE=ers_coast_torque.lut`, `DISCHARGE_TIME=53640.87 s` (energy capacity), `HAS_BUTTON_OVERRIDE=0`, `MAX_KJ_PER_LAP=200000` (200 MJ), `DEFAULT_CONTROLLER=1` (script), `BRAKE_REAR_CORRECTION=0`. `[HEAT] CHARGE_K=0`, `TORQUE_PERC=0` (MGU-H disabled). `[FRONT_MOTORS] TORQUE_CURVE=ers_front_torque.lut`, `DISCHARGE_TIME=53640.87`, `FRONT_TORQUE_VECTORING_BIAS=0`. `[COCKPIT_CONTROLS] RECOVERY=0`, `DELIVERY_PROFILE=1`, `MGU_H_MODE=0`.

#### driver3d.ini
Driver model & animation: `[MODEL] NAME=driver`, `POSITION=0,0,0`. `[STEER_ANIMATION] NAME=steer.ksanim`, `LOCK=360°` matches steering rack. `[SHIFT_ANIMATION] BLEND_TIME=50 ms`, `POSITIVE_TIME=100 ms` upshift, `STATIC_TIME=10 ms`, `NEGATIVE_TIME=100 ms` downshift, `PRELOAD_RPM=7800`, `INVERT_SHIFTING_HANDS=0`. `[HIDE_OBJECT_0] NAME=DRIVER:HELMET`, `[HIDE_OBJECT_1] NAME=DRIVER:GEO_Driver_FACE` (hidden in cockpit view). `[HEAD_MOVEMENT] FILTER=0.15` G-filter, `MAX_G=2.0`, `ROLL_MAX_DEG=-12°`, `PITCH_MAX_DEG=-15°`, `YAW_MAX_DEG=20°` per G.

#### cameras.ini
Six onboard cameras: each `[CAMERA_N] POSITION=(X,Y,Z) m`, `FORWARD=(X,Y,Z)` unit vector, `UP=(X,Y,Z)` unit vector, `FOV` degrees, `EXPOSURE` HDR bias, `EXTERNAL_SOUND=0/1` (0=interior mic, 1=exterior). `CAMERA_0` helmet cam slightly left/forward, `CAMERA_1` high cockpit, `CAMERA_2` low nose, `CAMERA_3` centre-top roll-hoop rearward, `CAMERA_4/5` left/right trackside TV cams with external sound.

#### setup.ini
Massive 1214-line setup menu definition for CM/CSP. `[DISPLAY_METHOD] SHOW_CLICKS=1`. `[GEARS] USE_GEARSET=1` enables gear-set selector. Adjustable items each have a block: `[WING_1]` rear wing `MIN=0 MAX=100 STEP=5` clicks, `TAB=AERO`, `HELP=HELP_REAR_WING`, `PITSTOP=5 s`. `[WING_4]` air vents `MIN=0 MAX=100 STEP=50`, `TAB=VISUALS`. Four tyre pressures `[PRESSURE_LF/RF/LR/RR] MIN=20 MAX=30 STEP=1 psi`, `TAB=TYRES`. `[ARB_F/R]` front/rear ARB `MIN/MAX/STEP` in N/m, `TAB=SUSP:MAIN`, `EXT_CONTROLLER=ctrl_arb_front/rear.ini` links to script. Four cambers `[CAMBER_LF/RF/LR/RR] MIN/MAX/STEP` degrees, `TAB=SUSP:ALIGNMENT`. `[FRONT_BIAS]` brake bias `MIN=0 MAX=100 STEP=0.5%`. `[STEER_ASSIST]` FFB linearity `MIN=0 MAX=200`. `[BRAKE_POWER_MULT]` brake pressure `MIN=20 MAX=120%`, `EXT_CONTROLLER=ctrl_brake_power.ini`. `[ENGINE_MAPS]` ICE map selector `LUT=setup_engine_maps.lut`. `[BRAKE_DUCT_F/R]` ducting `MIN=0 MAX=100 STEP=10%`. 31 `[CUSTOM_SCRIPT_ITEM_N]` blocks expose script variables to the setup UI: `ID=` script variable, `TAB=` category, `NAME=` label, `MIN/MAX/STEP/DEFAULT`, `LUT=` for mapped values, `POS_X/Y` grid position, `HELP=` tooltip, `UNITS=`, `DISPLAY_MULTIPLIER=` for scaling. IDs include: `ALLOWED_POWER`, `TC_TARGET_SLIP`, `TC_CUTOFF_MAP`, `ARB_REAR`, `DEPLOYMENT_STRAT`, 4× deployment strat speed windows, `BOP_DRY/WET_MIN_SPEED`, `STINT_MAX_MJ`, `TORQUE_SPLIT`, `GRIP_LEVEL`, `BRAKE_MIGRATION` ±5% with `RAMP_UP/DOWN` pedal %, `BRAKE_BIAS_COARSE` 50–62% ×0.1 display, `BRAKING_LEVEL` 70–120%, `POWER_GAIN_SPEED/PERC`, `REGEN_LEVEL` 0–100%, `ARB_FRONT`, `TC_LAT/LONG`, `PEDAL_MAP` 0–4, `MAX_ERS_POWER` 40–120 kW, `REAR_DIFF_POWER/COAST/PRELOAD`. Two gear sets `[GEAR_SET_0/1]` with 7 ratios each. Bump-stop gaps `[BUMPSTOP_GAP_LF/RF/LR/RR_C0/C1]` per-corner & heave `MIN/MAX/STEP mm`. Bump-stop rates `[BUMPSTOP_LF/RF/LR/RR_C0/C1]` via `LUT=setup_*.lut`. Heave springs `[SPRING_RATE_LF/LR_C1] MIN/MAX/STEP N/m`. Torsion bars `[TORSION_RATE_LF/RF/LR/RR_0] MIN/MAX/STEP Nm/deg`. Ride-height offsets `[DJ4_LENGTH_OFFSET_LF/RF/LR/RR] MIN/MAX/STEP mm ×0.1`. Toe links `[STEER_JOINT_0_LENGTH_OFFSET_LF/RF/LR/RR] MIN/MAX/STEP mm`. Dampers 4-per-corner ×2 heave: `[DAMPER_BUMP/FAST_BUMP/REBOUND/FAST_REBOUND_LF/RF/LR/RR_C0/C1] MIN/MAX/STEP/DEFAULT` clicks. Steering ratio `[STEER_JOINT_0_POS_B_SETTING_LF/RF] LUT=setup_steering_ratio.lut`. Experimental weight distribution `[DJ6_LENGTH_OFFSET_LR] LUT=setup_shifter_wb.lut`.

#### fuel_cons.ini
`[FUEL_EVAL] KM_PER_LITER=2.308508` — single scalar used by the ECU script to estimate stint range and fuel-per-lap; the script multiplies this by current fuel mass to project remaining laps.

#### hybrid.ini
(not present; hybrid logic in `ers.ini` + CSP scripts)

#### thermal.ini
(not present; tyre thermal in `tyres.ini` `[THERMAL*]` blocks)

#### drs.ini
(not present; DRS via aero `WING_1` dynamic controller + script)

#### mirrors.ini
Three mirrors: `[MIRROR_0] NAME=MIRROR_L` (left), `[MIRROR_1] NAME=MIRROR_C` (centre), `[MIRROR_2] NAME=MIRROR_R` (right) — node names must exist in the 3D model; CSP uses these for real-time planar reflections.

#### lods.ini
LOD switching distances (metres): `[COCKPIT_HR] DISTANCE_SWITCH=5` (high-res cockpit <5 m), `[DRIVER_HR] DISTANCE_SWITCH=25` (high-res driver <25 m). Four mesh LODs: `[LOD_0] FILE=vrc_pt_2024_ferrenzo_a.kn5 IN=0 OUT=15`, `[LOD_1] FILE=vrc_pt_2024_ferrenzo_b.kn5 IN=15 OUT=40`, `[LOD_2] FILE=vrc_pt_2024_ferrenzo_c.kn5 IN=40 OUT=175`, `[LOD_3] FILE=vrc_pt_2024_ferrenzo_d.kn5 IN=175 OUT=2000`.

#### colliders.ini
`[_EXTENSION] VERSION=2`. `[COLLIDER_0] CENTRE=0,-0.27,0` `SIZE=0.355,0.02,3.1` (half-extents: radius 0.355 m, height 0.02 m, length 3.1 m) `GROUND_ENABLE=1` — single flat ground collider plate under the car for track-surface interaction.

#### ambient_shadows.ini
`[SETTINGS] WIDTH=1.10931360721588 LENGTH=2.76095461845398` — dimensions (metres) of the baked ambient-occlusion shadow caster rectangle projected under the car.

#### blurred_objects.ini
Wheel-blur LOD objects per corner (indices 0=LF, 1=RF, 2=LR, 3=RR): eight `[OBJECT_N]` blocks alternating static rim (`RIM_LF/RF/LR/RR` `MIN_SPEED=0 MAX_SPEED=50 km/h`) and blurred disc (`RIM_BLUR_LF/RF/LR/RR` `MIN_SPEED=30 MAX_SPEED=10000 km/h`).

#### digital_instruments.ini
536-line CSP digital dash definition. `[KERS_CHARGE_SERIE_0] PREFIX=KERS_LEVEL_ START_INDEX=1 END_INDEX=34` — 34-frame animated KERS bar texture. 27 `[ITEM_N]` blocks on `PARENT=DISPLAY_WHEEL` (steering-wheel screen) with `TYPE=GEAR/SPEED/RPM/LAPTIME/FUEL/PERF/PRESSURE/TC_LEVEL/WATER_TEMP/LAST_LAP/FUEL_CONS/KERS_CHARGE/CURRENT_LAP/POSITION_CAR/GEAR_COLOR/BEST_LAP`, each with `POSITION=(x,y,z)`, `SIZE`, `COLOR=R,G,B`, `INTENSITY`, `FONT=german_led`, `VERSION=2`, `ALIGN=CENTER/LEFT/RIGHT`, `DECIMALS`, `PREFIX/POSTFIX`, `UNITS=KPH/KML`, `TYRE_NUMBER=0..3`, `N_TIME=0.1`, `COLOR_2/INTENSITY_2/RPM_TRIGGER=8000` for gear-shift light. 12 `[LED_N]` RPM shift LEDs `OBJECT_NAME=LED_RPM_2..12` with `RPM_SWITCH=7300..7600`, `EMISSIVE=R,G,B`, `DIFFUSE=0.2`, `BLINK_SWITCH=8000`, `BLINK_HZ=5`; one ERS LED `LED_RPM_1` `RPM_SWITCH=100`; one KERS-enabled LED `KERS_ENABLED_0 OBJECT_NAME=LED_RPM_1 EMISSIVE=200,10,0 DIFFUSE=0.2`. 20 `[TYRE_LOCK_SLIP_N]` per-corner slip LEDs (5 per front wheel) `OBJECT_NAME=LED_SLIP_L/R_1..5`, `TYRE_INDEX=0/1`, `EMISSIVE=0,0,160`, `EMISSIVE_LOCK=0,0,0`, `DIFFUSE=0.25`, `SLIP_SWITCH=0.6..1.0`, `SHOW_LOCK=0`, `WHEEL_SPEED_MULT=0.8`. Secondary display `PARENT=DISPLAY_MONITOR` (dash LCD): 8 `[ITEM_19..26]` `TYPE=PLACE_HOLDER` static labels "Lap Time:/Last Lap:/Best Lap:/Delta:" + `TYPE=LAPTIME/LAST_LAP/BEST_LAP/PERF` dynamic values.

#### dash_cam.ini
`[DASH_CAM] POS=0.146,0.753,0.443365` — dashboard camera offset (metres) from driver eyes for photo-mode / replay.

#### flames.ini
`[HEADER] EDIT_MODE=0 INTENSITY=30 EDIT_BIG=1 EDIT_STATE=4 BURN_FUEL_MULT=4 FLASH_THRESHOLD=8`. Two exhaust flame emitters `[FLAME_0/1] POSITION=(±0.09009,0.62567,-1.31893)`, `DIRECTION=(∓0.13012,0.01421,-0.99140)`, `VSIZE_START=0.1 VSIZE_END=0.02 LSIZE=0.1 SIZE_MULT=0.5 IS_LEFT=0 GROUP=0`.

#### flame_presets.ini
`[HEADER] SIZE_MULT=1`. Particle presets: `[PRESET_START_0/1/2]`, `[PRESET_END_0/1/2]`, `[PRESET_LOOP_0/1/2]`, `[PRESET_FLASH_0/1/2]` — each with `TRIGGER=0/1/2` (start/end/loop/flash), `OFFSET=(x,y,z)`, `SIZE`, `TYPE=0/1/2` (particle type), `RGB=R,G,B,Alpha`.

#### sounds.ini
(not read; legacy sound config, superseded by FMOD `sfx.bank`)

#### ai.ini
`[HEADER] VERSION=4`. `[GEARS] UP=7600 DOWN=76.3 SLIP_THRESHOLD=1.02 GAS_CUTOFF_TIME=0.03` — AI shift points. `[PEDALS] GASGAIN=4 BRAKE_HINT=0.88 TRAIL_HINT=0.5`. `[STEER] STEER_GAIN=1.8`. `[LOOKAHEAD] BASE=10 GAS_BRAKE_LOOKAHEAD=5 SPEED_GAIN=0.1`. `[ULTRA_GRIP] VALUE=1.2` AI grip multiplier. `[PHYSICS_HINTS] AERO_HINT=1`. `[TYRES] HINT=1`. `[WING_VARIATION] VALUE=0`.

#### ai_tyres.ini
Per-compound AI tyre life (km before mandatory pit): `[S] MAX_KM=200`, `[M] MAX_KM=250`, `[H] MAX_KM=350`, `[WET] MAX_KM=250`.

#### ctrl_*.ini
Script controllers referenced by `setup.ini` `EXT_CONTROLLER`: each `[CONTROLLER_0]` (some have multiple) with `COMBINATOR=ADD/MULT`, `INPUT=SCRIPT_N/GAS/RPMS/SPEED_KMH`, `LUT=*.lut` or inline `(| 0=0 | 200=200 |)`, `FILTER=0/0.9/0.95/0.99`, `UP_LIMIT/DOWN_LIMIT`. Files: `ctrl_arb_front.ini` (INPUT=SCRIPT_11, arb_f.lut, 85000–165000), `ctrl_arb_rear.ini` (SCRIPT_10, arb_r.lut, 0–84000), `ctrl_brake_power.ini` (SCRIPT_3, inline, 0–110), `ctrl_ebb.ini` (SCRIPT_2, inline, 0–1), `ctrl_front_share.ini` (SCRIPT_0, awd_share.lut, 0–100, FILTER=0.95). Four rear ERS maps `ctrl_ers_0..3.ini`: each `[HEADER] NAME=1..4`, three controllers: `CONTROLLER_0 ADD SCRIPT_1 *_SCRIPT_1.lut`, `CONTROLLER_1 MULT GAS *_GAS.lut`, `CONTROLLER_2 MULT RPMS *_RPMS.lut`. Four front ERS maps `ctrl_ers_front_0..3.ini`: each `NAME=1..4`, `CONTROLLER_0 ADD GAS *_FRONT_GAS.lut FILTER=0.9`, `CONTROLLER_1 MULT SPEED_KMH *_FRONT_SPEED_KMH.lut`, `CONTROLLER_2 MULT SCRIPT_4 *_FRONT_SCRIPT_4.lut`. Two turbo controllers `ctrl_turbo0/1.ini`: `INPUT=RPMS ADD turbo0/1.lut FILTER=0.99 UP_LIMIT=20500`.

#### script_*.lua
`script.lua` (92 lines) — dynamic module loader: scans car data files for `script__*.lua` (init modules) and `script_*.lua` (main modules), requires them in order (`script_sensors`, `script_ecu` first), wraps each into a combined `script.update(dt)` and `script.reset()` with per-module caches. `script_ecu.lua` (547 lines) — full ECU simulation: classes `AWD` (front torque, rear diff power/coast/preload), `ANTIROLL_BAR_FRONT/REAR` (controllerInputs[11/10]), `GEAR_BOX` (antistall clutch), `POWER_UNIT` (stint energy tracking MJ, lap/pitstop reset, fuel-used-last-lap), `IGNITION` (boot sequence, starter/bump-start, engine kill, race auto-start), `ENGINE` (pit-limiter throttle, fuel consumption, engine start/run/stall/antistall, RPM override, turbo wastegate), `HYBRID` (MGU-K energy store 200 MJ, charge/discharge, front MGU-K torque vectoring, regen level, overtake button), `EBB` (electronic brake-by-wire: brake migration curve via smootherstep, kers torque compensation, dynamic controllerInputs[2/3] for bias/pressure), `AUDIO` (replay data packet: 11 boolean/float channels to controllerInputs[100..109]). Instantiates all, runs `PLAYER_ECU` (full) or `AI_ECU` (simplified) per frame. `script_traction_control.lua` (135 lines) — player TC control: reads `CustomSetupItem` for `TC_TARGET_SLIP` (LUT `setup_tc_target_slip.lut`), `TC_CUTOFF_MAP` (LUT `setup_tc_cutoff_coeff.lut`), `TC_LAT` (LUT `setup_tc_lat.lut`), `TC_LONG` (LUT `setup_tc_long.lut`); loads 2D LUT maps from `TC_TARGET_SLIP_INI` and `TC_CUTOFF_INI` `[MAP]` sections; binds steering-wheel buttons (`CONTROLS.TC_TARGET_SLIP_DN/UP`, `TC_CUTOFF_MAP_DN/UP`, `TC_FINE_PICKUP_DN/UP`, `TC_FINE_EXIT_DN/UP`) to adjust setup items with on-screen messages; runs at `TC_FREQUENCY` Hz, computes slip vs target, applies torque cut via cutoff maps. Other scripts (not read): `script_ers_kinetic.lua`, `script_awd.lua`, `script_antiroll_bar_front/rear.lua`, `script_brakes.lua`, `script_ecu.lua`, `script_engine_brake.lua`, `script_idle.lua`, `script_sensors.lua`, `script_starter.lua`, `script_throttle_body.lua`, `script_torque_split.lua`, `script_antistall.lua`, `script_bump_start.lua`, `script_cb_lights.lua`, `script__allocate/config/constants/controls/utils/ai.lua` — implement ERS deployment strategies, AWD torque split, active ARBs, brake migration, engine braking, anti-stall, bump-start, custom dash lights, sensor fusion, torque vectoring.

- `ui_car.json` — controls how the car appears in the selector: `name`, `brand`, `class`, `tags`, `specs` (power, weight, top speed, year), and `torqueCurve`/`powerCurve` graphs for display.
- `badge.png` / `brand.png` — icons (256×256, transparent PNG).

> **📌 Note:** Testing loop: (1) Load in showroom — check 3D, skins, UI. (2) Hotlap — check physics, FFB, sound. (3) Race with AI — check they don't collide oddly. (4) Night + rain (CSP) — check lights and wet behavior. (5) Multiplayer — verify all players see the car identically.

> **💡 Pro Tip:** Packaging: Compress the `my_car` folder to `.7z` or `.zip`. Include a `description.ini` and a `README` (credits, install steps, known issues). Distribute via RaceDepartment, the AC forum, or a personal site.

---

## Category 2 — Tracks

A track mod creates a new circuit, environment, and racing surface. Tracks are more complex than cars because they must include AI paths, cameras, surface physics, and often weather/night systems.

### 2.1 Folder Structure

```
content/tracks/my_track/
├── data/
│   ├── track.ini           # Master config (length, pits, direction, GPS)
│   ├── surfaces.ini        # Surface type → physics properties
│   ├── cameras.ini         # TV / replay / trackside cameras
│   ├── pit.ini             # Pit lane limits, speed, positions
│   ├── timing.ini          # Sectors, start/finish, speed traps
│   ├── drs_zones.ini       # DRS detection/activation zones
│   └── ai/
│       ├── fast_lane.ai    # Optimal racing line
│       ├── left_lane.ai    # Left boundary
│       ├── right_lane.ai   # Right boundary
│       ├── pit_lane.ai     # Pit lane path
│       ├── hint_corner.ai  # Corner speed hints
│       ├── hint_apex.ai    # Apex hints
│       └── hint_overtake.ai# Overtaking zones
├── 3d/
│   ├── track.kn5           # Main mesh (asphalt, kerbs, walls, scenery)
│   ├── objects/            # Separate detail objects
│   └── *.ksanim            # Animated flags, lights, gates
├── textures/
│   └── *.dds               # All track textures
├── materials/
│   └── *.mtl               # Material/shader definitions
├── extension/
│   ├── config/
│   │   ├── ext_config.ini  # Enable CSP features
│   │   ├── weather_*.ini   # Weather presets
│   │   ├── track_lights.ini# Night lighting
│   │   ├── seasons.ini     # Seasonal changes
│   │   └── ...
│   └── shader/             # Custom shaders (CSP)
├── ui/
│   ├── ui_track.json       # Menu layout + previews
│   ├── preview_1.jpg ...   # Selection thumbnails
│   └── badge.png           # Track icon
├── layouts/                # Layout variants
│   └── short/
│       ├── data/           # Overrides (timing, ai, pit)
│       └── ui/
└── description.ini
```

> **💡 Pro Tip:** The `layouts/` folder is one of the most underutilized features in track modding. A single high-quality track with 2–3 layouts (GP, Short, Historic) gets significantly more downloads than three separate tracks because users appreciate the consolidated install and shared textures.

---

### 2.2 Terrain, Asphalt & Geometry (3D)

The racing surface is built as a 3D ribbon following a centerline spline with width, banking, and elevation.

#### Tools
| Tool | Type | Best For |
|------|------|----------|
| **Blender + KN5 exporter** | 3D suite | Full control, custom scenery |
| **RaceTrack Builder (RTB)** | Dedicated | Automated mesh, AI, and surfaces from centerline |
| **Bob's Track Builder** | Legacy | Older but still functional |
| **Google Earth / LIDAR** | Data source | Real-world elevation and layout accuracy |

#### Steps
1. Define centerline (X, Y, Z points) — direction must match `track.ini` clockwise setting.
2. Generate asphalt mesh with proper width; add runoff, kerbs (red/white), curbs.
3. Add banking on corners and elevation changes.
4. Build **walls/barriers** — Armco, concrete, tire walls, fences. These need a collision mesh.
5. Add **scenery** — grandstands, buildings, trees, bridges, tunnels, start gantry.
6. **LODs**: create simplified distant versions.
7. **Chunking**: split large meshes so the engine culls unseen parts (improves FPS).

> **⚠️ Critical:** Direction must match `track.ini` `DIRECTION=1` (clockwise) or `-1` (anticlockwise). A mismatch causes AI to drive backwards or crash immediately.

---

### 2.3 Textures & Materials

Textures for a track are usually tiling DDS (asphalt, grass, concrete, gravel, kerb, guardrail). `materials/*.mtl` maps these to Kunos/CSP shaders:

| Shader | Use Case |
|--------|----------|
| `ksPerPixelAT` | Asphalt with detail normal |
| `ksPerPixel` | Generic objects |
| `ksGrass` / CSP `cs_grass` | Grass with wind and wet response |
| `ksDirt` / `cs_dirt` | Gravel/runoff |
| `ksGuardRail` | Barriers |
| `ksWater` | Puddles (CSP) |
| `ksEmissive` | Trackside lights, signage |

**surfaces.ini** is the bridge between visual material and physics. Each `[SURFACE]` block assigns an ID used by the 3D mesh and defines:
```ini
[SURFACE_0]
NAME=ASPHALT
FRICTION=1.0
ROLLING_RESISTANCE=0.015
BUMP=0.0005
DUST=0.0
WET_FRICTION_MULT=0.85
```

Different surfaces (grass, gravel, kerb, wall) get different friction, bumpiness, and particle effects. This file is critical to how the track "feels."

> **📌 Note:** `WET_FRICTION_MULT` is the single most important parameter for wet-weather drivability. Values below 0.75 make the track nearly undrivable in rain; values above 0.90 feel unrealistic.

---

### 2.4 Physics, AI & Gameplay Data

#### track.ini
```ini
[INFO]
NAME=My Track
LENGTH=4200
PITBOXES=24
[START]
X=0 Y=0 Z=0   ; start/finish coordinates
DIRECTION=1   ; 1 = clockwise
[GROOVE]
...
```
Plus GPS latitude/longitude (for sun position accuracy), timezone, and surface defaults.

#### AI Paths (`data/ai/`)
These `.ai` files are splines that tell the AI how to drive and define track boundaries:

| File | Purpose | Format |
|------|---------|--------|
| `fast_lane.ai` | Racing line with target speeds | `X Y Z SPEED` (m, m/s) |
| `left_lane.ai` / `right_lane.ai` | Track edges for collision/width | `X Y Z` (speed ignored) |
| `pit_lane.ai` | Pit lane path with speed limits | `X Y Z SPEED` |
| `hint_corner.ai` | Corner speed hints | `X Y Z TYPE VALUE` |
| `hint_apex.ai` | Apex hints | `X Y Z TYPE VALUE` |
| `hint_overtake.ai` | Overtaking zones | `X Y Z TYPE VALUE` |

> **⚠️ Critical:** Bad AI lines cause crashes, spinning, or AI stopping. Spend significant time here. Use the **AI Line Helper** app in-game or **CSP AI Editor** to record by driving clean laps.

#### cameras.ini
Defines trackside TV cameras, onboard cams, helicopter/drone sweeps, and the replay director logic. Coordinates reference world space; each camera has position, look-at, FOV, and timing.

#### pit.ini
Box positions, pit entry/exit speed limit, and the speed-limiter trigger zone.

#### timing.ini
Sector split lines and start/finish; speed trap locations for telemetry.

#### drs_zones.ini
Detection and activation zones for DRS (if F1-style track).

---

### 2.5 Sound (Tracks)

Tracks have ambient sound: engine echoes off barriers, crowd noise, wind, and localized speakers. Configured via FMOD banks in `sfx/` or CSP's `extension/config/ext_sound.ini` which places 3D sound emitters (grandstand speakers, tunnel reverb). Most track mods keep ambient sound minimal but CSP allows rich spatial audio.

---

### 2.6 Extension / CSP Features

The `extension/config/` folder unlocks modern features through **Custom Shaders Patch**:

| Feature | File | Description |
|---------|------|-------------|
| **ext_config.ini** | Master toggle | Enables dynamic lights, rain FX, grass FX, traffic, etc. |
| **weather_*.ini** | Weather presets | `clear`, `overcast`, `light_rain`, `heavy_rain`, `storm`. Controls sky, fog, sun intensity, wetness, puddle maps, and particle rain. |
| **track_lights.ini** | Night lighting | Animated trackside lamps, billboards, start gantry, with on/off schedules and emissive glow. |
| **seasons.ini** | Seasonal changes | Swaps textures/vegetation between spring/summer/autumn/winter. |
| **grass_fx.ini** | Grass effects | Wind-blown grass with physical interaction. |
| **dynamic_shadows.ini** | Shadows | Advanced atmospheric and rendering effects. |
| **real_sky.ini** | Sky | Realistic sky rendering. |

These are optional but expected for high-quality modern releases.

---

### 2.7 Layouts

The `layouts/` folder lets one track ship multiple configurations (e.g., GP, Short, Historic, Drift). Each layout overrides `data/` (timing, AI, pit, surfaces) and provides its own `ui/` previews, while sharing the base `3d/` and `textures/`. This is efficient and keeps file size down.

---

### 2.8 UI, Packaging & Testing

- `ui_track.json` — display name, description, country, length, pit boxes, tags, and which previews to show.
- `preview_*.jpg` — high-quality renders used in the track selector (typically 4 angles).

> **📌 Note:** Testing checklist: Hotlap (check grip, bumps, walls), Race with full AI grid (check they race cleanly), Replay (cameras), Wet + Night (CSP), VR (performance). Watch for: invisible walls, holes, AI cutting, FPS drops, broken reflections.

> **💡 Pro Tip:** Packaging: `.7z`/`.zip` the `my_track` folder with `description.ini` and credits. Note required CSP version.

---

### 2.9 Detailed INI File Reference — Track Data Files (Line-by-Line)

#### track.ini
Master track descriptor. `[HEADER] VERSION=1`. `[INFO] NAME=` display name, `LENGTH=` metres, `PITBOXES=` count, `COUNTRY=` ISO code, `TAGS=` comma-separated (e.g., "road,permanent,fia"), `DESCRIPTION=` multi-line. `[START] X= Y= Z=` start/finish line centre (world metres), `DIRECTION=1` (1=clockwise, -1=anticlockwise), `WIDTH=` track width at start line. `[GROOVE]` racing-line rubber build-up: `LATERAL_GAIN=`, `LONGITUDINAL_GAIN=`. `[DYNAMIC]` optional: `TRACK_STATUS=0` (green), `WATER_LEVEL=0`. `[POSITIONS]` `START_FINISH=0` spline metre, `SPLIT_1=`, `SPLIT_2=`, `SPEED_TRAP=`. `[PIT]` `LANE_BOUNDS=`, `SPEED_LIMIT=`, `ENTRY=`, `EXIT=` spline metres. `[WEATHER]` `SUN_AZIMUTH=`, `SUN_ALTITUDE=`, `NORTH_OFFSET=` degrees from +Z. `[TIMEZONE]` `GMT_OFFSET=` hours. `[SURFACE_DEFAULTS]` `DRY_FRICTION=1.0`, `WET_FRICTION_MULT=0.85`, `DUST=0`, `BUMP=0.0005`.

#### surfaces.ini
Maps each material ID (referenced in the 3D mesh) to physics properties. One `[SURFACE_N]` block per material:
```ini
[SURFACE_0]
NAME=ASPHALT
ID=0
FRICTION=1.0
ROLLING_RESISTANCE=0.015
BUMP=0.0005
DUST=0.0
WET_FRICTION_MULT=0.85
PARTICLE=ASPHALT
SOUND=ASPHALT
```

`ID` must match the material name in the `.mtl`/3D mesh. `PARTICLE` and `SOUND` reference particle-effect and tyre-scrub sound presets in `sys/particles.ini` and `sys/sounds.ini`. Common surfaces: ASPHALT, KERB, GRASS, GRAVEL, SAND, CONCRETE, WALL, RUMBLE_STRIP, PITLANE. CSP adds `WET_PUDDLE_DEPTH`, `AQUAPLANING_SPEED`, `TEMPERATURE_TRANSFER`.

#### ai/fast_lane.ai
Spline of `N` waypoints recorded by driving the ideal line. Each line: `X Y Z SPEED` (metres, m/s). Speed is the AI target velocity at that point. CSP AI Editor can smooth and resample. Must cover full lap continuously; first point ≈ last point.

#### ai/left_lane.ai / right_lane.ai
Track-edge boundaries. Same spline format but `SPEED` is ignored; defines the drivable corridor width. AI will not cross these. Must enclose fast_lane with margin.

#### ai/pit_lane.ai
Pit lane centreline from pit-entry to pit-exit. Includes `SPEED_LIMIT` enforcement zone markers (CSP reads `PIT_SPEED_LIMIT` from `track.ini`). Must connect to `fast_lane` at entry/exit.

#### ai/hint_*.ai
Optional hint splines: `hint_corner.ai` (corner apexes), `hint_braking.ai` (brake zones), `hint_overtake.ai` (overtake zones). Format: `X Y Z TYPE VALUE`. `TYPE` enum: 0=apex, 1=brake, 2=overtake. `VALUE` = target speed or priority.

#### cameras.ini
TV/replay camera definitions. Each `[CAMERA_N]`:
```ini
[CAMERA_0]
POS=100.5, 2.0, -45.3
TARGET=105.0, 1.5, -44.0
FOV=35
TIME=0.0
TYPE=TRACKSIDE
NAME=Turn1_Entry
```

`POS` world position, `TARGET` look-at, `FOV` degrees, `TIME` activation time (seconds from race start, or spline metre for lap-relative), `TYPE` enum: `TRACKSIDE`, `HELICOPTER`, `DRONE`, `ONBOARD`, `BONNET`, `BUMPER`. `NAME` for director script. CSP adds `EXPOSURE`, `MOTION_BLUR`, `POST_PROCESS` profile.

#### pit.ini
Pit lane configuration:
```ini
[PIT]
LANE_BOUNDS=100.0, 500.0
SPEED_LIMIT=80
ENTRY=120.0
EXIT=480.0
BOX_SPACING=12.0
FIRST_BOX=125.0
```

`LANE_BOUNDS` spline metre range where speed limiter active. `BOX_SPACING` and `FIRST_BOX` place garages along lane.

#### timing.ini
Sector and trap definitions:
```ini
[TIMING]
SPLITS=3
SPLIT_1=1450.0
SPLIT_2=2900.0
FINISH=4200.0
SPEED_TRAP=3800.0
```

`SPLITS` count, `SPLIT_N` spline metre of sector line, `FINISH` start/finish line, `SPEED_TRAP` spline metre for top-speed trap.

#### drs_zones.ini
F1-style DRS:
```ini
[DRS_0]
DETECTION=1200.0
ACTIVATION_START=1400.0
ACTIVATION_END=2100.0
```

`DETECTION` spline metre where gap measured, `ACTIVATION_START/END` range where DRS may open if within 1s.

#### ext_config.ini
CSP master config for track:
```ini
[SHADER_REPLACEMENT]
ACTIVE=1
[RAIN_FX]
ACTIVE=1
PUDDLES=1
[GRASS_FX]
ACTIVE=1
[TRAFFIC]
ACTIVE=0
[DYNAMIC_LIGHTS]
ACTIVE=1
[SEASONS]
ACTIVE=1
```

Each feature block toggles CSP systems. `[SHADER_REPLACEMENT]` enables custom material overrides. `[RAIN_FX]` enables puddle maps, spray, wet reflections. `[GRASS_FX]` enables wind-blown grass with physical interaction. `[DYNAMIC_LIGHTS]` enables track-light schedules. `[SEASONS]` enables texture swaps.

#### weather_*.ini
Per-weather preset (e.g., `weather_clear.ini`, `weather_overcast.ini`, `weather_light_rain.ini`, `weather_heavy_rain.ini`, `weather_storm.ini`):
```ini
[SKY]
TEXTURE=sky_clear.dds
SUN_COLOR=1.0,0.95,0.85
SUN_INTENSITY=1.2
AMBIENT=0.3,0.35,0.4
[FOG]
DENSITY=0.0005
COLOR=0.8,0.85,0.9
[LIGHT]
DIRECTION=-0.3,-0.7,-0.6
[SURFACE]
WETNESS=0.0
PUDDLE_MAP=puddles_dry.dds
[PARTICLES]
RAIN=0
```

`WETNESS` 0–1 drives puddle opacity and tyre wet friction. `PUDDLE_MAP` greyscale texture (white=deep puddle). `RAIN` enables particle rain emitter with density.

#### track_lights.ini
Night lighting schedules for emissive objects:
```ini
[LIGHT_0]
NAME=Grandstand_Floods
OBJECT=GEO_Grandstand_Lights
TYPE=FLOOD
COLOR=1.0,0.95,0.85
INTENSITY=5000
SCHEDULE=18:00-06:00
```

`OBJECT` = 3D node name, `TYPE` enum `FLOOD`/`SPOT`/`OMNI`/`EMITTER`, `SCHEDULE` 24h on/off times (or `ALWAYS`/`SUNSET_SUNRISE`). CSP interpolates intensity at dawn/dusk.

#### seasons.ini
Texture/vegetation swaps per season:
```ini
[SEASONS]
ACTIVE=1
[SPRING]
TEXTURE_OVERRIDE=textures_spring
GRASS_TINT=0.6,0.9,0.4
TREE_LEAVES=leaves_spring.dds
[SUMMER]
TEXTURE_OVERRIDE=textures_summer
GRASS_TINT=0.4,0.7,0.3
TREE_LEAVES=leaves_summer.dds
[AUTUMN]
TEXTURE_OVERRIDE=textures_autumn
GRASS_TINT=0.8,0.5,0.2
TREE_LEAVES=leaves_autumn.dds
[WINTER]
TEXTURE_OVERRIDE=textures_winter
GRASS_TINT=0.5,0.5,0.5
TREE_LEAVES=leaves_winter.dds
SNOW_COVER=1
```

`TEXTURE_OVERRIDE` subfolder under `textures/` with season-specific albedo/normal. `GRASS_TINT` multiplies `cs_grass` shader. `SNOW_COVER=1` enables snow shader on horizontal surfaces.

#### ui_track.json
Menu metadata:
```json
{
  "name": "My Track",
  "country": "IT",
  "length": 4200,
  "pitBoxes": 24,
  "tags": ["road","permanent","fia"],
  "previews": ["preview_1.jpg","preview_2.jpg","preview_3.jpg","preview_4.jpg"],
  "layouts": [
    {"name": "GP", "data": "data", "ui": "ui"},
    {"name": "Short", "data": "layouts/short/data", "ui": "layouts/short/ui"}
  ],
  "cspVersion": "0.1.78"
}
```

`layouts` array lets CM show layout selector with per-layout data/ui folders. `cspVersion` minimum required.

#### layout data/ overrides
Each layout's `data/` can contain any subset of `track.ini`, `surfaces.ini`, `ai/*.ai`, `pit.ini`, `timing.ini`, `drs_zones.ini`, `cameras.ini`. Missing files fall back to base track. Typical: different `timing.ini` (shorter sectors), `ai/fast_lane.ai` (different line), `pit.ini` (different pit entry).

#### description.ini
Free-form multi-line description shown in CM:
```ini
[DESCRIPTION]
My Track is a fictional 4.2 km road course...
Built for Assetto Corsa with CSP 0.1.78+.
```

---

## Essential Tools Summary

| Purpose | Tools |
|---------|-------|
| 3D Modeling | Blender (free, KN5 scripts), 3ds Max, Maya |
| KN5 Import/Export | Kunos exporter, `io_import_ksp_cars`, Blender KN5 add-on |
| Textures / DDS | Photoshop, GIMP, Paint.NET, NVIDIA Texture Tools, Substance Painter |
| Car Physics | Car Tuner app, CM Physics Checker, spreadsheets for curves |
| Track Building | RaceTrack Builder (RTB), Blender, Bob's Track Builder, Google Earth/LIDAR |
| AI Lines | In-game AI Line Helper, CSP AI Editor |
| Audio | FMOD Studio, Audacity, Reaper |
| Management / Test | Content Manager (CM), Custom Shaders Patch (CSP) |
| Packaging | 7-Zip, CM auto-pack |

---

## Recommended Workflow

### Car Workflow
```
[1] Collect real specs (mass, power, dimensions, suspension type)
    ↓
[2] Model high-poly → retopo → UV → bake
    ↓
[3] Export KN5 with correct node names and LODs
    ↓
[4] Write .ini physics starting from a similar Kunos car
    ↓
[5] Create default skin + template
    ↓
[6] Build FMOD sound bank
    ↓
[7] Add driver/cockpit, UI, badges
    ↓
[8] Iterate: showroom → hotlap → AI race → night/rain → MP
    ↓
[9] Package with credits and README
```

### Track Workflow
```
[1] Research layout (map, GPS, onboard video, LIDAR)
    ↓
[2] Build centerline spline with elevation/banking
    ↓
[3] Generate asphalt, kerbs, walls, scenery; assign LODs/chunks
    ↓
[4] Create materials + surfaces.ini
    ↓
[5] Record AI fast_lane + hints + pit lane
    ↓
[6] Place cameras, pit, timing, DRS
    ↓
[7] Add CSP weather, night lights, seasons
    ↓
[8] Derive layout variants
    ↓
[9] Test all conditions + full AI grid + MP
    ↓
[10] Package with previews and CSP version note
```

---

## Community Resources

- **Assetto Corsa Official Forum** — modding subforum, tutorials, feedback.
- **RaceDepartment** — downloads, WIP threads, tutorials.
- **Custom Shaders Patch Discord / Patreon** — CSP docs, shader help.
- **AC Modding Discord** — active help for cars and tracks.
- **GitHub** — `ac-tools`, `ContentManager`, `CustomShaderPatch` source and issue trackers.
- **YouTube** — channels like The Simpit, Empty Box, and various modders publish step-by-step videos.

---

## Final Notes

Modding Assetto Corsa rewards patience and iteration. Start small: repaint a skin, then tweak an existing car's physics, then build from scratch. For tracks, begin with a simple oval or short circuit before attempting a full GP layout. Study other mods' files — they are the best teachers. Always respect licensing: do not port assets from other games or commercial titles without permission. Share your work, ask for feedback, and enjoy the process of bringing new cars and circuits to life.

---

# Appendices

---

## Appendix A: File Extension Quick Reference

| Extension | Full Name | Category | Description |
|-----------|-----------|----------|-------------|
| `.ini` | Initialization | Config | Plain-text key-value configuration files |
| `.kn5` | Kunos 5 | 3D | Proprietary 3D mesh format (cars, tracks, objects) |
| `.ksanim` | Kunos Animation | 3D | Keyframe animation for nodes (steering, pedals, shifter) |
| `.dds` | DirectDraw Surface | Texture | Compressed texture with mipmaps (DXT1/BC1, DXT5/BC3) |
| `.png` | Portable Network Graphics | Texture | Lossless image (UI, previews, source art) |
| `.lut` | Lookup Table | Physics | 1D/2D data tables for curves and maps |
| `.2Dlut` / `.2dlut` | 2D Lookup Table | Physics | Two-dimensional parameter maps |
| `.rto` | Ratio | Physics | Gear ratio definition file |
| `.bank` | FMOD Bank | Audio | Compiled FMOD Studio sound bank |
| `.lua` | Lua Script | Logic | CSP scripting for physics, display, lights |
| `.json` | JavaScript Object Notation | UI | Metadata for car/track selectors |
| `.mtl` | Material | 3D | Shader and texture assignment definitions |
| `.ai` | AI Spline | AI | Path spline for AI driving behavior |
| `.psd` | Photoshop Document | Texture | Layered source file for skin templates |
| `.zip` / `.7z` | Archive | Packaging | Distribution compression formats |
| `.ksanim` | Kunos Animation | Driver | Steering wheel, pedal, and shifter keyframe animations |
| `driver.ini` | Driver Config | Driver | Visibility, placement, and skin overrides |
| `driver3d.ini` | Driver 3D Config | Driver | Seat position, head movement, animation linking, LODs |
| `dashboard.ini` | Dashboard Config | Driver | Analog gauge and digital display layout |
| `digital_instruments.ini` | Digital Instruments | Driver | CSP steering-wheel and dash LED/display definitions |
| `mirrors.ini` | Mirror Config | Driver | Virtual mirror placement and node mapping |

---

## Appendix B: Glossary of AC Modding Terms

| Term | Definition |
|------|------------|
| **AO** | Ambient Occlusion — baked contact shadows |
| **ARB** | Anti-Roll Bar — stabilizer bar connecting left/right suspension |
| **BC / DXT** | Block compression formats for DDS textures |
| **CSP** | Custom Shaders Patch — community graphics/physics extension |
| **CM** | Content Manager — advanced launcher and mod manager |
| **CoG** | Center of Gravity — mass center point |
| **FMOD** | Firelight Technologies audio middleware |
| **FFB** | Force Feedback — steering wheel resistance |
| **KN5** | Kunos 3D format (proprietary) |
| **LOD** | Level of Detail — simplified distant meshes |
| **LUT** | Lookup Table — data curve file |
| **Pacejka** | Magic Formula tyre model (B, C, D, E coefficients) |
| **Retopo** | Retopology — rebuilding mesh for game-ready topology |
| **UV** | Texture coordinate mapping (U, V axes) |
| **VBO** | Vertex Buffer Object (mesh data) |
| **HANS** | Head and Neck Support — safety device preventing basilar skull fracture |
| **LOD** (Driver) | Driver-specific Level of Detail: close-up helmet/suit vs distant simplified mesh |
| **Visor Tint** | Helmet visor opacity controlled by shader or CSP time-of-day |
| **Skin Override** | Ability for skinners to replace driver textures via `skin.ini` without base-mod edits |
| **Shift Light** | LED on steering wheel indicating optimal upshift RPM |
| **Cockpit LOD** | Interior mesh detail level; high for player car, low for AI/opponents |
| **Planar Mirror** | CSP real-time reflection using mirror plane geometry |
| **Head Movement** | Physics-driven G-force head bob/roll/pitch in `driver3d.ini` |
| **Helmet Cam** | First-person camera positioned at driver's eye level (`DRIVEREYES`) |

---

## Appendix C: Troubleshooting Matrix

### Car Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Car invisible in showroom | Missing `car.kn5` or wrong folder name | Verify file path: `content/cars/<name>/3d/car.kn5` |
| Driver head clips through helmet | Wrong `HEAD_MOVEMENT` max angles or helmet mesh too small | Increase helmet interior clearance or reduce `ROLL_MAX_DEG`/`PITCH_MAX_DEG` in `driver3d.ini` |
| Hands float off steering wheel | `steer.ksanim` pivot misaligned or `STEER_LOCK` mismatch | Re-export animation with wheel at geometric center; verify `LOCK` in `driver3d.ini` matches `STEER_LOCK` in `car.ini` |
| Driver visible in cockpit view (should be hidden) | Missing or incorrect `[HIDE_OBJECT_*]` in `driver3d.ini` | Add `NAME=DRIVER:HELMET` and `NAME=DRIVER:GEO_Driver_FACE` to `[HIDE_OBJECT_*]` blocks |
| Helmet visor is opaque / no reflection | Wrong shader or missing reflection map | Use `ksGlass` with `REFLECTION=1` or `ksCarPaint` with clearcoat; verify `cube.dds` environment map exists |
| Shift lights don't blink | `LED_N` RPM switch mismatched to `engine.ini` `LIMITER` | Set `BLINK_SWITCH` to `LIMITER - 200` RPM; verify `OBJECT_NAME` matches 3D node exactly |
| Dashboard too dark at night | Missing `lights.ini` interior emissive or `ksEmissive` intensity too low | Add `[LIGHT_N]` blocks for dashboard/backlight; increase `INTENSITY` on emissive materials to 0.5–1.0 |
| Dashboard too bright in daylight | `ksEmissive` intensity too high or no auto-exposure | Reduce `INTENSITY` to 0.2–0.4; use `DASHBOARD_EMISSIVE` multiplier in `ext_config.ini` if available |
| Mirrors show black / no reflection | Missing `mirrors.ini` or wrong node names | Verify `MIRROR_0/1/2` `NAME=` matches 3D nodes; enable `[DYNAMIC_LIGHTS]` in `ext_config.ini` for CSP planar mirrors |
| Seat/harness casts no shadow | Interior mesh not in shadow caster list or alpha planes | Convert harness straps to geometry; check `ksPerPixel` shader has `CAST_SHADOWS=1` |
| Pedals don't move | Missing `*.ksanim` files or wrong node names | Name nodes exactly `PEDAL_THROTTLE`, `PEDAL_BRAKE`, `PEDAL_CLUTCH`; link animations in `driver3d.ini` |
| Gloves clip through wheel rim | Wheel radius too small or hand rig offset wrong | Increase wheel rim diameter in 3D model; adjust hand IK offset in animation export |
| Driver suit texture blurry | Low-resolution DDS or wrong mipmapping | Use 2048×2048 minimum DXT5 for suit; regenerate mipmaps with NVIDIA Texture Tools |
| HANS device clips neck | HANS geometry too large or driver head position wrong | Scale HANS down 5–10%; verify `POSITION` in `driver3d.ini` `[MODEL]` aligns with neck pivot |
| Skin not appearing | Missing `skin.ini` or wrong texture name | Check `skin.ini` `[SKIN]` block and texture filenames |
| No engine sound | Missing `sfx.bank` or FMOD version mismatch | Rebuild bank with correct FMOD version; verify `GUIDs.txt` present |
| AI crashes immediately | Bad `ai.ini` or physics NaNs | Run CM Physics Checker; verify `ai.ini` gear ratios match `gearbox.ini` |
| FFB too weak/strong | `ff_effects.ini` or `car.ini` FFMULT | Adjust `FFMULT` in `car.ini`; tune `ff_effects.ini` |
| Car floats / sinks | Wrong ride height or collision mesh | Check `suspension.ini` ride height; verify collider in `colliders.ini` |
| Tyres overheat instantly | `tyres.ini` thermal values too aggressive | Reduce `FRICTION_K` or increase `COOL_FACTOR` |

### Track Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Track invisible | Missing `track.kn5` or wrong path | Verify `content/tracks/<name>/3d/track.kn5` |
| AI drives off track | Bad `fast_lane.ai` or missing boundaries | Re-record AI line; verify `left_lane.ai` / `right_lane.ai` enclose track |
| No pit lane speed limit | Missing `pit.ini` or wrong spline metres | Check `PIT_SPEED_LIMIT` in `track.ini` and `pit.ini` bounds |
| Wet weather looks wrong | Missing `weather_*.ini` or bad puddle map | Verify `extension/config/weather_*.ini` and `PUDDLE_MAP` texture |
| Night lights don't work | Missing `track_lights.ini` or `ext_config.ini` | Enable `[DYNAMIC_LIGHTS] ACTIVE=1` in `ext_config.ini` |
| FPS drops in corners | No LODs or oversized mesh chunks | Create LOD meshes; split `track.kn5` into smaller chunks |
| Invisible walls | Missing collision mesh or wrong `surfaces.ini` ID | Verify material IDs in `surfaces.ini` match 3D mesh material names |

---

## Appendix D: Release Checklists

### Car Release Checklist
- [ ] All LODs present and switching correctly
- [ ] Collision mesh functional (no floating, no sinking)
- [ ] Default skin + at least one alternate skin with preview
- [ ] `ui_car.json` filled with correct specs and curves
- [ ] `badge.png` and `brand.png` present (256×256 transparent)
- [ ] FMOD bank loads without errors in CM Sound tab
- [ ] Physics validated: no NaNs, no AI crashes, FFB smooth
- [ ] All cameras positioned correctly (helmet, cockpit, bonnet, bumper)
- [ ] Mirrors functional (CSP virtual or `ksMirror` shader)
- [ ] Lights work in day and night conditions
- [ ] **Driver / Cockpit:**
  - [ ] Driver model visible in external views; helmet + suit + gloves present
  - [ ] Driver hidden correctly in cockpit view (helmet/face hidden, arms/gloves visible)
  - [ ] Hands grip steering wheel at full lock without clipping or floating
  - [ ] Shifting animation triggers within 100 ms of gear change; correct gate for H-pattern
  - [ ] Pedals travel full physical range; heel-toe plausible for manual cars
  - [ ] Head movement filtered naturally (`FILTER=0.10–0.25`); no VR nausea
  - [ ] Helmet visor reflects environment; tint appropriate for day/night
  - [ ] Default helmet/suit/gloves thematically match default skin
  - [ ] Steering wheel buttons/LEDs functional; shift lights blink at correct RPM
  - [ ] Dashboard readable in day and night; backlight/emissive tuned
  - [ ] Interior LODs switch correctly; AI cars use low-detail cockpit
  - [ ] Seat and harness cast shadows; straps are geometry not alpha planes
  - [ ] Cockpit camera position realistic (eye height ~1.05–1.15 m)
  - [ ] Driver skin override works (skin folder can replace helmet/suit/gloves)
- [ ] Setup menu (`setup.ini`) readable and functional in CM
- [ ] Tested in multiplayer with at least 2 clients
- [ ] `description.ini` and README included with credits
- [ ] Folder cleaned of WIP/source files before packaging

### Track Release Checklist
- [ ] AI fast lane covers full lap, first point ≈ last point
- [ ] Left/right boundaries enclose track with safe margin
- [ ] Pit lane connects cleanly to fast lane at entry/exit
- [ ] All timing sectors and speed traps placed correctly
- [ ] `surfaces.ini` IDs match 3D mesh material names exactly
- [ ] LODs created for main track mesh and key objects
- [ ] Trackside cameras placed with good coverage
- [ ] Weather presets configured (at least clear and rain)
- [ ] Night lighting functional if track supports night racing
- [ ] Layout variants tested independently
- [ ] Previews rendered at 1920×1080 or higher
- [ ] `ui_track.json` includes correct country, length, pit boxes
- [ ] No invisible walls or holes in collision mesh
- [ ] Tested with full AI grid (20+ cars) for FPS stability
- [ ] VR-tested if claiming VR compatibility
- [ ] `description.ini` and README with CSP version requirement

---

*Document enhanced for readability, cross-referencing, and practical workflow support. All technical specifications preserved from original source material.*
