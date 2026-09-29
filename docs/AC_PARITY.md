# Content & overlay interoperability

> Historical note: some loaders and memory layouts match patterns used by
> popular racing titles so modders can reuse assets and overlays. **ksim does
> not claim affiliation with those titles.** Prefer the name **ksim** in docs
> and UI.

## Stack
```
ksim
 ├── ksengine
 ├── adapters/content   (was: adapters focused on shared layouts)
 └── network
```

## Technical targets (not branding)
| Feature | Implementation |
|---------|----------------|
| Car/track data folders | Content root + INI loaders |
| Overlay shared memory | Publisher with stable page layout |
| surfaces.ini-style grip | TrackSurface via content adapter |
| Lap / sector | LapSectorTimer |
| Multiplayer | network peer |

## Policy
- Keep format parsers in **adapters**, not in engine core identity.
- Do not require a third-party install path as the only content source.
- Window title / about box: **ksim** (or project name), not a third-party mark.
