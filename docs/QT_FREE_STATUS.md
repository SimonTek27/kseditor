# KSEngine Qt-free status

**Updated:** 2026-09-27

## Policy
No full build of SimulatorApp/ksengine until every **linked** translation unit is Qt-free.
`KSENGINE_QT_FREE=1` is defined on the `ksengine` target.

## Runtime stack — DONE (Qt-free)

### Engine / devices / XR
| Area | Files |
|------|--------|
| Core loop | Engine.h, EngineModule.h |
| Devices | DeviceManager, SimRacingDevices (TripleMonitor) |
| XR | XrManager, XrIntegration, XrInput, XrViewportRenderer |
| App | SimulatorApp.cpp, NativeRenderer.h (Vulkan, no Qt) |
| UI | UiRenderer, GameMenuOverlay (batched, dirty-flag) |
| GPU | GpuProfiler (Vulkan timestamps) |

### Physics (integrated in `phys_Simulator`)
| Module | Role |
|--------|------|
| PhysicsCoreTypes / PhysicsTypes | PhysVec3, SimulationState, enums |
| PhysicsEngine | rigid bodies / world |
| AeroModel + AeroSimulator + AeroDraft | wings, DF, draft |
| PacejkaTireModel + TireSimulator + TireFlatSpot | tire forces |
| EngineModel + GearboxModel + DifferentialModel | powertrain |
| HybridSystem | ERS |
| WeatherPhysics | rain / density / wind |
| TrackSurface | spatial grip / rubber |
| SuspensionModel | loads + ride height |
| BrakeThermalModel | disc fade |
| ChassisSimulator | yaw / sideslip |
| DamageSystem | collision multipliers |
| PhysicsLogger / PhysicsProfiler | stdio + frame timing |
| **phys_Simulator** | facade + full tick |

### Tick order
1. Weather → TrackSurface.sync
2. Engine → auto gearbox
3. Diff + Hybrid
4. Aero (draft/DRS/damage)
5. Suspension → normal loads
6. Brake thermal
7. Tires (Pacejka × track grip)
8. Longitudinal step
9. Chassis yaw
10. Rubber deposit / callbacks / profile

## CMake
- Roots: Math, physics, devices, FileFormat, archive, Config, network, material, terrain, hwril
- **Excluded:** VehiclePhysics, TireCurveEditor, TrackPhysics, ChassisSimulator (old), HybridSystem (old Qt), `physics/weather/*` editors, `phys_*` legacy snapshots
- Allowlist helper: `CMakeLists_physics_QtFree.cmake`

## Still Qt (editor / excluded from ksengine)
Audio, Graphics (QVulkanWindow path), mesh editors, Scripting, animation, sys (LogManager Qt), VehiclePhysics monolith, TireCurveEditor UI, MultiplayerWidget, DeviceSettingsWidget

## Gate
Runtime scan of linked modules: no QObject/QString/QVector/Q_OBJECT in code.
When the local tree matches these ports and CMake filters apply, a real compile of SimulatorApp + ksengine is allowed under the Qt-free policy.
