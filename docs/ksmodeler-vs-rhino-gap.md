# ksmodeler vs Rhino — Feature Gap Analysis

> **Comparer:** ksmodeler (ksEditor 3D Modeler module) vs Rhinoceros 8 (McNeel, current stable, Oct 2023 — Rhino 9 WIP soft-launch expected Sept 2026, with Grasshopper 2 shipping on R9)
> **Date:** 2026-08-20
> **Version:** ksmodeler v1.16.x
> **Purpose:** Rhino is the de-facto NURBS surface modeler for sim-racing content — most professional circuit/layout work, class-A body skins, wheel/rim geometry and prototyping in the AC community is still done in Rhino. Assess where ksmodeler's new real-NURBS/STEP surface layer lands vs Rhino's B-rep kernel, what it still lacks, and how the two should interoperate for an Assetto Corsa pipeline.

---

## 1. Executive Summary

**Rhino** (McNeel, since 1998) is the industry-standard *free-form NURBS surface modeler*: precise CV curve/surface editing, trim/join/blend/fillet surface operations, tolerance-driven geometry, and a huge Grasshopper (node-based algorithmic modeling) ecosystem. It is *not* a game DCC — no UV pipeline for games, no material shader graph, no rig/animation/dynamics, no render-to-game integration. Its strength is exact, engineering-grade surface geometry; AC track builders and class-A surfacers treat it as the reference.

**ksmodeler** recently gained a genuinely real NURBS layer (curves + trimmed-able-tolerant surfaces via `MeshOperations`: loft, sweep, revolve, pipe, extend, CV-slide, curvature combs, tessellation) plus STEP import/export — a first toehold in Rhino's territory. But the NURBS kernel is an embryo: no trimming, no surface booleans, no blend/fillet surfaces on NURBS, no 3dm interchange, no tolerance/precision model as a first-class workflow object.

**Overall parity:** ~40–45% of Rhino's *surface-modeling* scope (and dropping fast as ksmodeler's NURBS matures); in the *full AC asset pipeline* Rhino is ~10% and dependent on fragile round-trips. The framing is not replacement — it is **NURBS interop + class-A surfacing gap-closing** so ksmodeler can take over from where Rhino assets land.

---

## 2. Context & Methodology

Inventory from `src/core/mesh/MeshOperations.cpp` (NURBSCurve/NURBSSurface, `createCurve`, `loft`, `sweep`, `revolve`, `pipe`, `extendSurface`, `slideCV`, `curvatureComb`, `tessellateCurve/Surface`), `src/modules/modellingEditor/3DModelingQmlBridge.cpp` (`m_nurbsSurfaces`, `nurbsSurface*` bridge methods), `src/core/FileFormat/` (CADTypes linear/angular tolerance, CAD::STEPParser import, faceted-BREP STEP export) and `src/core/mesh/` (mesh `filletEdges`, `offsetFaces`, shell) vs Rhino 8's documented feature set and McNeel forum/Rhino World coverage. Rhino 9 WIP items (filleting overhaul, GlobalEdgeContinuity, Grasshopper 2) noted where relevant.

Rating scale: ✅ equivalent/better · 🟡 functional but weaker · 🔴 missing/weak · ➖ out of scope

---

## 3. Feature-by-Feature Matrix

| Capability | ksmodeler | Rhino 8 | Notes |
|------------|-----------|---------|-------|
| **NURBS Kernel** | | | |
| CV / NURBS curve editing | 🟡 | ✅ | ksmodeler has real NURBS curves (degree, periodic, de Boor); Rhino CV toolset (on-curve edit, rebuild, extend, fair) is the reference |
| NURBS surface creation | 🟡 | ✅ | ksmodeler: loft/sweep/revolve/pipe from control grids; Rhino: rail revolve, 2-rail sweep, network, blend, patch |
| NURBS surface editing | 🟡 | ✅ | ksmodeler extendSurface + slideCV only; Rhino: CV/UV-point edit, rebuild, change-degree, match, edit-surface-on |
| Trim / untrim surfaces | 🔴 | ✅ | Missing — Rhino's trim/join on trimmed surfaces is core |
| NURBS booleans / split | 🔴 | ✅ | Rhino SolidBoolean/Split; ksmodeler has mesh CGAL booleans only |
| Blend / fillet / chamfer surfaces | 🔴 | ✅ | Rhino FilletSrf/BlendSrf/ChamferSrf (overhauled in R9 WIP); ksmodeler has mesh-edge fillet only, no surface blends |
| Surface offset / thicken on NURBS | 🔴 | ✅ | Missing on NURBS; mesh shell/offsetFaces exist |
| Paneling / edge-curve tools for surfaces | 🔴 | ✅ | Rhino curve-boom for trim boundaries; ksmodeler track splines only |
| Mirror / array / orient | 🟡 | ✅ | ksmodeler mesh-array + NURBS revolve; Rhino WIP-style object arrays |
| **Precision & CAD** | | | |
| Tolerance / precision model | 🟡 | ✅ | ksmodeler has CAD linear/angular tolerance (CADTypes/CADConverter) for import/tessellation only; Rhino absolute/relative tolerance is a first-class scene property driving all ops |
| Units / engineering accuracy | 🔴 | ✅ | Rhino unit+dimensional workflow (mm/cm, tolerance, 3dm exactness); ksmodeler single working float world |
| Curve / surface continuity analysis | 🟡 | ✅ | ksmodeler has `curvatureComb`; Rhino CurvatureAnalysis, Zebra, GlobalEdgeContinuity (R9), match+tangent targeting |
| STEP / IGES interchange | 🟡 | ✅ | ksmodeler imports STEP solids (to mesh) + exports faceted BREP; Rhino native exact B-rep STEP/IGES round-trip |
| 3dm native format | 🔴 | ✅ | Missing — no 3dm read/write in ksmodeler |
| DWG/DXF / drafting layout | 🔴 | ✅ | Rhino Layout/annotation/dim sheets for 2D docs; ksmodeler none |
| 3D printing mesh checks | 🟡 | 🟡 | ksmodeler has 3DPrintModule; Rhino makes watertight solids + CECKEP/MeshRepair |
| **Parametric / Algorithmic** | | | |
| Node-based grasshopper-style modeling | 🟡 | ✅ | ksmodeler GeometryNodes (50+ node types); Rhino Grasshopper (GH2 launching with R9) is the field reference — same paradigm, different maturity |
| Dataflow / data-matching breadth | 🟡 | ✅ | GH2 typed data structures, clusters, solvers; ksmodeler node graph dataflow simpler |
| **Poly & SubD** | | | |
| Mesh modeling (CGAL booleans, sculpting) | ✅ | 🟡 | ksmodeler mesh-centric and, on thin game geometry, more robust than Rhino's mesh booleans |
| SubD as a surface type | 🟡 | ✅ | Rhino 7+ native precision SubD; ksmodeler has a subdivision modifier (OpenSubdiv) but no Rhino-style SubD surface tree |
| QuadRemesh / shrinkwrap retopo | 🔴 | ✅ | Missing; Rhino 7 QuadRemesh + Rhino 8 ShrinkWrap/PushPull |
| N-gon / polygroup handling | 🟡 | ✅ | ksmodeler FaceGroupSystem; Rhino quad-mesh workflow cleaner for surfacing |
| **Beyond Surfacing (DCC side)** | | | |
| UV unwrap / pack for games | ✅ | 🔴 | ksmodeler full UV pipeline (LSCM/ABF++/xatlas); Rhino has mesh texture icons only — not game-UV tooling |
| Node material graph / ksCarPaint | ✅ | 🔴 | ksmodeler NodeMaterialEditor; Rhino material slots only |
| Texture painting | ✅ | 🔴 | ksmodeler projection painter + paint layers; Rhino none |
| Rigging / animation | ✅ | 🔴 | ksmodeler metarig, IK/FK, constraints, F-curves; Rhino none |
| Physics / dynamics / particles | ✅ | 🔴 | ksmodeler cloth/hair/particles/rigid (Bullet); Rhino none |
| Real-time Vulkan PBR viewport | ✅ | 🟡 | ksmodeler modern PBR; Rhino viewport accurate for CAD shading but not game-grade realtime |
| Production renderer | 🔴 | 🟡 | Rhino 8 raytraced renderer + Enscape/V-Ray/Octane ecosystem; ksmodeler experimental raytracer + screenshot queue |
| Scripting | 🟡 | 🟡 | ksmodeler Python (pybind11)/Lua/JS; Rhino Python/RhinoScript/Grasshopper CPython + massive plugin ecosystem (Rhino.Inside) |
| **AC-Specific Pipeline** | | | |
| KN5 / AC import-export | ✅ | 🔴 | ksmodeler native + encrypted; Rhino needs laborious manual export chains |
| Physics mesh / collision gen | ✅ | 🔴 | ksmodeler native (convex hull, decomposition); Rhino none |
| LOD gen + validation | ✅ | 🔴 | ksmodeler native; Rhino none |
| Live AC preview / telemetry | ✅ | 🔴 | ksmodeler native SDKBackend; Rhino none |
| Track layout / circuit precision | 🟡 | ✅ | Rhino is the track modder's tool of record (exact lines, camber/elevation, complex curves); ksmodeler track splines exist but aren't Rhino-grade |

---

## 4. Category Gap Analysis

### 4.1 The Kernel Gap — Trim, Blend, and Solid NURBS

Rhino's value is not NURBS *existence* (many tools have that) but the **trimmed-surface B-rep engine**: you draw boundary curves, trim surfaces, blend/fillet/flange them, and boolean solids — all while edges stay exact and history-free. ksmodeler's `NURBSSurface` is a real, evaluable surface with loft/sweep/revolve/pipe builders, but it is:

- **Untrimmed** — every surface is a full control grid; no trimming against boundary curves, which is how every real car body is actually constructed.
- **No NURBS booleans/split** — a door panel cut from a quarter panel, a brake duct punched through a fender: these are mesh-CGAL ops today (robust but losing surface continuity) or simply unavailable.
- **No blend/fillet surfaces** — `filletEdges` operates on mesh edges with tangent-arc rounding, not Rhino's variable-radius rolling-ball blends between two implicit surfaces (dramatically upgraded in R9 WIP).
- **No surface offset** — the single-walled panel that Rhino shells trivially requires ksmodeler's mesh-dedup heroics.

Closing this is the entire class-A surfacing gap. A pragmatic path is to keep geometry mesh-at-rest but make the *editing paradigm* B-rep-like: trimmed-view surfaces, boundary-curve-driven splits, and NURBS→mesh re-tessellation on commit.

### 4.2 Precision & Tolerance — Rhino's Quiet Superpower

Rhino artists work in real engineering units with a scene tolerance; every intersection/boolean/snap/loft is resolved to that tolerance, so a rim model built in mm can go straight to CNC or to STEP for analysis. ksmodeler's `CADTypes.h` already carries `linearTolerance`/`angularTolerance` and `CADConverter` applies angular tolerance to tessellation — but tolerance is a *conversion parameter*, not a universal scene property that every op respects. For circuits (exact kerb lines, elevation targets, apex reference models) and landing-then-editing Rhino assets, ksmodeler needs a scene tolerance/unit model.

### 4.3 Grasshopper vs GeometryNodes — the Friendly Parallel

ksmodeler's node-graph procedural modeling is *structurally Rhino's Grasshopper*: dataflow components, instancing, parametric geometry. Rhino's own community just shipped Grasshopper 2 (typed data, better data-matching, performance) on the Rhino 9 train — so the paradigm is being actively re-legitimized. This is ksmodeler's best empathy hook with Rhino artists and the cheapest high-value convergence: make `GeometryNodes` speak the same *conceptual language* (expression-driven parameters, baked outputs, examine UI) and market it as "the accessible Grasshopper." Do not chase GH feature-parity; GH's physics/analysis solvers (Kangaroo, Karamba) are a separate universe.

### 4.4 Interop Is the Real Migration Vector

Almost no one starting a car/track in ksmodeler will re-model a Rhino asset by hand — yet ksmodeler cannot currently open a native `.3dm`. That is the single most common file in the AC modding world next to `.fbx`/`.kn5`. Priorities:

1. **3dm import** (read exact NURBS → ksmodeler NURBS surfaces; fall back to mesh/Rhino tessellation on demand) — unblocks every existing Rhino track/skin.
2. **Exact STEP/BREP round-trip** so tuned class-A surfaces survive (current STEP export is faceted).
3. **Shared-track workflow**: Rhino keeps layout/circuit precision; ksmodeler ingests, retessellates to game tolerance, and owns UV/LOD/physics/KN5 downstream.

### 4.5 Where Rhino Cannot Follow

Rhino stops at "a beautiful shell." It has no game UV texture atlas, no shader graph for ksCarPaint paint/clearcoat stacks, no rig/skin for the driver, no physics mesh, no LOD, no KN5, no live-in-game telemetry preview, no open-format licencing — every one of which ksmodeler ships natively. The AC content that starts in Rhino must complete its life in *some* game DCC; ksmodeler is the least-swappable one.

---

## 5. Critical Gaps (blockers for Rhino-pipeline adoption)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Trim/untrim + boundary-curve-driven surface splitting | Blocking — how every car body is actually built | High |
| 2 | 3dm import (exact NURBS, mesh fallback) | Blocking — the AC modding world's default source file | High |
| 3 | Exact STEP/BREP round-trip (export NURBS, not faceted) | High — class-A surface survival | Very High |
| 4 | Blend/fillet/chamfer surfaces on NURBS (rolling-ball, variable radius) | High — SG blend surfaces, wheel arches | Very High |
| 5 | Scene tolerance/unit model (universal, op-respecting) | High — camber, kerbs, rim precision | Medium |
| 6 | NURBS surface offset / thicken (double-wall panels) | High — shells, wings, ducts | High |

## 6. Strategic Gaps

- **Continuity tooling**: G1/G2 match + Zebra-style analysis to close the class-A surfacing loop (ksmodeler has `curvatureComb` as a seed).
- **NURBS booleans/split** on the surface layer itself.
- **Grasshopper-bridge**: import/export `.gh`/`.ghx` graphs into `GeometryNodes` (at least dataflow-simple graphs) — a magnet for Rhino parametric users.
- **QuadRemesh / ShrinkWrap** for scanned and boolean-result meshes on AC bodies/tracks.
- **Track-precision pass**: tolerance-aware splines with audit vs reference elevation/camber data.

---

## 7. Where ksmodeler Already Wins

- **End-to-end AC pipeline**: KN5 (incl. encrypted), physics/collision gen, LOD + validation, live preview/telemetry, pack — Rhino cannot deliver a single one natively.
- **Game-side completeness**: UV pipeline, node material graph (ksCarPaint), texture painting, rigging/animation, cloth/hair/particles — all absent in Rhino.
- **Robust mesh booleans** (CGAL) on thin game-engine geometry; Rhino's mesh booleans are a side-feature.
- **Modern real-time viewport** (Vulkan PBR, shader hot-reload) vs Rhino's CAD-accurate but game-irrelevant shading.
- **Real NURBS exist now** (`MeshOperations::createSurface/loft/sweep/revolve/pipe`, CV slide, curvature combs) — a credible seed for the P1 kernel work, and honestly further along than most "mesh-first" DCCs.
- **Open source (GPL-3.0), free, active** vs Rhino's commercial model (Rhino 9 WIP riding on R8 licenses at ~60% upgrade).
- **GeometryNodes node graph** — the same *paradigm* as Grasshopper, with a friendlier entry.

---

## 8. Recommended Roadmap (Rhino-relevant)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Interop** | Unblock existing Rhino assets | 3dm import (exact NURBS + mesh fallback), 3dm export of surfaces, exact STEP/BREP round-trip |
| **P2 — Kernel** | B-rep-like surfacing | Trim/untrim, boundary-curve split on NURBS surfaces, NURBS booleans (baseline) |
| **P3 — Blend & match** | Class-A quality | FilletSrf/BlendSrf variants on NURBS, G1/G2 match, zebra/continuity analysis panel |
| **P4 — Precision** | Engineering rigor | Scene tolerance/unit model threaded through every op, CAD audit for tracks |
| **P5 — Parametric bridge** | Grasshopper draw | `.gh` import into GeometryNodes, expression-driven parameters, bake workflows |

---

## 9. Verdict

Rhino is **not a competitor ksmodeler must beat — it is the origin pipeline** for the exact assets ksmodeler is built to finish (tracks, class-A bodies, rims, prototypes). The gap is real but now *migratory*: ksmodeler has genuine NURBS and STEP today, and the blockers are the B-rep habits (trim, blend, tolerance) plus the one file-format bridge (3dm) that would put every existing Rhino project on KSEditor's rails.

The strategic posture is **interop-first, kernel-second**: ship `3dm` + exact STEP round-trip so Rhino artists land their tuned geometry losslessly (P1), then grow the trim/blend/tolerance kernel (P2–P4) so they can finish class-A work in-editor. Position `GeometryNodes` as the approachable Grasshopper (P5) to convert the parametric crowd. The pipeline competitors to Rhino — Blender/3ds Max/Maya — all *lose* this fight because they cannot open KSEditor's game-native path either; ksmodeler's differentiator is being the NURBS-precision source *and* the game-ready destination.

**Recommendation:** Target the Rhino-track-modder and class-A surfer out-reach story — "start exact in Rhino, land lossless in ksmodeler, ship to Assetto Corsa" — and prioritize the 3dm + exact-STEP bridges ahead of feature-for-feature kernel parity. Once the trimmed-surface kernel lands (P2–P3), ksmodeler becomes the only AC-native DCC that speaks NURBS at all.