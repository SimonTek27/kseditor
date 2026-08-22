# ksliveryeditor vs PhotoGIMP — Feature Gap Analysis

> **Comparer:** ksliveryeditor (ksEditor Paint/Livery module, built on GIMP source code with Qt6 UI) vs PhotoGIMP 2024 (GIMP 2.10 fork with Photoshop-style UI, Diafast, Decent Sampler)
> **Date:** 2026-08-22
> **Version:** ksliveryeditor v1.16.x · PhotoGIMP (GIMP 2.10.38 fork)
> **Purpose:** ksliveryeditor wraps GIMP's core image processing pipeline (GEGL/BABL) inside a Qt6 widget tree, providing a 2D texture canvas, layer stack, brush engine, and 3D projection viewport for Assetto Corsa livery painting. PhotoGIMP is the closest GIMP-ecosystem comparison: a community fork that reskins GIMP with Photoshop-like UX, adds non-destructive adjustment layers, and ships pre-configured brushes/patterns. Assess where ksliveryeditor's Qt-UI reimplementation of GIMP primitives lands vs PhotoGIMP's full GIMP feature surface, and how the two should interoperate.

---

## 1. Executive Summary

**PhotoGIMP** is a GIMP 2.10 community fork that preserves GIMP's full 2D image editing surface — layers, masks, channels, paths, selection tools, filters, scripting (Script-Fu/Python-Fu), color management (ICC), and format breadth — while adding Photoshop-like UX conveniences (shortcut remapping, non-destructive adjustment layers, organized brush/pattern presets). It runs on GIMP's native GTK UI and uses GEGL for non-destructive processing.

**ksliveryeditor** is a Qt6/C++ reimplementation of a GIMP-inspired 2D paint canvas purpose-built for AC livery work. It inherits GIMP's core concepts — layer stack with blend modes, brush engine (size/hardness/flow), selection tools, color swatches, undo/redo — but reimplements them as Qt Widgets rather than wrapping the full GIMP codebase. Critically, it adds what GIMP/PhotoGIMP cannot: **3D projection painting directly on a car model with live PBR viewport**, **stencil/decal projection mapping**, **material mask painting** (paint/carbon/chrome/matte channels), and **direct DDS export with mip-chain generation** to AC's texture arrays.

The gap structure vs PhotoGIMP:

1. **2D image editing depth** — PhotoGIMP has full GIMP's filter library, channels, paths, masks, color management, scripting. ksliveryeditor's 2D canvas is functional but intentionally shallow (it is a livery tool, not a general image editor).
2. **Non-destructive workflow** — PhotoGIMP inherits GEGL's non-destructive adjustment layers. ksliveryeditor's layer stack is flat/destructive.
3. **Scripting & extensibility** — PhotoGIMP has Script-Fu/Python-Fu. ksliveryeditor has no scripting surface for paint operations.
4. **3D projection painting** — ksliveryeditor's decisive advantage: paint on the 3D model with real-time UV feedback, something PhotoGIMP/GIMP cannot do.

**Overall parity:** ~55–60% of PhotoGIMP's *2D image editing* scope; but ksliveryeditor's *3D paint + AC pipeline integration* is 100% unique and covers the exact workflow PhotoGIMP cannot touch.

---

## 2. Context & Methodology

Inventory from `src/core/paint/` (PaintEditor, PaintDocument, PaintCanvasWidget, PaintPainter, PaintTypes), `src/modules/PaintEditor/PaintEditorWidget.cpp` (tool setup, layer panel, skin panel, vector panel, palette panel), `resources/ui/styles/paint-dark.qss` / `paint-light.qss` (GIMP-style dock/toolbox UI naming), and `src/modules/modellingEditor/3DModelingQmlBridge.h` (projection paint, stencil, tiling) vs PhotoGIMP's documented feature set and GIMP 2.10 reference.

Rating scale: ✅ equivalent/better · 🟡 functional but weaker · 🔴 missing/weak · ➖ out of scope

---

## 3. Feature-by-Feature Matrix

| Capability | ksliveryeditor | PhotoGIMP | Notes |
|------------|---------------|-----------|-------|
| **2D Canvas & Editing** | | | |
| Canvas zoom/pan/navigate | ✅ | ✅ | Both standard |
| Undo/redo (multi-level) | ✅ | ✅ | ksliveryeditor: PaintDocument undo stack; PhotoGIMP: GIMP undo |
| Copy/cut/paste | ✅ | ✅ | Both standard |
| Crop / canvas resize | 🟡 | ✅ | ksliveryeditor basic; PhotoGIMP full GIMP crop/canvas tools |
| Rotate / flip / shear | 🟡 | ✅ | ksliveryeditor transform on layers; PhotoGIMP full transform dialog |
| Guides / grid / snap | 🟡 | ✅ | ksliveryeditor grid overlay; PhotoGIMP guides + grid + snap-to |
| **Selection Tools** | | | |
| Rectangular / elliptical select | ✅ | ✅ | Both |
| Free select (lasso) | 🟡 | ✅ | ksliveryeditor basic lasso; PhotoGIMP full GIMP free select |
| Magic wand (fuzzy select) | 🟡 | ✅ | ksliveryeditor tolerance-based; PhotoGIMP GIMP fuzzy select with threshold |
| Color range select | 🔴 | ✅ | Missing in ksliveryeditor |
| Foreground select (matting) | 🔴 | ✅ | GIMP foreground select tool; ksliveryeditor none |
| Quick mask | 🔴 | ✅ | GIMP quick mask mode; ksliveryeditor none |
| Select by color | 🔴 | ✅ | GIMP select-by-color; ksliveryeditor none |
| **Layer System** | | | |
| Layer stack with reorder | ✅ | ✅ | Both |
| Blend modes (normal, multiply, overlay, screen, etc.) | ✅ | ✅ | ksliveryeditor has standard modes; PhotoGIMP full GIMP set (~20+) |
| Layer opacity | ✅ | ✅ | Both |
| Layer groups / folders | ✅ | ✅ | ksliveryeditor `Ctrl+G` group; PhotoGIMP GIMP groups |
| Layer masks | 🔴 | ✅ | GIMP layer masks (grayscale, alpha, selection); ksliveryeditor none |
| Adjustment layers | 🔴 | ✅ | PhotoGIMP adds non-destructive adjustment layers (brightness/contrast, curves, hue/sat); ksliveryeditor destructive only |
| Alpha channel / transparency | 🟡 | ✅ | ksliveryeditor alpha per-layer; PhotoGIMP full alpha management |
| Layer effects / styles (drop shadow, bevel) | 🔴 | ✅ | GIMP has layer effects via Script-Fu; ksliveryeditor none |
| **Brush Engine** | | | |
| Round brush (size/hardness/flow) | ✅ | ✅ | Both core |
| Eraser tool | ✅ | ✅ | Both |
| Clone stamp | ✅ | ✅ | ksliveryeditor clone; PhotoGIMP GIMP clone tool |
| Airbrush / soft brush | ✅ | ✅ | Both via hardness/flow |
| Pattern / texture brushes | 🔴 | ✅ | GIMP has vast brush/pattern library; ksliveryeditor basic |
| Custom brush import (ABR/GBR) | 🔴 | ✅ | PhotoGIMP ships pre-configured GIMP brushes; ksliveryeditor none |
| Brush dynamics (pressure/tilt) | 🔴 | ✅ | GIMP dynamics; ksliveryeditor none (no tablet API) |
| MyPaint brushes | 🔴 | ✅ | GIMP integrates MyPaint brushes; ksliveryeditor none |
| **Color** | | | |
| FG/BG color swatches | ✅ | ✅ | Both (GIMP-style UI in ksliveryeditor) |
| Color picker / eyedropper | ✅ | ✅ | Both |
| Color dialog (HSV/RGB/CMYK) | ✅ | ✅ | ksliveryeditor QColorDialog; PhotoGIMP GIMP color dialog |
| Color palettes | ✅ | ✅ | ksliveryeditor palette panel; PhotoGIMP GIMP palettes |
| Color management (ICC profiles) | 🔴 | ✅ | GIMP has full ICC/color management; ksliveryeditor none |
| **Filters & Adjustments** | | | |
| Blur (Gaussian, box, motion) | 🟡 | ✅ | ksliveryeditor blur filter; PhotoGIMP full GIMP blur suite |
| Sharpen | 🟡 | ✅ | ksliveryeditor basic; PhotoGIMP unsharp mask + GIMP sharpen |
| Brightness/contrast | ✅ | ✅ | Both |
| Curves / levels | 🔴 | ✅ | GIMP curves/levels; ksliveryeditor none |
| Hue/saturation | 🔴 | ✅ | GIMP hue-saturation; ksliveryeditor none |
| Color balance | 🔴 | ✅ | GIMP color balance; ksliveryeditor none |
| Invert / grayscale / sepia | ✅ | ✅ | ksliveryeditor has these as filters |
| Noise reduction / denoise | 🔴 | ✅ | GIMP noise filters; ksliveryeditor none |
| Distort / warp | 🔴 | ✅ | GIMP distortions; ksliveryeditor none |
| artistic filters (oil paint, cubism, etc.) | 🔴 | ✅ | GIMP artistic; ksliveryeditor none |
| **Text Tool** | | | |
| Text entry on canvas | ✅ | ✅ | Both |
| Font selection | ✅ | ✅ | ksliveryeditor QFontComboBox; PhotoGIMP GIMP text tool |
| Text on path / in shape | 🔴 | ✅ | GIMP text-on-path; ksliveryeditor none |
| **Vector / Paths** | | | |
| Vector shape drawing | ✅ | ✅ | ksliveryeditor VectorDesignCanvas (rect, ellipse, polygon, line) |
| SVG import | ✅ | 🟡 | ksliveryeditor SVG import for decals; PhotoGIMP can import SVG via Inkscape bridge |
| Pen / bezier paths | 🔴 | ✅ | GIMP paths with bezier; ksliveryeditor no pen tool |
| Path stroke / fill | 🔴 | ✅ | GIMP path operations; ksliveryeditor vector shapes only |
| **Channels & Masks** | | | |
| RGBA channel editing | 🔴 | ✅ | GIMP channel editor; ksliveryeditor layer-level alpha only |
| Selection to channel | 🔴 | ✅ | GIMP saves selections as channels; ksliveryeditor none |
| Channel math / operations | 🔴 | ✅ | GIMP channel operations; ksliveryeditor none |
| **Format Support** | | | |
| DDS (DXT1/3/5) export | ✅ | 🟡 | ksliveryeditor native DDS + mip-chain; PhotoGIMP DDS via plugin |
| PNG / JPG / TGA / BMP | ✅ | ✅ | Both |
| PSD import/export | 🔴 | ✅ | GIMP reads PSD; ksliveryeditor none |
| TIFF / EXR / HDR | 🔴 | ✅ | GIMP format breadth; ksliveryeditor limited |
| **3D Paint & AC Pipeline** | | | |
| 3D projection paint on model | ✅ | 🔴 | ksliveryeditor's core advantage — paint directly on car mesh with UV projection; PhotoGIMP is 2D-only |
| Material mask painting | ✅ | 🔴 | Paint per-channel (paint/carbon/chrome/matte); PhotoGIMP none |
| Stencil/decal projection | ✅ | 🔴 | Load image, project onto 3D surface; PhotoGIMP none |
| 3D viewport with PBR preview | ✅ | 🔴 | Live lighting on painted livery; PhotoGIMP none |
| DDS export with AC mip-chains | ✅ | 🔴 | Game-ready output; PhotoGIMP needs manual DDS plugin |
| AC template system | ✅ | 🔴 | Pre-built UV layouts per car; PhotoGIMP none |
| **Scripting & Extensibility** | | | |
| Script-Fu / Python-Fu | 🔴 | ✅ | PhotoGIMP full GIMP scripting; ksliveryeditor none |
| Plugin architecture | 🔴 | ✅ | GIMP plug-in system; ksliveryeditor Qt widget only |
| Batch processing / automation | 🔴 | ✅ | PhotoGIMP batch via Script-Fu; ksliveryeditor none |
| **UI & Workflow** | | | |
| PhotoGIMP-style shortcut remapping | ✅ | ✅ | ksliveryeditor uses GIMP-style dock/toolbox naming (gimpToolbox, gimpLayerList, etc.) with Qt6 |
| Customizable workspace | 🟡 | ✅ | PhotoGIMP Photoshop-like workspace; ksliveryeditor fixed layout |
| Multi-window mode | 🔴 | ✅ | GIMP multi-window; ksliveryeditor single-window Qt |

---

## 4. Category Gap Analysis

### 4.1 The 2D Depth Gap — Filters, Channels, Masks

PhotoGIMP inherits GIMP's entire 2D image processing surface: curves, levels, color balance, channel operations, layer masks, quick mask, selection-to-channel, and 100+ filters. ksliveryeditor reimplements only the subset needed for livery painting (blur, sharpen, brightness/contrast, invert, grayscale, sepia). For *general-purpose image editing* (photo retouching, compositing, channel math), ksliveryeditor is deliberately not a competitor — it is a livery tool, not GIMP.

**Impact on AC workflow:** Minimal for most livery artists. The missing filters (curves, levels, noise reduction) matter primarily when editing source photography (sponsor logos, reference images) before projection onto the car. The workaround is to preprocess in GIMP/PhotoGIMP first, then import into ksliveryeditor for 3D projection — which is the recommended workflow.

### 4.2 The Layer Mask Gap

GIMP/PhotoGIMP's layer masks are a fundamental compositing primitive: paint grayscale to reveal/conceal layer content non-destructively. ksliveryeditor has no layer mask support — layers are flat/alpha-blended only. This limits complex livery compositions where artists need to selectively reveal texture detail beneath paint layers.

**Mitigation:** ksliveryeditor's material mask painting (per-channel paint/carbon/chrome/matte) provides a domain-specific alternative for the most common compositing need in livery work. Full layer masks would still benefit multi-layer designs.

### 4.3 The Scripting & Automation Gap

PhotoGIMP inherits GIMP's Script-Fu/Python-Fu scripting, enabling batch operations, procedural texture generation, and custom filter creation. ksliveryeditor has no scripting surface for paint operations. For repetitive livery tasks (batch color replacement, pattern fills across multiple skins), this forces manual work or external scripting.

**Opportunity:** ksEditor already has Python/Lua scripting in other modules. Exposing the paint canvas to Python (via pybind11 bindings to PaintDocument/PaintPainter) would close this gap without requiring GIMP's full plug-in architecture.

### 4.4 The 3D Projection Paint Advantage

This is where ksliveryeditor decisively wins and PhotoGIMP cannot compete at all. PhotoGIMP is a pure 2D image editor — it has no concept of a 3D model, UV coordinates, or texture projection. ksliveryeditor provides:

- **Direct 3D paint on car mesh** — brush strokes are projected through UV space onto the model in real-time.
- **Stencil/decal projection** — load any image, position/scale/rotate it in 3D space, and stamp it onto the surface.
- **Material mask painting** — paint per material channel (paint, carbon, chrome, matte) simultaneously.
- **Live PBR viewport** — see the livery under realistic lighting as you paint.
- **AC template system** — pre-built UV layouts per car model for instant alignment.

This is the entire reason ksliveryeditor exists: GIMP/PhotoGIMP cannot do 3D projection paint on game assets, and ksliveryeditor is purpose-built for that exact workflow.

### 4.5 Interop Recommendation

The two tools are **complementary, not competing**:

1. **Use PhotoGIMP/GIMP** for source image preparation: crop, color-correct, resize, remove backgrounds, generate patterns, apply filters to sponsor logos/reference images.
2. **Import prepared images into ksliveryeditor** for 3D projection painting onto the car model.
3. **Export from ksliveryeditor** as DDS with mip-chains for direct AC integration.

ksliveryeditor should not try to replicate GIMP's full 2D surface — that would duplicate effort and miss the point. The strategic play is to make the GIMP→ksliveryeditor handoff seamless (clipboard paste, drag-drop, shared file path) and focus ksliveryeditor development on 3D paint depth.

---

## 5. Critical Gaps (blockers for GIMP/PhotoGIMP migration)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Layer masks (grayscale/alpha/selection) | High for complex compositing | Medium |
| 2 | Curves / levels adjustment | High for photo retouching of source images | Medium |
| 3 | Selection refinement (grow/shrink/feather, color range) | Medium for precise decal placement | Low |
| 4 | Scripting surface (Python bindings to paint canvas) | Medium for batch operations | Medium |
| 5 | Tablet/pressure sensitivity | Medium for natural painting feel | High |

## 6. Strategic Gaps

- **Non-destructive adjustment layers** — the PhotoGIMP differentiator over stock GIMP; would benefit ksliveryeditor for live livery preview.
- **Brush dynamics** (pressure/tilt via Windows Ink/Tablet API) — critical for natural painting; ksliveryeditor currently mouse-only.
- **Pattern fill / texture brushes** — GIMP's pattern库 is vast; ksliveryeditor has no pattern system.
- **Clipboard integration with GIMP** — shared clipboard for copy/paste between PhotoGIMP and ksliveryeditor would eliminate file-save steps.
- **GIMP plug-in compatibility layer** — not full GIMP plug-in API, but a thin adapter so GIMP's most common Script-Fu operations can run inside ksliveryeditor's paint canvas.

---

## 7. Where ksliveryeditor Already Wins

- **3D projection paint on car model** — PhotoGIMP/GIMP is 2D-only; ksliveryeditor paints directly on 3D mesh with UV projection.
- **Material mask painting** — per-channel paint/carbon/chrome/matte; PhotoGIMP has no concept of game material channels.
- **Stencil/decal projection** — load image, project in 3D space; PhotoGIMP has no 3D.
- **Live PBR 3D viewport** — see livery under realistic lighting; PhotoGIMP renders 2D only.
- **DDS export with mip-chains** — game-ready output; PhotoGIMP needs manual DDS plugin configuration.
- **AC template system** — pre-built UV layouts per car; PhotoGIMP has no game-specific tooling.
- **Qt6 native UI** — faster, more responsive than GIMP's GTK on Windows; modern widget styling (GIMP-inspired dock/toolbox naming).
- **Direct KN5/car integration** — load car model, paint, preview, export without leaving the editor.

---

## 8. Recommended Roadmap (GIMP-relevant)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Selection & mask** | Compositing depth | Layer masks (grayscale, alpha), selection refine (grow/shrink/feather), quick mask |
| **P2 — Adjustments** | Photo retouching | Curves, levels, hue/saturation, color balance (as non-destructive adjustment layers) |
| **P3 — Brush dynamics** | Natural painting | Windows Ink / Tablet API pressure/tilt sensitivity, brush dynamics engine |
| **P4 — Scripting** | Automation | Python bindings to PaintDocument/PaintPainter, batch operations API |
| **P5 — Interop** | GIMP handoff | Clipboard sharing, drag-drop import, shared color palette, GIMP Script-Fu adapter |

---

## 9. Verdict

PhotoGIMP is the *full 2D image editing reference* — its filter library, channel/mask system, scripting, and format breadth are inherited from 20+ years of GIMP development. ksliveryeditor intentionally does not compete on 2D depth; it reimplements only the livery-relevant subset (layers, brushes, basic filters, color tools) inside a Qt6 shell.

The strategic posture is **complementary workflow**: PhotoGIMP for source image preparation, ksliveryeditor for 3D projection paint and AC export. The gap that matters is not 2D feature parity but **seamless handoff** (clipboard, drag-drop, shared palettes) and **brush dynamics** (tablet pressure) — the two things that would make the GIMP→ksliveryeditor pipeline feel like one tool.

ksliveryeditor's unique value — 3D paint on car mesh, material masks, stencil projection, PBR viewport, DDS export — is entirely outside PhotoGIMP's scope. No amount of GIMP plug-in development can replicate real-time 3D projection painting on a game asset. That is ksliveryeditor's moat, and it should be deepened (brush dynamics, multiresolution paint, tileable texture projection) rather than diluted by chasing 2D filter parity.

**Recommendation:** Position ksliveryeditor as "the GIMP for game liveries" — same 2D editing concepts artists already know from PhotoGIMP/GIMP, plus the 3D paint layer that GIMP cannot offer. Prioritize P1 (layer masks) and P3 (tablet pressure) as the two highest-impact gaps; P5 (GIMP interop) as the long-tail convenience play.
