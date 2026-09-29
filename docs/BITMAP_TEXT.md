# Text rendering (bitmap)

## Stack
```
FontAtlas (8×8 R8 ASCII 32–126)
  → BitmapText (layout, wrap, align, shadow, outline)
    → TextRenderer (facade + HUD presets)
      → UiRenderer / DrawList verts
        → VulkanUiPipeline (alpha = atlas.r)
```

## TextRenderer
```cpp
TextRenderer text;
text.draw(ui, 20, 20, "Hello");
text.drawCentered(ui, 640, 100, "Title", TextRenderer::menuTitle());
text.drawf(ui, 20, 40, TextRenderer::hudValue(), "SPD %d", 247);

auto m = text.measure("Multi\nline");
```

## Presets
| Style | Use |
|-------|-----|
| `hudLabel()` | labels, muted |
| `hudValue()` | speed/rpm, outline+shadow |
| `menuTitle()` | large title |
| `menuItem()` | menu rows |

## NativeUiHub
Dashboard / telemetry / menu use `TextRenderer` with shared atlas.
