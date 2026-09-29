# Content format adapters

Path kept for history; treat as **content-format pack** for ksim.

These modules parse on-disk layouts (INI, banks, shared-memory page shapes,
surface tables) used by many racing mods. They exist so **ksim** can interoperate
with existing content and overlay tools.

**ksim is an independent product.** Format compatibility is not product branding.

| Component | Role |
|-----------|------|
| Shared-memory publisher | Overlay-compatible live pages |
| Surfaces loader | Grip tables → TrackSurface |
| Config / GUID / bank helpers | Optional content tooling |

Linked by **SimulatorApp** only. Never a hard dependency of `ksengine` core.
