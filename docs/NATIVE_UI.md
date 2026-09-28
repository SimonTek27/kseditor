# Native UI overlays (Qt-free)

## Font atlas
`ui/FontAtlas.h` bakes an **8×8 monospace** set for ASCII 32–126 into a **128×48 R8** texture.

```cpp
UiRenderer ui;
// GPU once:
uploadR8(ui.atlasPixels().data(), ui.atlasWidth(), ui.atlasHeight());
// each frame: ui.endFrame() → vertices with correct glyph UVs
```

- Solid rects/lines sample the **white texel** in the atlas.
- Text samples per-glyph UV; `fontScale` scales quads.
- `FontAtlas::loadR8(...)` can replace the built-in bake with an external sheet (same layout).

## Layout
```
ui/NativeUiTypes.h   DrawList
ui/FontAtlas.h       glyphs + R8 pixels
ui/UiRenderer.h      batch + UV
ui/DeviceSettingsOverlay.h
ui/MultiplayerOverlay.h
ui/NativeUiHub.h
```

## Hotkeys
Esc menu · F1 devices · F2 multiplayer · arrows/enter navigate
