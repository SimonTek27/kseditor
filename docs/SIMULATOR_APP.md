# SimulatorApp (ksim)

Independent **open-source racing simulator** built on **ksengine**.

```
ksim
 ├── ksengine
 ├── adapters/content   (format loaders)
 └── network
```

## Principles
1. **Independent product** — own name, roadmap, and license surface.
2. **Format interoperability** — can load content that uses common racing-sim on-disk layouts (cars, tracks, surfaces, telemetry pages) so existing mod workflows stay useful.
3. **No brand coupling** — code and user-facing strings do not require or advertise a commercial third-party simulator as a dependency of identity.
4. **Engine stays generic** — ksengine remains usable by any simulator app, not only ksim.

## Compatibility (technical)
| Capability | Meaning |
|------------|---------|
| Content folders | Optional roots that mirror widely used car/track data layouts |
| Shared-memory pages | Optional publisher so existing overlay tools can attach |
| Surfaces / tyre / aero INI | Parsers under adapters/content |

Interoperability ≠ product affiliation.

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksimulator
```
