# KSEngine Qt-free status

**Updated:** 2026-09-28

## Latest pass
- Config (ConfigLoader, Schema, Editor, CSP, PPFilter)
- network (NetworkManager, NetSystem, NetRace, NetworkConfig)
- material (MaterialSystem/Library, ShaderManager, Texture*, QmlBridge stub)
- FileFormat: INIParser (real minimal), JSON, MeshData, Project, KS3D,
  CAD/OBJ/STL/DXF/FBX/GLB, Bank parsers/writers, audio importers,
  AC/Alembic/BIS/FSPRO/Grasshopper/LXO/P3D/PAA/Rhino/USDA/XSI stubs

## Still excluded from CMake (editor monoliths)
VehiclePhysics*, TireCurveEditor, PhysicsSimulations, TrackPhysics,
PhysicsValidator, devices/3dprint, devices/scanners

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```

If a residual `#include <Q...>` appears, stub that path the same way.
Original Qt bodies remain in git history before stub commits.
