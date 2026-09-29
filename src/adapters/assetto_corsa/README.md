# Assetto Corsa content adapters

**Not part of the core engine.**

| Component | Role |
|-----------|------|
| `AcSharedMemory*` | AC-compatible physics/graphics/static pages for overlays |
| `AcSurfacesLoader` | `surfaces.ini` → TrackSurface grip |
| `CspConfigParser` | CSP configs |
| `ACGuidsParser` | GUID tables |
| `FSPROImporter` / `Exporter` | FMOD / AC banks |

## Shared memory (Windows)
```
Local\\acpmf_physics
Local\\acpmf_graphics
Local\\acpmf_static
```
Publisher fills pages each physics tick from `AcLiveInput`.

```cpp
ks::ac::AcSharedMemoryPublisher pub;
pub.open();
// each frame:
pub.publish(liveInput);
```

## Surfaces
```cpp
ks::ac::AcSurfacesLoader surf;
surf.load(trackDir + "/data/surfaces.ini");
```
