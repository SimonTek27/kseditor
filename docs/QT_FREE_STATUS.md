# KSEngine Qt-free

**Updated:** 2026-09-28

## Link-set scan
After stubbing excluded monoliths: **0 real Qt hits** under `src/engine` for files that would compile into `ksengine`.

## Converted to std stubs (safe if include slips)
- physics: VehiclePhysics*, PhysicsSimulations, TrackPhysics, PhysicsValidator, TireCurveEditor, weather/*
- devices: 3dprint/* headers, scanners/* headers
- Audio: ksAssettocorsasndeventdefs redirect
- TextureTools / QualitySystem false positives cleaned

## Real runtime
Engine, VehicleSimulator, DeviceManager, MathCore, NativeRenderer, ConfigLoader,
SimucubeFFB, FFB bridges, Vulkan facades, INIParser

```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine
```
