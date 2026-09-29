# Bitmap text rendering

## Stack
```
FontAtlas (8×8 R8, ASCII 32–126)
    → BitmapText (layout)
        → DrawList / UiRenderer verts
            → VulkanUiPipeline (R8 alpha)
```

## TextStyle
```cpp
TextStyle st;
st.color = Color::rgb(255, 255, 255);
st.scale = 2.f;
st.align = TextAlign::Center;
st.shadow = true;
st.outline = true;
st.maxWidth = 320.f; // word wrap

ui.addTextStyled(640, 100, "KS Simulator", st);
// or:
BitmapText bt(font);
bt.emit(text, x, y, st, verts, indices);
```

## Features
| Feature | Notes |
|---------|--------|
| Multiline | `\n` |
| Tab | expands to 4 spaces |
| Align | Left / Center / Right per line |
| Wrap | `maxWidth` breaks at spaces |
| Shadow | offset + color |
| Outline | 8-direction 1px (scaled) |
| Measure | `measure()` → width/height/lines |

## GPU
Fragment: `outColor.a *= texture(atlas, uv).r` (nearest filter).
