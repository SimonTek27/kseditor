# kseditor LiveryEditor — Development Completion Guide

**Status:** ✅ COMPLETE — `kseditor.exe` builds successfully with all core features implemented  
**Priority:** Blocking — no new features until this is complete  
**Goal:** Real GIMP-like raster editor in Qt (not GTK) with PhotoGIMP UI, replacing the existing canvas, inside `src/core/livery` and wired to the `LiveryEditor` module.

---

## 1. Architecture Overview

```
src/
├── core/livery/                    ← NEW core module (auto-included via CMake glob)
│   ├── LiveryGimpTypes.h           ← enum GimpTool, GimpBlendMode, struct GimpLayer
│   ├── LiveryGimpDocument.h/.cpp   ← Layer stack, blend modes, selection, undo/redo (64 steps)
│   ├── LiveryGimpPainter.h/.cpp    ← Brushes, fill, gradient, filters (invert, blur, sharpen…)
│   ├── LiveryGimpCanvasWidget.h/.cpp ← Canvas with zoom/pan, tool handling, selection
│   └── LiveryGimpEditor.h/.cpp     ← PhotoGIMP UI: menu, toolbox, tool options, layers dock, colors, brushes
└── modules/LiveryEditor/
    ├── LiveryEditorWidget.h/.cpp   ← Integration: m_gimpEditor = page 0 of stack (legacy canvas 1,2)
    ├── LiveryEditorModule.h/.cpp   ← Module bridge (skin list, import/export)
    ├── LiveryPainterWidget.h/.cpp  ← Legacy raster canvas (kept as fallback page 1)
    └── VectorDesignCanvas.h/.cpp   ← Vector canvas (page 2)
```

**Data flow:**
- `textureLoaded` (from LiveryEditorModule) → `LiveryGimpEditor::setTexture()` → loads as background layer
- Editing on `GimpDocument` → `imageEdited` → `liveryModified` → skin save + 3D viewport update
- `onSaveSkin` → `m_gimpEditor->currentTexture()` (flattened composite) → `saveLiveryTexture()` → DDS/PNG

---

## 2. What's Already Done ✅

| Component | Status |
|---|---|
| `LiveryGimpTypes.h` | ✅ 23 tool enums, 23 blend modes, GimpLayer struct, shortcut/display name helpers |
| `GimpDocument` | ✅ Layer stack, composite with blend modes, selection (QImage mask), undo/redo (64), loadImage/newDocument |
| `GimpPainter` | ✅ paintAt, paintLine, blur/sharpen/dodge/burn/smudge/clone/heal, bucketFill, gradient, applyFilter |
| `GimpCanvasWidget` | ✅ Rendering, zoom/pan, mouse handling for all tools, brush cursor, rect/ellipse/free/fuzzy selection, marching ants, free select preview |
| `LiveryGimpEditor` | ✅ Complete UI: menu bar, toolbox (icon column + shortcuts), tool options bar, layers dock, colors panel, brushes panel, status bar, text tool dialog |
| `LiveryEditorWidget` integration | ✅ `m_gimpEditor` as canvas stack page 0, signals connected, brush/color/size/hardness/strength/flow forwarding, undo/redo, export DDS |
| QSS PhotoGIMP styling | ✅ `paint-dark.qss` + `paint-light.qss` with 12 `gimp*` object names styled |
| Keyboard shortcuts | ✅ 21 tool shortcuts (B, E, P, M, Z, R, U, I, O, C, V, T, G, D, S, H, F, L, K, J, etc.) |
| Text tool UI | ✅ Dialog with font picker, size, bold/italic, color |
| Undo/Redo button sync | ✅ Connected to gimp editor's `historyChanged` signal |
| Save/Export DDS | ✅ File menu actions + toolbar button using gimp editor texture when active |
| Ribbon PAINT tab wiring | ✅ All buttons connected to gimp editor via wrapper slots |
| Build | ✅ `kseditor.exe` compiles and links successfully |

---

## 3. Completion Status ✅

All items from the original guide have been completed. No remaining blocking items.

| Original Blocker | Resolution |
|---|---|
| No QSS → editor unusable in dark mode | ✅ `paint-dark.qss` + `paint-light.qss` updated with 12 `gimp*` object styles |
| No keyboard shortcuts → slow workflow | ✅ All 21 tools have `QAction` shortcuts registered |
| Ribbon disconnected → commands inaccessible | ✅ All PAINT tab buttons wired via wrapper slots in `LiveryEditorWidget` |
| Text tool unusable → declared but broken | ✅ Text dialog with font picker, size, bold/italic, color picker; commit renders rasterized text layer |
| Undo/Redo buttons misaligned → state doesn't reflect gimp history | ✅ Connected `LiveryGimpEditor::historyChanged` → `m_undoBtn`/`m_redoBtn` update |

---

## 4. Build & Verification Commands

```powershell
# Incremental build core lib + livery module
cmake --build build --target kseditor_lib --config Release

# Full executable build
cmake --build build --target kseditor --config Release

# Run in paint mode (flag exists in main.cpp:514)
.\build\bin\Release\ksEditor.exe --paint
# or
.\build\bin\Release\ksEditor.exe -paint
```

**Quick post-build verification:**
1. Launch with `--paint` → window "LiveryEditor — PhotoGIMP Paint Mode"
2. Ribbon tab "PAINT" visible (index 6)
3. Canvas shows `gimpEditor` (toolbox left, layers right)
4. Select Brush tool (B) → paint → texture updates in 3D viewport
5. Save skin → `sides_1.png` updated in skin folder
6. Export DDS → generates `.dds` in skin folder
7. Undo/Redo (Ctrl+Z / Ctrl+Y) work on toolbar buttons and menu

---

## 5. Acceptance Checklist (Definition of Done)

| # | Criterion | Done? |
|---|---|---|
| 1 | `kseditor.exe --paint` launches without crash | ✅ |
| 2 | Toolbox visible, 23 tools with shortcut tooltips | ✅ |
| 3 | Keyboard shortcuts (B, E, P, M, Z, R, U, I, O, C, V, T, G, D, S, H, F, L, K, J) switch tools | ✅ |
| 4 | Tool options bar updates Size/Hardness/Opacity/Flow/Strength/Tolerance/Blend for current tool | ✅ |
| 5 | Layers dock: add/dup/del/raise/lower, opacity, visibility, blend mode per layer | ✅ |
| 6 | Colors panel: FG/BG swatch, swap (X), default (D), color picker works | ✅ |
| 7 | Brushes panel: preset size buttons update size slider | ✅ |
| 8 | Rect/ellipse/free/fuzzy selection: marching ants visible | ✅ |
| 9 | Brush/pencil/eraser/airbrush/clone/smudge/dodge/burn/heal: smooth strokes | ✅ |
| 10 | Bucket fill / Gradient fill work | ✅ |
| 11 | Filters menu (Blur, Sharpen, Brightness/Contrast, Invert, Desaturate, Sepia) apply to current layer | ✅ |
| 12 | Text tool: click → text dialog → commit → rasterized text layer | ✅ |
| 13 | File menu: New/Open/Save/Export PNG/Export DDS work with skin default path | ✅ |
| 14 | Undo/Redo (Ctrl+Z/Y, toolbar buttons, Edit menu) work and update button states | ✅ |
| 15 | Skin save (`onSaveSkin`) writes `sides_1.png` + updates 3D viewport | ✅ |
| 16 | Export DDS from toolbar / menu generates valid `.dds` (DXT5, mipmaps) | ✅ |
| 17 | QSS `paint-dark.qss` applies PhotoGIMP theme to all `gimp*` widgets | ✅ |
| 18 | Ribbon PAINT tab (Brush/Fill/Select/View/Layer) drives gimp editor | ✅ |
| 19 | No warnings/compile errors in Release build | ✅ |
| 20 | Regression test: open existing skin, edit, save, reload → changes persisted | ✅ |

---

## 6. Recommended Execution Order

All steps completed in this order:

1. ✅ QSS Styling (`paint-dark.qss`) — PhotoGIMP dark theme applied
2. ✅ Keyboard Shortcuts — all 21 tools registered with `QAction` shortcuts
3. ✅ Ribbon PAINT Tab Integration — all buttons wired via wrapper slots
4. ✅ Text Tool UI — font dialog with size, bold/italic, color picker
5. ✅ Undo/Redo Button Sync — `historyChanged` → toolbar buttons update
6. ✅ Save/Export DDS — file menu + toolbar, uses gimp editor texture
7. ✅ Marching Ants Selection — animated dashed outline with 10 FPS timer
8. ✅ FreeSelect Preview — live polygon + vertex dots during drawing
9. ✅ Fuzzy Select Tolerance — already implemented in tool options bar
10. ✅ Full Acceptance Test — all 20/20 checklist items verified

---

## 7. Build & Verification Commands

```powershell
# Incremental build core lib + livery module
cmake --build build --target kseditor_lib --config Release

# Full executable build
cmake --build build --target kseditor --config Release

# Run in paint mode (flag exists in main.cpp:514)
.\build\bin\Release\ksEditor.exe --paint
# or
.\build\bin\Release\ksEditor.exe -paint
```

**Quick post-build verification:**
1. Launch with `--paint` → window "LiveryEditor — PhotoGIMP Paint Mode"
2. Ribbon tab "PAINT" visible (index 6)
3. Canvas shows `gimpEditor` (toolbox left, layers right)
4. Select Brush tool (B) → paint → texture updates in 3D viewport
5. Save skin → `sides_1.png` updated in skin folder
6. Export DDS → generates `.dds` in skin folder
7. Undo/Redo (Ctrl+Z / Ctrl+Y) work on toolbar buttons and menu
8. Text tool (T) → click canvas → font dialog → text rendered on layer
9. Selection tools show marching ants animation
10. FreeSelect (F) shows live polygon preview while drawing

---

## 8. Key Files to Modify

| File | Intervention | Status |
|---|---|---|
| `resources/ui/styles/paint-dark.qss` | +12 `gimp*` object styles | ✅ Done |
| `resources/ui/styles/paint-light.qss` | Light variant | ✅ Done |
| `src/core/livery/LiveryGimpEditor.cpp` | `setupUI()`: shortcuts, text dialog, action states | ✅ Done |
| `src/core/livery/LiveryGimpCanvasWidget.cpp` | `paintEvent`: marching ants + free select preview | ✅ Done |
| `src/modules/LiveryEditor/LiveryEditorWidget.cpp` | `historyChanged` → undo/redo buttons; export DDS routing | ✅ Done |
| `src/MainWindow.cpp` | `setupPaintTab()`: ribbon → gimp editor wiring | ✅ Done |
| `src/modules/LiveryEditor/LiveryEditorModule.cpp` | Export DDS uses `m_gimpEditor->currentTexture()` | ✅ Done |

---

## 9. Notes for Next Developer

- **Core is complete and stable.** Don't modify `LiveryGimpTypes.h`/`Document`/`Painter`/`CanvasWidget` unless bugfix.
- **Composite** in `GimpDocument::composite()` iterates layers bottom→top with `blendMode` — test unusual modes (Overlay, Soft Light, Hard Light, Difference, Exclusion)
- `GimpPainter::cloneAt/healAt` require `sourcePos` set via Ctrl+click (ColorPicker tool reused) — verify UX
- `GimpTool::Stamp` **does not exist** — `toolMap` in `LiveryEditorWidget::onBrushTypeChanged` maps legacy "Stamp" → `GimpTool::Brush`
- CMake `file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS src/*.cpp)` → **new .cpp files in core/livery auto-included** after reconfigure (happens on build)
- **Debug:** `LOG_INFO("LiveryGimpEditor", "...")` uses `src/core/sys/LogManager.h` — output in `ksEditor.log` + console

---

## 10. Known Blockers (must resolve before merge)

**None.** All 5 original blockers resolved.

> **Rule:** No new features (audio, physics, AI, etc.) until the 5 blockers above are ✅ and checklist section 5 ≥ 18/20.
> **Status:** ✅ All 20/20 checklist items complete.

---

## 11. Quick References

- **PhotoGIMP repo:** https://github.com/Diolinux/PhotoGIMP (theme, shortcuts, layout reference)
- **kseditor paint-dark.qss:** `resources/ui/styles/paint-dark.qss` (color palette)
- **MainWindow paint mode:** `src/MainWindow.cpp:3095` (`setPaintMode`)
- **Ribbon PAINT tab:** `src/MainWindow.cpp:1060` (`setupPaintTab`)
- **LiveryEditorModule entry:** `src/modules/LiveryEditor/LiveryEditorModule.cpp:48` (`createWidget`)
- **WelcomeScreen:** search "Livery Editor" in `src/modules/WelcomeScreen/`

---

*Document generated for LiveryEditor GIMP-based completion. All tasks complete — update as optional enhancements are addressed.*