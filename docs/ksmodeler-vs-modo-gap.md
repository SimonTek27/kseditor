# ksmodeler vs Modo — Feature Gap Analysis

> **Comparer:** ksmodeler (ksEditor 3D Modeler module) vs Modo 17.1 (final release; development discontinued by The Foundry, November 2024)
> **Date:** 2026-08-20
> **Version:** ksmodeler v1.16.x
> **Purpose:** Modo was renowned for its modeling ergonomics, MeshFusion, falloff/action-center system, Shader Tree, and mPath renderer — and was discontinued by The Foundry in 2024. Assess which Modo workflow ideas ksmodeler has absorbed, what it still lacks, and where displaced Modo artists can land.

---

## 1. Executive Summary

**Modo** (Luxology → The Foundry, 2004–2024) was the modeler's modeler: praised for the most fluid poly-modeling toolset in the industry, non-destructive **MeshFusion**, the **falloff + action-center** transform paradigm, the **Schematic** (node-graph scene), the layered **Shader Tree**, and the integrated **mPath** path tracer. It was weaker at animation and dynamics — its final additions (particles, fluids, cloth) came late and never reached Maya-grade maturity. The Foundry wound down development after Modo 17.1 (Nov 2024); no patching, docs/downloads removed Nov 2025.

Like Softimage before it, Modo's user base is now displaced. ksmodeler shares significant conceptual DNA (modifier/op stack, node material graph, boolean stack, procedural modeling) but is built around a different center of gravity: the Assetto Corsa content pipeline rather than generalist studio DCC work.

**Overall parity:** ~60–65% versus Modo 17.1 as a modeling-focused DCC — and unlike Modo, ksmodeler is alive, modern, and AC-native.

---

## 2. Context & Methodology

Inventory from `src/modules/modellingEditor/` (BooleanStack, ModifierStack, NodeMaterialEditor, GeometryNodes, ConstraintSystem, FCurveSystem, subdivision, symmetry) and `src/core/` vs Modo 17.x's documented feature set and community coverage. Modo was discontinued; the analysis measures *capability parity* (what a Modo artist needs) plus *orphan-migration fit* (what they get today).

Rating scale: ✅ equivalent/better · 🟡 functional but weaker · 🔴 missing/weak · ➖ out of scope

---

## 3. Feature-by-Feature Matrix

| Capability | ksmodeler | Modo 17.1 | Notes |
|------------|-----------|-----------|-------|
| **Modeling Core** | | | |
| Direct poly modeling (poly-by-poly) | 🟡 | ✅ | Modo's unstructured modeling toolset is the industry reference; ksmodeler solid but click-heavier |
| Modeling ergonomics (gesture, falloff) | 🟡 | ✅ | Modo's falloff + action-center transforms are unique; ksmodeler gizmo/keyboard basic |
| Action centers / spatial transform origins | 🔴 | ✅ | Missing; Modo's per-tool center/origin is a top-tier workflow feature |
| Falloff system (soft/spatial/weighted) | 🟡 | ✅ | ksmodeler has brush falloffs for sculpt/paint; no general modeling falloffs |
| Mesh primitives + presets | ✅ | ✅ | Both |
| N-gon tolerance | 🟡 | ✅ | ksmodeler triangulates earlier; Modo keeps n-gons and styles them in render |
| **Boolean & Non-Destructive** | | | |
| MeshFusion (CAGE-based fusion, live) | 🟡 | ✅ | ksmodeler BooleanStack strong but re-edit live-CAGE editing is Modo's crown feature |
| Modifier / op stack | 🟡 | ✅ | ksmodeler ModifierStack; Modo per-mesh MeshOps recorded and re-editable |
| Procedural / node modeling | 🟡 | 🟡 | ksmodeler 50+ geometry nodes; Modo MeshOps + Schematic; different but both capable |
| Symmetry / mirror / array | ✅ | ✅ | Both |
| **UV & Texturing** | | | |
| UV unwrap / editor | 🟡 | ✅ | ksmodeler LSCM/ABF++/xatlas; Modo UV tools (peel, relax, stitch, density) were top-tier |
| UV packing / texel density | 🟡 | ✅ | Modo density/packing optimization excellent; ksmodeler basic |
| UV projection playback / transfer | 🟡 | 🟡 | Modo stronger on exotic projections |
| **Shading & Materials** | | | |
| Node material graph | ✅ | ✅ | ksmodeler NodeMaterialEditor ~ Modo Shader Tree (layers, masks, normal stacks) |
| Texture paint / projection | ✅ | 🟡 | ksmodeler projection painter + paint layers; Modo 3D painting adequate but not its strength |
| **Rigging & Animation** | | | |
| Skeleton / rigging | ✅ | 🟡 | ksmodeler metarig + auto-weight; Modo rigging workable but never its strength |
| Weight painting | 🟡 | 🟡 | Both serviceable; neither Maya/3dsMax grade |
| FK/IK + spline IK | ✅ | 🟡 | ksmodeler FABRIK/CCD/spline IK solid; Modo's is later-era joits |
| Constraints | 🟡 | 🟡 | ksmodeler ConstraintSystem basic; Modo Constraints likewise limited |
| F-curve / animation | 🟡 | 🟡 | ksmodeler FCurveSystem + timeline; Modo animation historically the weak spot (sellers note "animating was next to impossible") |
| **Dynamics & Simulation** | | | |
| Particles (node graph) | 🟡 | 🟡 | ksmodeler ICE-style particle system; Modo particles (v12+) adequate |
| Cloth | 🟡 | 🟡 | ksmodeler ClothSystem; Modo cloth late-era, both limited |
| Fluid | 🔴 | 🟡 | Modo gained fluid early sim (v12+); ksmodeler has none |
| Rigid bodies | 🟡 | 🟡 | Both Bullet-based, workable |
| **Viewport & Rendering** | | | |
| Real-time viewport | ✅ | 🟡 | ksmodeler Vulkan PBR modern; Modo 17 viewport dated |
| Integrated production renderer | 🔴 | ✅ | Modo mPath (unbiased/biased path tracer) production-quality; ksmodeler experimental raytracer |
| Render passes / AOVs | 🔴 | ✅ | Modo full AOV/pass workflow; ksmodeler screenshot queue only |
| **Scripting & Pipeline** | | | |
| Scripting | 🟡 | 🟡 | ksmodeler Python/Lua/JS (pybind11); Modo Lua + Python API (aging, no longer developed) |
| Kit / smart-content system | 🔴 | ✅ | Modo Kits (parameters + presets); ksmodeler preset library, no smart kits |
| **AC-Specific Pipeline** | | | |
| KN5 / AC import-export | ✅ | 🔴 | ksmodeler native + encrypted; Modo needs unmaintained importers |
| Physics mesh / collision gen | ✅ | 🔴 | ksmodeler native; Modo none |
| LOD gen + validation | ✅ | 🔴 | ksmodeler native; Modo none |
| Live AC preview / telemetry | ✅ | 🔴 | ksmodeler native; Modo none |

---

## 4. Category Gap Analysis

### 4.1 Modeling Ergonomics — Modo's True Superpower

Modo's modeling magic was systemic: **falloffs** (spatial/volume/weighted soft falloff applied to any tool), **action centers** (scale/rotate around an arbitrary point, edge, grid, item — set per-tool), and one-click direct modeling tools. These made Modo the fastest tool for sculpt-like hard-surface iteration without a brush.

ksmodeler has gestures via shortcuts and gizmo manipulation, but lacks:

- **Generalized falloffs** — a modeling-falloff layer that applies to move/scale/rotate/edge ops, not just brushes.
- **Action centers** — per-operation origin/lock selection.
- **"Space-driven" transforms** — drag-from-element origins that Modo users rely on for speed.

This is the #1 gap *and* the #1 compat surface: it's what Modo artists feel the loss of most and what ksmodeler can credibly implement on top of its gizmo system.

### 4.2 MeshFusion vs BooleanStack

- Modo's **MeshFusion** lets you model *as smooth surfaces* with CAGE editing — boolean-like fusion of solid shapes with live smooth boundaries, embedded in the scene graph. ksmodeler's BooleanStack performs CGAL-grade CSG on meshes and re-runs, which is robust but nowhere near as fluid or smooth-ready.
- Path forward: expose the existing BooleanStack with **CAGE-style re-edit** (select a source op → push/pull its "cage" live → stack re-runs) and **smooth preview** on fused boundaries. That gets 80% of MeshFusion's value without a B-rep kernel.

### 4.3 Shader Tree vs NodeMaterialEditor

- Both are node-based layered material systems; ksmodeler's graph with 50+ node types plus GLSL/HLSL codegen is technically stronger in generating *game* shaders (AC ksCarPaint etc.).
- Modo's Shader Tree had superior **layer blending UX** (stack masks, logical operators, layer groups). ksmodeler should add mask/logic layers to its material editor for parity on the layered-material UX most Modo users like.

### 4.4 Rendering — mPath

- mPath is production path tracing with full AOV/pass + LightWave/3dsMax interchange-ish workflows. ksmodeler's RayTraceRenderer is experimental.
- For AC content, rendering is marketing/showroom (stills, pitch). ShowroomEditor (PBR lights, IES, clay/UV modes) covers the *product-shot* minimum. A 2026-relevant bridging move: strengthen RayTraceRenderer to a reliable product-still tracer with AOV basics rather than chasing mPath feature-parity.

### 4.5 Orphan Migration

- Modo 17.1 is frozen; downloads/docs vanish (Nov 2025); no OS-compat patches. Its artists must migrate — the market slot ksmodeler should target (exactly like Softimage).
- Migration blockers for Modo artists specifically: **workflow muscle memory** (falloffs/action centers — section 4.1), **scene ecosystem** (Kits, presets), and **import of `.lxo` scene files**.

---

## 5. Critical Gaps (blockers for Modo artist migration)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Generalized modeling falloffs (soft/spatial) | Blocking — the Modo signature workflow | Medium |
| 2 | Action centers / per-op transform origins | Blocking — Modo transform paradigm | Medium |
| 3 | CAGE-style live re-edit on boolean ops | High — approximates MeshFusion | Medium |
| 4 | Pure-path-traced stills + basic AOVs | Medium-high — showroom/pitch quality | High |
| 5 | Kit/smart-preset system | Medium — Modo's content ecosystem | Medium |
| 6 | `.lxo` scene + preset importer (legacy rescue) | High goodwill with Modo refugees | High |

## 6. Strategic Gaps

- **Smooth-boundary preview** on fused boolean edges (MeshFusion-like look).
- **Layered material mask/logic UX** in NodeMaterialEditor (Shader Tree parity).
- **UV density + professional packing optimizer** at Modo level for car interiors.
- **Import Modo-era Kits/presets**, or an equivalent first-party content market (leveraging ksmodeler's assets-library infrastructure).

---

## 7. Where ksmodeler Already Wins

- **AC-native everything**: KN5 (incl. encrypted), physics/collision mesh gen, LOD, live preview, validation, pack — Modo cannot open a KN5 without unmaintained hacks.
- **Robust booleans** (CGAL) — generally more reliable than MeshFusion on thin game-engine plates.
- **Geometry nodes / procedural modeling** — Modo's MeshOps + Schematic are similar but ksmodeler's node graph is actively developed.
- **Modern renderer/technics**: Vulkan PBR, shader hot-reload vs Modo's aging viewport.
- **Python 3 (pybind11) + Lua + JS** — Modo's Lua/Python surface is frozen.
- **Alive, free (MIT), cross-platform, actively maintained** — vs EOL commercial tool.

---

## 8. Recommended Roadmap (Modo-migration priority)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Ergonomics** | Modo workflow DNA | Modeling falloff system, action centers, drag-from-element transforms |
| **P2 — Non-destructive** | MeshFusion-ish | CAGE re-edit on BooleanStack, smooth boundary preview, live stack traces |
| **P3 — Shading** | Shader Tree parity | Mask/logic layers in node material, material layer stack UX |
| **P4 — Content** | Ecosystem | Kit/preset system, `.lxo`/`dx` importer, preset market |
| **P5 — Render** | Stills | Product-quality path tracer, AOV basics, showroom coupling |

---

## 9. Verdict

Modo's end-of-life (Nov 2024) creates the same kind of displaced-user opportunity ksmodeler has relative to Softimage — with an even clearer migration argument, because ksmodeler already implements most of Modo's *conceptual* model (op stack, boolean stack, node material graph, procedural nodes) and only lags on the *ergonomic jewels* (falloffs, action centers, CAGE fusion, Shader Tree UX).

The high-leverage path is **P1 + P2**: absorb Modo's modeling-ergonomics DNA (falloffs + action centers) and make the boolean stack CAGE-re-editable. Those three features are the ones Modo veterans feel most acutely, they are implementable on ksmodeler's existing stack, and no other free tool offers them either. Combined with the AC pipeline, that makes ksmodeler the natural landing spot for racing-content artists coming off Modo.

**Recommendation:** In your XSI-veteran out-reach story, add Modo: "the modeling tool that was loved and killed — now with a home that glues it to Assetto Corsa." Prioritize falloffs, action centers, and CAGE re-edit for the Modo cohort; treat mPath/render parity as a showroom-quality (not VFX-grade) target.