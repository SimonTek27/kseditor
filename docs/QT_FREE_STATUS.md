# KSEngine status — stubs vs real

**Updated:** 2026-09-28

## Real (not stubs)
| Module | Notes |
|--------|--------|
| `Engine.h` | fixed timestep, modules, registry |
| `physics/VehicleSimulator` | Pacejka + aero + gears + INI load |
| `devices/DeviceManager` | monitors, racing input, VR hooks |
| `Math/MathCore` | Vec/Mat/Quat std |
| `FileFormat/INIParser` | real parse |
| `Config/ConfigLoader` | **real INI load/save** (resolved) |
| `sim/NativeRenderer` | **mesh upload + frame API** (resolved) |
| `Audio/AudioCore` | **rpm/throttle level model** (resolved) |
| `network/NetworkManager` | **session state + callbacks** (resolved) |
| `Graphics/VulkanRenderer` | **handle holder** (resolved) |

## Still thin facades (editor/optional)
Tools, Scripting Blueprint, mesh sculpt, FileFormat CAD/Bank writers, SevenZip,
SSGI, TextureTools (no QImage path yet)

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```
