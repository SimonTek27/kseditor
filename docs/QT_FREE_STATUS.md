# KSEngine / SimulatorApp Qt-free

**Updated:** 2026-09-28

## Runtime
| Component | Status |
|-----------|--------|
| Engine | ✅ |
| SimulatorApp | ✅ NativeRenderer |
| SimulationLoop | ✅ `NativeRenderer*` (no Graphics VulkanRenderer) |
| InputManager DI/XInput | ✅ |
| Vehicle + FFB | ✅ |

## Build ksimulator (no Qt)
```bash
cmake -DKSIMULATOR_QT_FREE=ON -DKSENGINE_QT_FREE=ON ..
# include(cmake/CMakeLists_ksimulator_QtFree.cmake) from root CMakeLists
cmake --build . --target ksimulator
```

## Still Qt (editor)
Graphics/*, DeviceSettingsWidget, MultiplayerWidget, SimulatorServerApp, PostProcessing
