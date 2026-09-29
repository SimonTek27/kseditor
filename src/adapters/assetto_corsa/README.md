# Assetto Corsa / CSP adapters

Used by **SimulatorApp** (open-source AC+CSP product).  
**Not** linked into core `ksengine` as a hard dependency.

| Component | Role |
|-----------|------|
| `AcSharedMemory*` | AC overlay shared memory |
| `AcSurfacesLoader` | surfaces.ini → grip |
| `CspConfigParser` | Custom Shaders Patch configs |
| `ACGuidsParser` | GUID tables |
| `FSPROImporter` / `Exporter` | Audio banks |

SimulatorApp owns the product UX; these modules speak AC/CSP file formats.
