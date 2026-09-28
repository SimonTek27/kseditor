# KSEngine Qt-free status

**Updated:** 2026-09-28

## Latest
- FileFormat: USDParser, FormatValidator, BankParser/WriterRegistry, BankVersion, CADAdvancedParsers
- devices/vr/XrConfig — no QSettings
- Scripting: HotReload, Coroutine, ScriptDebuggerFrontend
- mesh: MeshData.cpp, SculptLayersManager
- archive: SevenZipLibrary stub

## Core already free
MathCore, Engine, physics runtime, devices input/FFB, Graphics facades (Vulkan*, TextureTools, RenderSystem)

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```
