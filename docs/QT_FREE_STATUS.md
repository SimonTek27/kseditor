# KSEngine Qt-free status

**Updated:** 2026-09-28

## Engine = generic open-source sim core
AC/CSP → `src/adapters/assetto_corsa/` only.

## Latest Qt-free pass
- Graphics: SceneMesh, SceneObject, ShaderMaterial, ShaderParamRegistry, ComputePipeline, PBRUtils
- sys: UserProfile, TransactionManager, SystemDllManager, SystemDllInitializer
- assets: CloudSync, FormatConverter, Packaging, ProjectBuilder/Templates, RaceFlag, SimInstallDetector, AssetSearchEngine

## DeviceManager
Already Qt-free (comment-only QObject mention).

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```
