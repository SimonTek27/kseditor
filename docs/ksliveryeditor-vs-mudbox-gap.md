# ksliveryeditor vs Autodesk Mudbox — Feature Gap Analysis

> **Comparer:** ksliveryeditor (ksEditor Paint/Livery module, Qt6-based 3D texture paint) vs Autodesk Mudbox 2025 (digital sculpting + 3D texture painting)
> **Date:** 2026-08-22
> **Version:** ksliveryeditor v1.16.x · Mudbox 2025
> **Purpose:** ksliveryeditor provides 2D texture canvas + 3D projection paint on car models with material mask channels, stencil/decal projection, and DDS export for Assetto Corsa. Mudbox is Autodesk's combined sculptor + 3D painter: multiresolution sculpting, stencil projection painting, paint layers, and tight Maya integration. Mudbox's paint toolset is the most directly comparable to ksliveryeditor's 3D paint workflow. Assess where ksliveryeditor's paint pipeline lands vs Mudbox's combined sculpt+paint surface, and identify which Mudbox paint capabilities translate to AC livery work.

---

## 1. Executive Summary

**Mudbox** (Autodesk, since 2007) is a *combined digital sculptor and 3D texture painter*: multiresolution sculpting (Mudbox-style level management that ksliveryeditor's ksmodeler module explicitly emulates — see `3DModelingQmlBridge.h:503` "Multiresolution sculpting (Mudbox-style level management)"), stencil projection painting, paint layers with blend modes, PBR viewport, and seamless Maya interoperability (GoZ-like Send To). Its paint system is the closest industry parallel to what ksliveryeditor does: paint directly on a 3D mesh via projection, with stencil overlays and layer compositing.

**ksliveryeditor** is a texture-paint tool purpose-built for AC livery work: 2D layer-based canvas, 3D projection paint on car mesh, material mask channels (paint/carbon/chrome/matte), stencil/decal projection, and DDS export with mip-chains. It operates on KN5/FBX game meshes within the AC asset pipeline.

The gap structure vs Mudbox:

1. **Multiresolution sculpting** — Mudbox's core sculpting feature; ksliveryeditor does not sculpt (that capability is in ksmodeler, which explicitly implements Mudbox-style multires — `multiresLevelList`, `multiresAddLevel`, `multiresSubdivide`, `multiresBake` in `3DModelingQmlBridge.h`).
2. **3D paint depth** — Mudbox's paint system (stencil projection, paint layers, Ptex support, bake-to-texture) is the most directly comparable to ksliveryeditor's. Mudbox paint is more general; ksliveryeditor's is game-pipeline-specific.
3. **Maya integration** — Mudbox's Send To workflow with Maya is a key production pipeline. ksliveryeditor's pipeline integration is KN5/AC, not Maya.
4. **Material channels** — Mudbox paints color + displacement + normal + specular as separate paint layers; ksliveryeditor paints per material mask channel (paint/carbon/chrome/matte) — different channel semantics for different pipelines.

**Overall parity:** ~40–45% of Mudbox as a *combined sculpt+paint tool* (because ksliveryeditor does not sculpt); ~75–80% of Mudbox's *paint-only* capabilities; but ksliveryeditor's *AC pipeline integration + material mask channels* are 100% unique and cover the exact workflow Mudbox cannot touch.

---

## 2. Context & Methodology

Inventory from `src/core/paint/` (PaintEditor, PaintDocument, PaintCanvasWidget, PaintPainter), `src/modules/PaintEditor/PaintEditorWidget.cpp` (tool setup, projection paint, stencil, material mask, layer panel), `src/modules/modellingEditor/3DModelingQmlBridge.h` (multiresolution sculpting explicitly labeled "Mudbox-style", sculpt layers, projection paint, stencil, tiling, matcap), and `resources/ui/styles/paint-dark.qss` vs Mudbox 2025 documented feature set and Autodesk release notes.

Note: ksmodeler's multiresolution sculpting (`3DModelingQmlBridge.h:503-512`) is explicitly implemented as "Mudbox-style level management" with `multiresLevelList`, `multiresAddLevel`, `multiresSubdivide`, `multiresBake`. Sculpt layers (`3DModelingQmlBridge.h:514-527`) implement Mudbox-style per-layer sculpting with blend modes, opacity, visibility, and bake. This is in the *modeling module*, not ksliveryeditor — but it is a direct Mudbox feature clone.

Rating scale: ✅ equivalent/better · 🟡 functional but weaker · 🔴 missing/weak · ➖ out of scope

---

## 3. Feature-by-Feature Matrix

| Capability | ksliveryeditor | Mudbox 2025 | Notes |
|------------|---------------|-------------|-------|
| **Sculpting** | | | |
| Multiresolution sculpting | ➖ | ✅ | Mudbox signature feature; ksmodeler implements Mudbox-style multires (separate module) |
| Sculpt layers (per-layer weights, blend, opacity) | ➖ | ✅ | ksmodeler has Mudbox-style sculpt layers (separate module); ksliveryeditor none |
| Sculpt brushes (grab, smooth, inflate, flatten, pinch, etc.) | ➖ | ✅ | Mudbox full brush set; ksmodeler sculpt brushes (separate module) |
| Stencil sculpting (project detail from image) | ➖ | ✅ | Mudbox stencil-based sculpting; ksmodeler none |
| **3D Texture Paint — Core** | | | |
| Paint directly on 3D mesh (projection) | ✅ | ✅ | Both project brush strokes through UV onto mesh |
| Paint layers with blend modes | ✅ | ✅ | ksliveryeditor layer stack; Mudbox paint layers (add, multiply, overlay) |
| Layer opacity | ✅ | ✅ | Both |
| Layer visibility toggle | ✅ | ✅ | Both |
| Layer reordering | ✅ | ✅ | Both |
| Layer groups | ✅ | 🟡 | ksliveryeditor group/ungroup; Mudbox no native layer groups (workaround: flatten) |
| **3D Texture Paint — Stencil** | | | |
| Stencil image projection | ✅ | ✅ | ksliveryeditor stencil/decal; Mudbox stencil projection with transform |
| Stencil transform (position/rotate/scale) | ✅ | ✅ | Both |
| Stencil opacity | ✅ | ✅ | Both |
| Stencil wrap (fit to surface curvature) | 🟡 | ✅ | Mudbox has surface-aware wrap; ksliveryeditor flat projection |
| **3D Texture Paint — Brushes** | | | |
| Round brush (size/hardness/flow) | ✅ | ✅ | Both core |
| Eraser | ✅ | ✅ | Both |
| Clone stamp | ✅ | 🟡 | ksliveryeditor clone; Mudbox clone exists but less refined |
| Smudge / blur brush | 🔴 | ✅ | Mudbox has smudge/blur; ksliveryeditor none |
| Paint through alpha (grayscale brush tip) | 🔴 | ✅ | Mudbox alpha brushes; ksliveryeditor basic round only |
| Spray / particle brush | 🔴 | ✅ | Mudbox spray; ksliveryeditor none |
| **3D Texture Paint — Channels** | | | |
| Color (diffuse/albedo) painting | ✅ | ✅ | Both |
| Specular / roughness painting | 🔴 | ✅ | Mudbox paints specular as separate channel; ksliveryeditor material masks are game-specific |
| Normal map painting | 🔴 | ✅ | Mudbox paint normals; ksliveryeditor none |
| Displacement painting | 🔴 | ✅ | Mudbox paint displacement; ksliveryeditor none |
| Material mask painting (paint/carbon/chrome/matte) | ✅ | 🔴 | ksliveryeditor per-channel game material masks; Mudbox has no concept of game material channels |
| Emissive / glow painting | 🔴 | ✅ | Mudbox emissive channel; ksliveryeditor none |
| **3D Viewport & Preview** | | | |
| PBR viewport | ✅ | ✅ | ksliveryeditor Vulkan PBR; Mudbox PBR viewport |
| MatCap / material preview | ✅ | 🟡 | ksliveryeditor matcap presets; Mudbox has MatCap + basic material slots |
| Wireframe overlay | ✅ | ✅ | Both |
| UV seam display | ✅ | ✅ | Both |
| **UV & Mesh** | | | |
| UV unwrap | ➖ | 🟡 | ksliveryeditor does not unwrap (ksmodeler does); Mudbox has basic auto-unwrap |
| UV layout editor | ➖ | ✅ | Mudbox UV editor; ksliveryeditor none (ksmodeler has UV tools) |
| Mesh import (OBJ/FBX) | ➖ | ✅ | Mudbox imports any mesh; ksliveryeditor uses KN5/FBX via ksmodeler |
| **Projection Paint Details** | | | |
| Visibility painting (hide faces while painting) | 🔴 | ✅ | Mudbox face visibility for painting backfaces; ksliveryeditor none |
| Flat影 (flat shading paint mode) | 🔴 | ✅ | Mudbox flat shade paint mode; ksliveryeditor none |
| Pick mode (sample existing texture) | 🟡 | ✅ | ksliveryeditor eyedropper; Mudbox pick-through-paint |
| **Export & Pipeline** | | | |
| DDS export with mip-chains | ✅ | 🔴 | ksliveryeditor native; Mudbox needs Substance/Photoshop |
| KN5 / AC pipeline | ✅ | 🔴 | ksliveryeditor native; Mudbox no game integration |
| Bake to texture (from high-poly) | ➖ | ✅ | Mudbox bake from sculpt to paint; ksliveryeditor none (ksmodeler has normal bake) |
| Maya Send To | ➖ | ✅ | Mudbox ↔ Maya live link; ksliveryeditor no Maya pipeline |
| **Layers — Sculpt vs Paint** | | | |
| Sculpt layers (separate from paint) | ➖ | ✅ | Mudbox independent sculpt + paint layer stacks; ksmodeler has sculpt layers (separate module) |
| Paint layers (separate from sculpt) | ✅ | ✅ | Both |
| Paint layer bake / merge down | ✅ | ✅ | ksliveryeditor flatten; Mudbox merge |
| **Scripting** | | | |
| MEL / Python API | 🔴 | ✅ | Mudbox MEL + Python; ksliveryeditor no scripting |

---

## 4. Category Gap Analysis

### 4.1 The Sculpting Divide — Mudbox's Feature That ksmodeler Already Cloned

Mudbox is defined by its combined sculpt+paint workflow: you sculpt at multiresolution levels, paint textures at any resolution, and sculpt and paint interactively. ksliveryeditor does not sculpt — but **ksmodeler explicitly implements Mudbox's sculpting paradigm**:

- `3DModelingQmlBridge.h:503` — `// Multiresolution sculpting (Mudbox-style level management)`
- `multiresLevelList()`, `multiresLevelCount()`, `multiresCurrentLevel()`, `multiresAddLevel()`, `multiresRemoveLevel()`, `multiresSubdivide()`, `multiresBake()`
- `3DModelingQmlBridge.h:514` — `// Sculpt layers (Mudbox-style per-layer sculpting)`
- `sculptLayerList()`, `sculptLayerAdd()`, `sculptLayerRemove()`, `sculptLayerSetBlendMode()`, `sculptLayerSetOpacity()`, `sculptLayerBakeCurrent()`

This is a direct Mudbox feature clone implemented in the modeling module. ksliveryeditor and Mudbox sculpting are **non-competing** — one paints, the other sculpts — but the combined sculpt+paint pipeline exists across ksmodeler + ksliveryeditor as two modules, just as Mudbox combines them in one app.

### 4.2 Paint System Comparison — The Closest Match

Of all the tools compared in these gap analyses, **Mudbox's paint system is the most directly comparable to ksliveryeditor**:

| Feature | ksliveryeditor | Mudbox |
|---------|---------------|--------|
| Projection paint on 3D mesh | ✅ | ✅ |
| Paint layers + blend modes | ✅ | ✅ |
| Stencil projection | ✅ | ✅ |
| Clone stamp | ✅ | ✅ |
| PBR viewport | ✅ | ✅ |
| Layer opacity/visibility | ✅ | ✅ |

The differences are in depth and target:
- **Mudbox** paints color + specular + normal + displacement + emissive as *separate channels* — each channel has its own layer stack. This is a general-purpose 3D paint workflow for film/game asset creation.
- **ksliveryeditor** paints color + material mask channels (paint/carbon/chrome/matte) — game-specific channels that control AC's shader behavior. This is a domain-specific workflow for AC liveries.

ksliveryeditor's material mask painting is **outside Mudbox's scope** — Mudbox has no concept of game material channels. Conversely, Mudbox's multi-channel paint (specular, normal, displacement) is **outside ksliveryeditor's scope** — AC liveries don't need those channels.

### 4.3 Brush Depth — Mudbox's Advantage

Mudbox has a richer brush library than ksliveryeditor:
- **Smudge/blur brush** — for blending paint strokes; ksliveryeditor has nothing equivalent.
- **Alpha brush tips** — grayscale textures as brush tips for surface patterns; ksliveryeditor has round brush only.
- **Spray/particle brush** — scattered paint application; ksliveryeditor none.
- **Visibility painting** — hide faces while painting backfaces; ksliveryeditor none.

For AC livery work, the most impactful missing brush is **smudge/blur** — blending paint strokes is common in livery design for gradient effects and soft edges. Alpha brushes matter for weathering/damage effects.

### 4.4 Stencil Projection — Near Parity

Both tools project images onto mesh surfaces via stencils. Mudbox's stencil is more mature:
- **Surface-aware wrap** — Mudbox's stencil conforms to surface curvature; ksliveryeditor projects flat.
- **Stencil sculpting** — Mudbox can sculpt detail from stencil images; ksliveryeditor only paints color through stencil.

For livery work, flat projection is usually sufficient (liveries are smooth decals on car panels). Surface-aware wrap matters more for organic sculpting.

### 4.5 Pipeline Integration — Different Targets, Same Philosophy

Mudbox's pipeline integration is **Maya-centric**: Send To workflow, FBX round-trip, bake to Maya material network. ksliveryeditor's pipeline integration is **AC-centric**: KN5 import/export, DDS with mip-chains, material mask channels, live PBR preview.

Both follow the same philosophy — paint in the 3D paint tool, export for the target pipeline — but target completely different production pipelines.

---

## 5. Critical Gaps (blockers for Mudbox-pipeline adoption)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Smudge / blur brush | High for paint blending quality | Medium |
| 2 | Alpha brush tips (grayscale brush textures) | Medium for weathering/damage effects | Low |
| 3 | Stencil surface-aware wrap | Low for flat car panels; medium for organic parts | Medium |
| 4 | Tablet/pressure sensitivity | High for natural painting feel | High |
| 5 | Visibility painting (hide faces while painting) | Low — rarely needed on car bodies | Low |

## 6. Strategic Gaps

- **Multi-channel paint** (specular, roughness, normal as separate paint layers) — relevant if AC ever exposes these as paintable channels in CSP.
- **Paint layer export** — Mudbox exports paint layers as separate texture maps; ksliveryeditor flattens to DDS. Layer-preserving export would benefit non-AC pipelines.
- **Mudbox ↔ ksliveryeditor bridge** — if a sculpt is done in Mudbox/ksmodeler, a "Send To ksliveryeditor" workflow for livery paint would close the sculpt→paint pipeline.
- **Stencil library management** — Mudbox organizes stencils by category; ksliveryeditor loads one-at-a-time. A stencil library (sponsor logos, number plates, pattern collections) would accelerate livery production.
- **Brush presets** — save/load named brush configurations (size, hardness, alpha, flow, dynamics).

---

## 7. Where ksliveryeditor Already Wins

- **Material mask painting** — per-channel paint/carbon/chrome/matte; Mudbox has no game material channels.
- **DDS export with mip-chains** — game-ready output; Mudbox needs external tools.
- **KN5 / AC pipeline** — load car, paint, preview, export; Mudbox has no game integration.
- **Layer groups** — ksliveryeditor supports layer folders; Mudbox does not (must flatten).
- **GIMP-style 2D canvas** — full 2D texture editor alongside 3D viewport; Mudbox's 2D canvas is minimal.
- **AC template system** — pre-built UV layouts per car model; Mudbox has no game-specific tooling.
- **PBR viewport** — both have PBR, but ksliveryeditor's is game-shader-accurate (ksCarPaint); Mudbox uses generic PBR.
- **Free (MIT)** vs Mudbox subscription ($105/month or $905/year).
- **Integrated into ksEditor** — no context switch; Mudbox is a standalone app requiring file round-trips.

---

## 8. Recommended Roadmap (Mudbox-relevant)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Brush depth** | Paint blending | Smudge/blur brush, alpha brush tip import (PNG), spray/particle brush |
| **P2 — Tablet input** | Natural painting | Windows Ink / Wintab pressure/tilt for size/opacity/flow dynamics |
| **P3 — Stencil library** | Workflow speed | Organized stencil browser (logos, numbers, patterns), drag-drop import, recent list |
| **P4 — Brush presets** | Consistency | Save/load named brush presets with all parameters |
| **P5 — Interop bridge** | Pipeline flow | "Send To ksliveryeditor" from ksmodeler / Blender for sculpt→paint workflow |

---

## 9. Verdict

Mudbox is the **most directly comparable tool** to ksliveryeditor's paint system — both paint directly on 3D meshes via projection, both have paint layers with blend modes, both use stencil projection, and both provide PBR viewport preview. The gap is in brush depth (smudge, alphas, spray) and Mudbox's combined sculpt+paint paradigm.

But the comparison reveals a **pipeline divergence, not a feature gap**:
- Mudbox is a general-purpose 3D paint tool for film/game asset creation, targeting Maya's material network.
- ksliveryeditor is a domain-specific livery paint tool for AC content, targeting KN5/DDS with material mask channels.

ksliveryeditor's material mask painting (paint/carbon/chrome/matte) is **entirely outside Mudbox's scope**. Mudbox's multi-channel paint (specular, normal, displacement) is **entirely outside ksliveryeditor's scope**. They serve different pipelines.

For the AC content pipeline: sculpt in ksmodeler (which already implements Mudbox-style multires + sculpt layers) → retopologize → UV unwrap → **paint in ksliveryeditor** → export DDS → KN5. The combined sculpt+paint workflow exists across ksmodeler + ksliveryeditor as two modules — functionally equivalent to Mudbox's combined approach, but in two specialized tools rather than one general-purpose app.

**Recommendation:** Focus on P1 (smudge/blur brush) and P2 (tablet pressure) as the two highest-impact painting improvements. These close the practical gap with Mudbox's paint quality while maintaining ksliveryeditor's pipeline-specific advantages. Do not chase Mudbox's sculpting — ksmodeler already cloned its multiresolution + sculpt layers paradigm. Position ksliveryeditor as "the 3D paint station for game liveries" — Mudbox-like paint workflow, zero game-pipeline friction.
