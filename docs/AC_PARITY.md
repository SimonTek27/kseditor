# SimulatorApp ↔ Assetto Corsa + CSP parity

**SimulatorApp** = open-source product aiming at the AC+CSP *experience*.
**ksengine** = generic engine underneath (no AC lock-in).

## Layer map
| AC / CSP | Our stack |
|----------|-----------|
| Game binary | SimulatorApp |
| Physics core | ksengine `physics/` |
| CSP | `adapters/assetto_corsa` (CspConfigParser + render hooks in app) |
| Shared memory | `AcSharedMemoryPublisher` |
| surfaces.ini | `AcSurfacesLoader` → TrackSurface |
| Overlays / apps | Same SM names as AC |
| Content | AC folder layout under configurable root |

## Done
- Qt-free runtime
- Pacejka / aero / suspension / FFB
- TrackSurface, LapSectorTimer
- Shared memory pages (physics / graphics / static)
- surfaces.ini loader
- UDP telemetry listener (AC-oriented)
- Native UI (menu, dash, devices, MP)

## Next (SimulatorApp-owned)
1. Wire SM + lap timer + surfaces into `SimulationLoop` every tick
2. Default content root = AC install; browser for car/track
3. KN5 → scene meshes
4. CSP config apply (lighting/post flags) without polluting engine
5. Session modes matching AC (practice / quali / race / hotlap)
6. Pit / flags / penalties in graphics page

## Forbidden
- CSP code inside `src/engine` core headers
- Qt on ksimulator hot path
