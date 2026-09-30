# KSEngine / ksimulator Qt-free

**Updated:** 2026-09-29 — **fase conclusa: `check_no_qt.ps1 -Strict` → 0.**
Milestone concluse: **ECS + `ks::Engine` usato dal simulatore**, **renderer
deferred/volumetrici/TAA**, **scripting Lua**.

## Status

| Gate | Result |
| --- | --- |
| `ksengine` (`-DKSENGINE_QT_FREE=ON`) | **builds, 0 errors** |
| `SimulatorApp` (`-DKSIMULATOR_QT_FREE=ON`) | **builds + links, 0 errors** |
| `tools/check_no_qt.ps1 -Strict` | **0 of 801 files touch Qt** (exit 0) |
| ECS standalone build (`cmake -S src/engine`) | **builds, 0 errors** |
| CTest qt-free (`ctest --test-dir build_qtfree -C Debug`) | **7/7 passed** |
| CTest standalone (`cmake -S tests/qtfree`) | **7/7 passed** |

## Build

```bash
cmake -S . -B build_qtfree -G "Visual Studio 18 2026" -A x64 \
      -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON
cmake --build build_qtfree --config Debug            # everything
cmake --build build_qtfree --config Debug --target SimulatorApp -j 8
powershell -ExecutionPolicy Bypass -File tools/check_no_qt.ps1 -Strict
```

Notes:
- Re-run the `cmake -S . -B build_qtfree` configure step after adding or
  deleting sources: the `file(GLOB_RECURSE)` results are cached.
- The qt-free early-exit block lives in the root `CMakeLists.txt`
  (≈ line 176). Qt `find_package` calls are skipped behind `KS_QT_FREE_BUILD`.

## Engine

- `Engine.h` + `Engine.cpp` + `EngineModule.h` — fixed tick, `update(dt)`, no Qt.
  `Engine.cpp` now holds the real out-of-line bodies (`initialize`, `shutdown`,
  `createEntity`, `destroyEntity`) so a standalone `ksengine` build compiles and
  instantiates the whole ECS, not just parses the headers.
- `Engine` owns a `ecs::Registry` (`registry()`) and exposes
  `createEntity(name)` / `destroyEntity(e)` — the app-side equivalent of
  `pEntitySystem->SpawnEntity()`.
- `scene/Registry.h` — sparse-set ECS, dependency-free. Handles are
  `(generation << 20 | index)` so a stale handle can never alias a recycled
  slot. `emplace/remove/has/tryGet/get` are O(1); `each<A, B>()` iterates the
  smallest pool and is available in const and non-const form (up to 4
  component types).
- `scene/Components.h` — `Name`, `Tag`, `Transform` (position/rotation/scale,
  rotation as roll-pitch-yaw matching `ks::physics::MotionState`), plus
  renderer-agnostic `MeshInstance` and `worldMatrix(Transform)`.
- `scene/SceneModule.h` — `EngineModule` (`ks.scene`) owning an ordered list
  of named systems; `Engine::tick()` runs them against `Engine::registry()`
  at the fixed timestep.
- `KsQtFreeGuard.h` — hard fail if Qt headers leak in
- CMake AUTOMOC/UIC/RCC off, no Qt link
- Qt-only sources are excluded in `src/engine/CMakeLists.txt` via
  `list(FILTER ... EXCLUDE REGEX ...)` as a safety net (the Qt/QML bridges no
  longer live under `src/engine` at all).
- `engine/AI/AiFileReader` — standalone AC AI-line reader (binary `\0AI`
  magic + CSV fallback), replaces the Qt editor's `AiSplineEditor::parseAiBinary`.
- `engine/Audio/BankParserBridge` — `ks::audio::parseBankFile` stub, reports
  `valid=false` until a standalone FMOD bank reader exists.

## ks::Engine wiring (SimulatorApp)

- `SimulationLoop::initialize()` → `Engine::initialize()`, registers
  `ks.input` (`devices::InputSystem`), `ks.render` (`graphics::RenderSystem`),
  `ks.scene` (`ecs::SceneModule`) and `ks.script` (`scripting::ScriptModule`)
  — **the first three were dead code before** (nothing in the repo ever called
  their `::instance()`).
- The three modules are Meyers singletons, so they are registered through an
  aliasing `shared_ptr` with a no-op deleter: `Engine` must not own/destroy
  them.
- `SimulationLoop::start()/stop()` drive `Engine::start()/stop()`;
  the destructor clears the systems, detaches the registry and shuts the
  engine down.
- `SimulationLoop::tick()` calls `Engine::tick(elapsed)` after its own 1 kHz
  physics pass, so ECS systems run at the engine's fixed 120 Hz step.
- `syncCarTransforms` is registered as a system: vehicle position →
  `Transform` of every `car_*` `MeshInstance` (replaces the old inline loop
  over `m_renderables`).
- `applyInput()` publishes `InputState` into `devices::InputSystem`, so engine
  code reads input from the module instead of reaching into the sim.
- `updateWeather()` is no longer a stub: time-of-day → sun direction
  (12:00 reproduces the historical `{0.3,-0.8,0.2}` default exactly, so the
  frame is unchanged until `setTimeOfDay()` is called), cloud cover → fog,
  `WeatherState` → rain/wetness, forwarded to `NativeRenderer::setSun`.
- `render()` runs the `RenderSystem` frame lifecycle (Shadow → Geometry →
  Post) and submits the scene **from the registry**: every
  `(Transform, MeshInstance)` entity becomes a `NativeRenderer::drawMesh`.
- `loadBakedScene(dir)` loads the `.nmsh` manifest through `NativeRenderer`
  and spawns one entity per mesh; `SimulatorApp` now calls this instead of
  reaching into the renderer directly.
- Dead field removed: `SimulationLoop::m_renderables` / `renderables()`
  (`std::vector<RenderableMesh>`, always empty, zero references anywhere in
  `src/`, `examples/`, `tests/` or `tools/`).

## Simulator

- `SimulationLoop` wires **NativeUiHub** each frame
- HUD from vehicle state; modal UI blocks driving input + FFB
- `handleUiKey(vk)` — Esc menu / F1 devices / F2 multiplayer
- Font atlas + UiRenderer batch ready for GPU upload
- `NativeRenderer` + `ShadowSystem` are the full Vulkan implementations
  (the 124-byte include shims left over from the reset were removed).
- `MqttClient.cpp` is **not** in the qt-free source list: it unconditionally
  includes `<mosquitto.h>`, which is not on this machine.

## Renderer — deferred, volumetrici, TAA

- Three passes: **GBuffer** (MRT: RT0 albedo+AO `R8G8B8A8_UNORM`, RT1
  normal+roughness `R16G16B16A16_SFLOAT`, RT2 worldPos+coverage) →
  **lighting** (fullscreen triangle, PBR GGX + cascade shadow + height fog +
  12-tap volumetric raymarch with a Henyey-Greenstein phase = god rays) →
  **resolve** (TAA or plain blit into the swapchain).
- `NativeRenderer::setDeferred(bool)` / `setTaa(bool)`, **both default off**:
  the TAA path is opt-in because this environment has no visual verification,
  so the gate is "builds + `glslc` compiles every shader", not "looks right".
- `taa.frag` reprojects through the *unjittered* view-projection, clamps
  against the 3×3 neighbourhood of the current frame and blends with the
  history at `taaParams.x` (0.9). Halton(2,3) jitter over 8 samples is applied
  to `proj(0,2)/(1,2)`. Two history images ping-pong; `m_historyValid` forces
  feedback 0 on the first frame after a swapchain rebuild.
- `FrameDataUBO` grew to 400 bytes (`viewProj`, `prevViewProj`,
  `taaParams` appended after `cameraPos`) — still layout-compatible with the
  shorter block declared by `native_forward.frag`.
- New shaders in `src/simulator/shaders`: `gbuffer.{vert,frag}`,
  `deferred_lighting.{vert,frag}`, `taa.frag`; all five verified with `glslc`
  and registered in `_ksim_shader_sources`.

## Scripting — Lua 5.4.8

- Vendored from `lua.org` into `src/engine/external/lua` (60 files, `lua.c` /
  `luac.c` excluded), built as its **own static library `ksengine_lua`**.
  Separate target on purpose: the engine puts `src/engine/sys` on the include
  path, and on a case-insensitive filesystem `lstate.h`'s `#include
  <signal.h>` resolves to the engine's C++ `Signal.h` instead of the CRT's.
- The vendored copy **wins over** `find_package(Lua)`, and the `HAS_LUA`
  variable is shared between the root `CMakeLists.txt` and
  `src/engine/CMakeLists.txt`, so every TU sees the same value.
- `Scripting/LuaScriptHost.{h,cpp}` — owns the `lua_State`.
  `initialize/shutdown/eval/runFile/getNumber/setNumber/hasFunction/
  callFunction/lastError`. `eval()` first tries `return <code>` (so
  expressions work) and falls back to the verbatim chunk (so statements
  work); failures never throw, they record `lastError()` and return false.
  Lua's headers carry no `extern "C"` guard, so the include is wrapped.
- `Scripting/ScriptHost.{h,cpp}` — facade; the only scripting type the rest
  of the engine talks to.
- `Scripting/ScriptModule.h` — `EngineModule` with `moduleId "ks.script"`,
  `priority() 100`: runs the queued startup files at `initialize()`, then
  publishes `delta_time` and calls a global `on_update(dt)` each fixed tick.
- Wired in `SimulationLoop::initialize()` as `ks.script`; a missing Lua is
  reported on stderr instead of failing startup.
- `Engine::tick()` now `stable_sort`s the module snapshot by `priority()`.
  `priority()` was dead before (nothing overrode it), so existing module
  ordering is unchanged — it just makes "scripting runs last" real.
- Tests: `tests/qtfree/lua_test.cpp`, `tests/qtfree/script_module_test.cpp`.

## Qt removed from `src/engine` + `src/simulator`

- **Moved** (editor-only, still needed by `src/main.cpp` and
  `src/sdk/kseditor/main.cpp`) to `src/sdk/kseditor/qmlbridges/`:
  `assets/AssetsLibraryQmlBridge`, `Audio/{AudioQMLBridge,
  AudioWaveformBridge, NoiseReducer, PeakMeter, Studio}`,
  `mesh/MeshLoaderQML`, `Scripting/{ACEContentQMLBridge, ContentQMLBridge,
  CspConfigQmlBridge}` — includes updated in `src/main.cpp`,
  `src/sdk/kseditor/main.cpp`, `SoundEditorModule.cpp`.
- **Deleted** (tracked, zero references, superseded by the native overlays):
  `src/simulator/DeviceSettingsWidget.{h,cpp}`,
  `src/simulator/MultiplayerWidget.{h,cpp}`.
