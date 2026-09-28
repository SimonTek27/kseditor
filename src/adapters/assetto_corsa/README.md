# Assetto Corsa content adapters

**Not part of the core engine.**

These modules speak AC/CSP formats and conventions so the simulator can load
Assetto Corsa tracks, cars, banks, and CSP configs. The engine itself stays a
generic open-source simulation runtime (physics, devices, render, loop).

| Component | Role |
|-----------|------|
| `CspConfigParser` | CSP (Custom Shaders Patch) config files |
| `ACGuidsParser` | AC GUID tables |
| `FSPROImporter` / `FSPROExporter` | FMOD Studio / AC audio banks |
| `P3DModelLoader` | Bohemia/AC-related model bits if needed |
| `PAATextureConverter` | Arma/PAA-style textures (legacy tooling) |
| `KsAcSndEventDefs` | AC sound event name tables |

Core engine must not `#include` these from `physics/` or `Engine.h`.
Wire them only from the simulator app or a content-pack plugin.
