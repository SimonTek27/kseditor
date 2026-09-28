# KSEngine Qt-free status

**Updated:** 2026-09-28 (scan + full stub pass)

## Completed this pass
- Tools: all 27 systems stubbed
- mesh: remaining sculpt/UV/physics mesh stubs
- assets: dependency, watcher, project, preview (no QWidget)
- sys: DB, plugin, module, hang, serializer, state machine
- Scripting: Blueprint, Lua, Python hosts
- animation: timeline, physics anim, shape keys

## Runtime facades (usable API)
Engine, physics, devices, Graphics/RenderSystem, AudioCore/DrivingAudio,
AssetManager, LogManager, SettingsManager, TaskSystem, NavMesh

## CMake
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```

## Note
Editor Qt implementations recoverable from git history before stub commits.
FileFormat/Config/network/material may still need a residual Qt grep if build fails.
