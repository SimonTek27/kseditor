# KSEngine Qt-free status

**Updated:** 2026-09-28

## Done this session
- `Math/MathCore.h` — no QVector/QMatrix
- `Graphics/VulkanFunctions.h` — LoadLibrary/dlopen
- `Graphics/VulkanShaderLoader` — SPIR-V via ifstream
- `Graphics/VulkanIntegration` — no QObject
- `Graphics/VulkanComputePipeline` — std facade
- `Graphics/VideoModesWrapper` — no QObject
- `Graphics/TextureTools` — no QImage (AC tools → adapters later)

## CMake excludes
physics monoliths, 3dprint, scanners, Streamline, SevenZip, AC adapters, VideoModesFunctions

## Next residual candidates
- `archive/SevenZipLibrary.*` (excluded)
- FileFormat: USDParser, FormatValidator, Bank*Registry cpp
- Scripting: HotReload, Coroutine cpp
- mesh: MeshData.cpp, SculptLayersManager.cpp
- devices/vr/XrConfig.h

```bash
cmake -DKSENGINE_QT_FREE=ON ..
cmake --build . --target ksengine
```
