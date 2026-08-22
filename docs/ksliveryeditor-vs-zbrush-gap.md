# ksliveryeditor vs ZBrush — Feature Gap Analysis

> **Comparer:** ksliveryeditor (ksEditor Paint/Livery module, Qt6-based 3D texture paint) vs Maxon ZBrush 2025 (digital sculpting + Polypaint)
> **Date:** 2026-08-22
> **Version:** ksliveryeditor v1.16.x · ZBrush 2025.1
> **Purpose:** ksliveryeditor provides 2D texture canvas + 3D projection paint on car models with material mask channels, stencil/decal projection, and DDS export for Assetto Corsa. ZBrush is the industry-standard digital sculptor with Polypaint (vertex-color painting directly on high-poly meshes), Surface Noise, and alpha/texture projection. Assess where ksliveryeditor's texture-paint workflow lands vs ZBrush's surface-paint ecosystem, and identify which ZBrush capabilities are relevant for AC livery/content work.

---

## 1. Executive Summary

**ZBrush** (Maxon, since 2001) is the industry-standard *digital sculptor*: millions-of-polygons brush sculpting, Polypaint (vertex-color painting without UVs), Surface Noise/projection, Alpha brushes, Spotlight (image projection painting), and a UV unwrapping pipeline (UV Master). Its strength is *organic surface detail and surface paint at extreme polygon counts* — character sculpting, hard-surface detail, and concept art. It has no game-engine integration, no material-channel painting, and no DDS/KN5 export — its output is displacement/normal maps and high-poly meshes that must be baked down in other tools.

**ksliveryeditor** is a texture-paint tool for AC livery work: 2D layer-based canvas, 3D projection paint on car mesh, material mask channels, stencil/decal projection, and DDS export. It operates on game-ready meshes (KN5/FBX) at game resolution — the opposite end of the polygon scale from ZBrush.

The gap structure vs ZBrush:

1. **Surface paint paradigm** — ZBrush Polypaint paints vertex colors on high-poly meshes; ksliveryeditor paints texture maps via UV projection on game meshes. Different paradigms, different polygon scales.
2. **Sculpting surface detail** — ZBrush's core strength (sculpting + alphas + displacement) is in ksmodeler (the 3D modeling module), not ksliveryeditor. ksliveryeditor does not sculpt.
3. **Alpha/texture projection** — ZBrush's Spotlight projects images onto surfaces; ksliveryeditor's stencil/decal system is structurally similar but game-resolution focused.
4. **Organic modeling** — ZBrush is the reference for characters, creatures, organic forms; ksliveryeditor does not model at all.

**Overall parity:** ~15–20% against ZBrush as a *sculpting/painting tool* (because ksliveryeditor does not sculpt); but in *AC livery texture painting specifically*, ksliveryeditor covers 100% of what ZBrush cannot do (game UV pipeline, material channels, DDS export, KN5 integration).

---

## 2. Context & Methodology

Inventory from `src/core/paint/` (PaintEditor, PaintDocument, PaintCanvasWidget), `src/modules/PaintEditor/PaintEditorWidget.cpp` (tool setup, projection paint, stencil, material mask), `src/modules/modellingEditor/3DModelingQmlBridge.h` (sculpt brushes, multiresolution, sculpt layers — in ksmodeler, not ksliveryeditor), and `resources/ui/styles/paint-dark.qss` (GIMP-style dock UI) vs ZBrush 2025 documented feature set and Maxon release notes.

Rating scale: ✅ equivalent/better · 🟡 functional but weaker · 🔴 missing/weak · ➖ out of scope

---

## 3. Feature-by-Feature Matrix

| Capability | ksliveryeditor | ZBrush 2025 | Notes |
|------------|---------------|-------------|-------|
| **Sculpting** | | | |
| Brush-based mesh sculpting | ➖ | ✅ | ksliveryeditor does not sculpt — sculpting is in ksmodeler module (separate) |
| DynaMesh / remesh | ➖ | ✅ | ZBrush DynaMesh dynamic topology; ksmodeler has boolean ops only |
| Multiresolution sculpting | ➖ | ✅ | ZBrush Multires; ksmodeler has Mudbox-style multires (separate module) |
| Insert Multi-Mesh (IMM) brushes | ➖ | ✅ | ZBrush IMM for hard-surface kitbash; ksmodeler none |
| Array Mesh / NanoMesh | ➖ | ✅ | ZBrush instancing; ksmodeler none |
| **Polypaint (Vertex Color Painting)** | | | |
| Paint vertex colors without UVs | 🔴 | ✅ | ZBrush Polypaint is UV-free; ksliveryeditor requires UV-mapped texture |
| Polypaint to texture map | 🔴 | ✅ | ZBrush projects Polypaint to UV space; ksliveryeditor paints directly on UV texture |
| Polypaint resolution control | 🔴 | ✅ | ZBrush per-vertex vs per-pixel painting modes; ksliveryeditor fixed texture resolution |
| **Texture Paint (2D Canvas)** | | | |
| Layer-based 2D canvas | ✅ | 🟡 | ksliveryeditor full layer stack; ZBrush has Texture > layers but limited vs GIMP |
| Brush engine (size/hardness/flow) | ✅ | ✅ | Both core paint tools |
| Clone stamp | ✅ | 🟡 | ksliveryeditor clone; ZBrush clone brush exists but less refined |
| Eraser | ✅ | ✅ | Both |
| **3D Projection Paint** | | | |
| Paint on 3D model with UV projection | ✅ | ✅ | ksliveryeditor 3D viewport projection paint; ZBrush projects through UV onto mesh |
| Stencil / image projection | ✅ | ✅ | ksliveryeditor stencil/decal system; ZBrush Spotlight projects images with transform |
| Symmetry painting | ✅ | ✅ | ksliveryeditor symmetry on car templates; ZBrush mirror paint |
| **Alpha & Brush Texture** | | | |
| Alpha brush tips (grayscale) | 🔴 | ✅ | ZBrush extensive alpha library; ksliveryeditor basic round brush only |
| Texture-on-brush | 🔴 | ✅ | ZBrush can apply texture through brush; ksliveryeditor none |
| DragRect / DragDot stroke | 🔴 | ✅ | ZBrush stroke types for stamp placement; ksliveryeditor none |
| **Material System** | | | |
| Material mask painting (paint/carbon/chrome/matte) | ✅ | 🔴 | ksliveryeditor per-channel material painting; ZBrush has MatCap preview only, no game material channels |
| PBR material preview | ✅ | 🟡 | ksliveryeditor live PBR viewport; ZBrush MatCap materials (not PBR) |
| **UV & Mesh** | | | |
| UV unwrap | ➖ | 🟡 | ksliveryeditor does not unwrap (ksmodeler does); ZBrush UV Master basic |
| UV seam editing | ➖ | 🟡 | ZBrush UV Master control painting; ksmodeler LSCM/ABF++ |
| Mesh export (OBJ/FBX) | ➖ | ✅ | ZBrush exports high-poly for baking; ksliveryeditor exports KN5/DDS |
| **Color & Selection** | | | |
| FG/BG color swatches | ✅ | 🟡 | ksliveryeditor GIMP-style; ZBrush color picker in canvas |
| Color picker / eyedropper | ✅ | ✅ | Both |
| Selection tools (rect, lasso, wand) | 🟡 | ✅ | ksliveryeditor basic; ZBrush SelectRect/SelectLasso/Ctrl+Shift select |
| **Filters & Adjustments** | | | |
| Blur / sharpen / brightness | 🟡 | 🔴 | ksliveryeditor has basic filters; ZBrush Adjustments palette limited |
| Curves / levels | 🔴 | 🔴 | Neither has GIMP-style curves (ZBrush Color > Adjustments basic) |
| **Text** | | | |
| Text tool on canvas | ✅ | 🟡 | ksliveryeditor text tool; ZBrush text via Spotlight text overlay |
| **Export & Pipeline** | | | |
| DDS export with mip-chains | ✅ | 🔴 | ksliveryeditor native; ZBrush needs Substance/Photoshop |
| KN5 / AC pipeline | ✅ | 🔴 | ksliveryeditor native; ZBrush no game integration |
| Displacement / normal map baking | ➖ | ✅ | ZBrush Bake slider for displacement/normal; ksliveryeditor none (ksmodeler has normal bake) |
| GoZ (bridge to other DCCs) | ➖ | ✅ | ZBrush GoZ links to Maya/Blender/3ds Max; ksliveryeditor none needed |
| **Scripting** | | | |
| ZScript / Python API | 🔴 | ✅ | ZBrush ZScript + Python; ksliveryeditor no scripting for paint |

---

## 4. Category Gap Analysis

### 4.1 The Sculpting Divide — Different Tools, Different Scope

ZBrush is a *sculptor*. ksliveryeditor is a *painter*. These are fundamentally different tools:

- ZBrush's core value is **brush-based mesh deformation** at extreme polygon counts — organic forms, hard-surface detail, concept sculpting. Its Polypaint paints vertex colors as a *byproduct* of sculpting workflow.
- ksliveryeditor's core value is **texture-map painting via UV projection** on game-ready meshes — livery design, decal placement, material channel painting. It does not deform mesh geometry.

The sculpting capabilities exist in **ksmodeler** (the 3D modeling module), not ksliveryeditor:
- `3DModelingQmlBridge.h` exposes `sculptBrush()`, `multiresLevelList()`, `multiresAddLevel()`, `sculptLayerList()`, `sculptLayerAdd()` — Mudbox-style multiresolution sculpting with per-layer weights.
- `test_SculptMode.cpp`, `test_AdvancedSculpt.cpp` test suites cover brush operations and multires workflows.

ksliveryeditor and ZBrush's sculpting are **non-competing** — one paints textures, the other deforms geometry.

### 4.2 Polypaint vs Texture Paint — The Polygon-Scale Mismatch

ZBrush Polypaint paints vertex colors on meshes with millions of polygons — no UVs required, each vertex carries color. This is ideal for concept sculpting where UVs haven't been created yet. ksliveryeditor paints on UV-mapped texture images at game resolution (2048×2048 or 4096×4096) — the opposite end of the polygon scale.

For AC content, this mismatch resolves naturally:
1. **Sculpt in ZBrush/ksmodeler** → detail the high-poly mesh.
2. **Retopologize to game mesh** → create low-poly with UVs.
3. **Paint in ksliveryeditor** → project livery textures onto the game mesh via UV.

ksliveryeditor does not need Polypaint because AC car meshes already have UVs and are game-resolution. The Polypaint-to-texture-map workflow in ZBrush is a bridge between sculpting and texturing — ksliveryeditor starts after that bridge.

### 4.3 Spotlight vs Stencil Projection — Similar Concept, Different Target

ZBrush Spotlight projects images onto the mesh surface for painting — load an image, position/scale/rotate it, paint through it. ksliveryeditor's stencil/decal system (`projectionLoadStencil`, `projectionSetStencilPosition/Rotation/Scale`, `projectionPaint`) is structurally identical in concept but targeted at game assets:

- **Spotlight** projects onto high-poly sculpt meshes, works with Polypaint, outputs vertex colors or UV-projected textures.
- **Stencil/decal** projects onto game-ready KN5/FBX meshes, works with material mask channels, outputs DDS with mip-chains.

The feature parity is reasonable; the output pipeline is entirely different.

### 4.4 Alpha Brushes — ZBrush's Deep Advantage

ZBrush ships with thousands of alphas (grayscale brush tips) for surface detail — pores, scales, cracks, fabric weave, mechanical patterns. ksliveryeditor has a basic round brush only. For livery painting, alphas matter less (liveries are smooth paint + decals), but for custom brush effects (weathering, tire marks, rubber scuffs on painted surfaces), ZBrush's alpha library is a real advantage.

**Relevant for AC:** Weathering/damage paint on car liveries would benefit from alpha brush tips. This is a medium-priority gap.

### 4.5 Material Mask Painting — ksliveryeditor's Unique Edge

ZBrush has no concept of game material channels. Its "materials" are viewport preview shaders (MatCap), not per-pixel material property maps. ksliveryeditor paints **material mask channels** (paint, carbon, chrome, matte) — the AC-specific textures that control surface reflectivity, clearcoat, metallic behavior in-game.

This is entirely outside ZBrush's scope and represents ksliveryeditor's domain-specific moat.

---

## 5. Critical Gaps (blockers for ZBrush-adjacent workflows)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Alpha brush tips (grayscale brush textures) | Medium — weathering/damage brush effects | Low |
| 2 | Stroke types (DragRect, DragDot, Spray) | Medium — stamp placement for repeated patterns | Low |
| 3 | Texture-on-brush (brush pattern overlay) | Low — niche for livery work | Medium |
| 4 | Tablet/pressure sensitivity | High — natural painting feel | High |

## 6. Strategic Gaps

- **Surface Noise / procedural texture overlay** — ZBrush's Surface Noise generates procedural surface detail; ksliveryeditor could benefit from procedural weathering/grunge overlays for livery aging.
- **Polypaint-style vertex painting** — relevant only if ksliveryeditor ever needs to paint on meshes without UVs (unlikely for AC content).
- **GoZ-style bridge** — if ZBrush users sculpt car details, a bridge to export displacement/normal maps into ksliveryeditor's material system would close the sculpt→paint pipeline.
- **Brush alpha import** — support for .zbp (ZBrush brush) or .png alpha files for custom brush tips.

---

## 7. Where ksliveryeditor Already Wins

- **Texture-map painting via UV projection** — ZBrush Polypaint is vertex-color-based; ksliveryeditor paints UV-mapped textures at game resolution.
- **Material mask painting** — per-channel paint/carbon/chrome/matte; ZBrush has no game material channels.
- **Stencil/decal projection** — 3D image projection onto game mesh; structurally similar to Spotlight but game-targeted.
- **DDS export with mip-chains** — game-ready output; ZBrush requires external tools.
- **KN5 / AC pipeline integration** — load car, paint, preview, export; ZBrush has no game integration.
- **Layer-based compositing** — full layer stack with blend modes; ZBrush texture layers are limited.
- **PBR viewport preview** — see livery under realistic lighting; ZBrush uses MatCap (not PBR).
- **AC template system** — pre-built UV layouts per car model; ZBrush has no game-specific tooling.

---

## 8. Recommended Roadmap (ZBrush-relevant)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Alpha brushes** | Brush texture tips | Grayscale alpha import (.png, .zbp), brush alpha slider, DragRect/DragDot stroke types |
| **P2 — Tablet input** | Natural painting | Windows Ink / Wintab pressure/tilt sensitivity for brush size/opacity/flow |
| **P3 — Weathering tools** | Surface aging | Procedural grunge/noise overlay, dirt/scratch stamps, tire-mark alphas |
| **P4 — Displacement/normal import** | Sculpt→paint bridge | Import displacement/normal maps from ZBrush sculpt as reference layers in paint canvas |
| **P5 — Brush library** | Preset management | Save/load custom brush presets (size, hardness, alpha, flow, dynamics) |

---

## 9. Verdict

ZBrush and ksliveryeditor are **non-competing tools with a complementary handoff point**. ZBrush excels at organic sculpting, high-poly surface detail, and Polypaint concept work — none of which ksliveryeditor attempts. ksliveryeditor excels at game-resolution texture painting with material channels, 3D projection, DDS export, and AC pipeline integration — none of which ZBrush supports.

The relevant gap is **brush depth** (alphas, stroke types, tablet pressure) — the painting feel, not the sculpting paradigm. ZBrush's alpha library and stroke types would improve ksliveryeditor's weathering/damage paint quality, but these are incremental enhancements, not structural blockers.

For the AC content pipeline: sculpt in ZBrush or ksmodeler → retopologize → UV unwrap → **paint in ksliveryeditor** → export DDS → KN5. The tools are sequential stages in a pipeline, not alternatives.

**Recommendation:** Focus on P1 (alpha brushes) and P2 (tablet pressure) as the two highest-impact improvements to painting feel. Do not chase ZBrush's sculpting or Polypaint — those live in ksmodeler. Position ksliveryeditor as "where ZBrush's concept art becomes a game livery" — the paint station that bridges sculpting/ concept to game-ready output.
