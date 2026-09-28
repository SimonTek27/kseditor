# KSEngine Qt residual status

**Updated:** 2026-09-28

## Fixed in this pass (in link set)
- `devices/simracing/SimucubeFFB.cpp` — no QString/qDebug/qBound
- `mesh/ShapeKeyModifier.cpp` — std::string
- `Scripting/Blueprint/BlueprintTypes.cpp` — empty TU (editor only)
- `Graphics/StreamlineFunctions.h` — no qDebug

## Still on disk but **CMake-excluded** from ksengine
- physics: VehiclePhysics*, PhysicsSimulations, TrackPhysics, weather, TireCurveEditor
- devices: 3dprint/*, scanners/*
- Audio: ksAssettocorsasndeventdefs
- Blueprint/, ShapeKeyModifier (exclude)

## Runtime path (clean)
Engine, VehicleSimulator, DeviceManager, MathCore, NativeRenderer,
ConfigLoader, FFB (incl. Simucube), Vulkan facades
