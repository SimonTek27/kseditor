# Native UI overlays (Qt-free)

## Layout
```
src/simulator/ui/
  NativeUiTypes.h      DrawList / Color / Rect / UiVertex
  UiRenderer.h         dirty-flag batch → vertices/indices
  DeviceSettingsOverlay.h
  MultiplayerOverlay.h
  NativeUiHub.h        composes menu + dash + telem + panels
```

## Frame loop
```cpp
ks::sim::ui::NativeUiHub hub;
hub.resize(width, height);
// each frame:
hub.telemetry().update(speed, rpm, throttle, brake, steer, latG, lonG);
hub.renderFrame(width, height);
const auto& verts = hub.renderer().vertices();
const auto& idx   = hub.renderer().indices();
// upload to GPU / NativeRenderer
```

## Hotkeys
| Key | Action |
|-----|--------|
| Esc | Open/close game menu |
| F1  | Device settings overlay |
| F2  | Multiplayer overlay |
| Arrows / Enter | Navigate panels |

## Widgets
`DeviceSettingsWidget` / `MultiplayerWidget` map to overlays when `KSENGINE_QT_FREE=1`.
