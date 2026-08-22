# ksmodeler vs 3ds Max — Feature Gap Analysis

> **Comparer:** ksmodeler (ksEditor 3D Modeler module) vs Autodesk 3ds Max 2025
> **Date:** 2026-08-20
> **Version:** ksmodeler v1.16.x
> **Purpose:** Identify feature gaps that prevent ksmodeler from being a drop-in replacement for 3ds Max in an Assetto Corsa / sim-racing content pipeline.

---

## 1. Executive Summary

**ksmodeler** is a Vulkan-based, Qt/QML modeling suite tightly integrated with the Assetto Corsa content pipeline. **3ds Max** is the de-facto industry standard for game-art hard-surface modeling and is the most commonly used external tool in the AC modding community (via the Kunos exporter).

ksmodeler already covers the *core* modeling surface: mesh editing, boolean ops, UV unwrap, modifier stack, rigging, sculpting. The gaps cluster into three zones:

1. **Production depth** — poly-modeling toolset breadth, retopology, cleanup utilities, UV packing/overlap resolution quality.
2. **Non-destructive pipeline maturity** — modifier stack correctness, instancing, referencing, MAXScript-style automation surface.
3. **Rendering/finalization** — no production renderer (Arnold-equivalent), limited scene/lighting tools.

**Overall parity:** ~65–70% of a typical AC-mod workflow in 3ds Max. ksmodeler wins on integration (KN5 native, physics mesh, live preview); 3ds Max wins on tool maturity, ecosystem and retargetable skill.

---

## 2. Context & Methodology

Capability inventory taken from `src/modules/modellingEditor/`, `src/core/mesh/`, `docs/modules.md` and `docs/KSEDITOR.md` (v1.16). Compared feature-by-feature against 3ds Max 2025 feature documentation. Ratings:

| Rating | Meaning |
|--------|---------|
| ✅ | Fully equivalent or better |
| 🟡 | Functional but meaningfully less capable |
| 🔴 | Missing or critically weak |
| ➖ | Not applicable / out of scope |

---

## 3. Feature-by-Feature Matrix

| Capability | ksmodeler | 3ds Max | Notes |
|------------|-----------|---------|-------|
| **Modeling & Mesh** | | | |
| Mesh primitives | ✅ | ✅ | Cube, sphere, cyl, cone, torus, plane, grid both |
| Editable poly modeling | 🟡 | ✅ | Vertex/edge/face/point modes exist; Max's polybyspoly + Graphite ribbon far deeper |
| N-gon support | 🟡 | ✅ | ksmodeler triangulates aggressively (`Ctrl+T` workflow), Max keeps n-gons |
| Boolean ops (CSG) | ✅ | 🟡 | CGAL-based, more robust than Max's ProBoolean on thin geometry; fewer control options |
| Bevel / chamfer | 🟡 | ✅ | Basic; Max bevel modifier has quad/fillet/angle control |
| Retopology | 🔴 | 🟡 | Missing; Max has Retopologize / quad-draw tools |
| Symmetry | ✅ | ✅ | SymmetryQmlBridge; Max has symmetry modifier + mirror |
| Loft / sweep / extrude | 🟡 | ✅ | Wizards exist; Max has true Loft compound object, PathDeform, Sweep modifier |
| Shell / inset / bridge | 🟡 | ✅ | Present but basic; Max inset/bridge robust |
| Normals / smoothing groups | 🟡 | ✅ | Recalculate normals only; Max has explicit smoothing groups + manual normals |
| OpenSubdiv (Catmull-Clark / Loop) | ✅ | ✅ | Both support; Max via OpenSubdiv modifier |
| **Non-Destructive** | | | |
| Modifier stack | 🟡 | ✅ | ksmodeler has ModifierStack but non-interactive sub-object editing of stack in Max is mature |
| Geometry nodes / procedural | 🟡 | 🟡 | ksmodeler 50+ node types (Blender-style); Max uses modifiers + PVC; different paradigm, both young |
| Shape keys / morph targets | ✅ | 🟡 | ksmodeler has MorphTargetEditor; Max uses Morpher |
| Instancing / referencing | 🟡 | ✅ | Max has true external file referencing (XRef) and instancing; ksmodeler basic instances |
| **UV & Texturing** | | | |
| UV unwrap (LSCM/ABF++/xatlas) | 🟡 | ✅ | Good unwrap algorithms but Max's Unwrap UVW editor has superior interactive tools |
| UV seams editing | 🟡 | ✅ | Seam editing exists; Max's peel/pelt/pack quality is best-in-class |
| UV packing | 🟡 | ✅ | Basic pack; Max packing with margin/rotation/alignment optimization |
| Texel density analysis | 🟡 | ✅ | Present in ksmodeler; Max has more density tools |
| Texture paint / projection | ✅ | ✅ | ksmodeler projection painter + paint layers solid |
| **Rigging & Animation** | | | |
| Skeleton generation (humanoid/quadruped) | ✅ | 🟡 | ksmodeler has metarig + auto-weight; Max relies on CAT/Biped (external complexity) |
| Weight painting | 🟡 | ✅ | ksmodeler has it; Max skin modifier is production reference |
| FK/IK solvers | ✅ | ✅ | FABRIK/CCD; Max has HI/HD/IK limb solvers |
| Spline IK | ✅ | 🟡 | Present in ksmodeler |
| Constraints | 🟡 | ✅ | ksmodeler ConstraintSystem basic; Max constraint list huge |
| F-curve / graph editor | 🟡 | ✅ | ksmodeler FCurveSystem exists but Max curve editor is reference |
| Cloth simulation | 🟡 | ✅ | ksmodeler ClothSystem basic; Max Cloth mature |
| Hair / fur | 🟡 | ✅ | ksmodeler HairSystem exists; Max Hair & Fur mature |
| **Dynamics & Particles** | | | |
| Rigid body dynamics | 🟡 | ✅ | ksmodeler RigidBodySystem (Bullet); Max MassFX reference |
| Particle system (ICE-style) | ✅ | ➖ | ksmodeler ICEParticleSystem + instancing; Max has PFlow (different, powerful) |
| **Viewport & Rendering** | | | |
| Real-time viewport (Vulkan/PBR) | ✅ | 🟡 | ksmodeler Vulkan PBR is modern; Max Nitrous dated but full-featured |
| Production renderer | 🔴 | ✅ | ksmodeler RayTraceRenderer experimental; Max Arnold/mental-ray grade |
| Render queue / farm | 🔴 | ✅ | Max has Backburner; ksmodeler has screenshot queue only |
| **Scripting & Pipeline** | | | |
| Scripting language | 🟡 | ✅ | ksmodeler Python/Lua/JS; Max has MAXScript (weak) but Python MXS is common |
| SDK / plugin API | 🟡 | ✅ | Max C++ SDK massive ecosystem |
| Batch processing | ✅ | ✅ | ksmodeler AdvancedBatchProcessor strong |
| **AC-Specific Pipeline** | | | |
| KN5 import/export | ✅ | 🟡 | Native + encrypted via plugin; Max needs Kunos exporter addon |
| Physics mesh (convex/hull) | ✅ | 🔴 | ksmodeler native PhysicsMeshGenerator; Max requires manual or custom scripts |
| LOD generation | ✅ | 🟡 | ksmodeler LODSystem quadric error; Max needs scripts |
| Live AC preview / telemetry | ✅ | 🔴 | Native SDKBackend; Max has none |
| Collision mesh authoring | ✅ | 🔴 | Native; Max manual |

---

## 4. Category Gap Analysis

### 4.1 Core Modeling & Topology

The largest practical gap. ksmodeler's mesh editing is serviceable for kit-bashing and clean box-modeling, but lacks:

- **Retopology / quad-draw** — no flow-based retopo for cleanup of scanned or boolean-result meshes.
- **Robust n-gon workflow** — 3ds Max artists rely on keeping n-gons during layout and only triangulating at export. ksmodeler pushes triangulation earlier.
- **Bevel/shape quality** — Max's bevel with `Shape` and `Quad` options is far more controllable for hard-surface.
- **Smoothing groups / explicit normals** — critical for automotive bodies (door gaps, panel lines). ksmodeler relies on auto-normal computation.

### 4.2 Non-Destructive Pipeline

- ksmodeler has a modifier stack and geometry nodes, but **no interactive sub-object editing while a stack is live**, no **stack-dependent falloff/animation**, and no **XRef-style external referencing**.
- Geometry nodes are Blender-inspired, which is fine, but 3ds Max modders use a *modifier* paradigm — muscle-memory mismatch, not a technical blocker.
- **Morpher/Shape keys** are fine but shape-key drivers/animatable blend amounts are basic.

### 4.3 Rendering & Finalization

- No production renderer. RayTraceRenderer exists but is experimental.
- No render farm, no USD export at production level (ASCII USD only), no multi-map material baking to AC's ksCarPaint standard (texture baker exists but is limited vs 3ds Max bake-to-texture).
- **AC-specific recommendation:** this matters less because the deliverable is a game asset, not a beauty render. But a minimum viable PBR still-shot (turntable/quality mode) is expected by mod teams.

### 4.4 Scripting

- ksmodeler's Python surface is strong and modern (pybind11), arguably nicer than MAXScript.
- Missing: **full-scene scripting API parity** (read/write every modifier, material, animation channel), **UI tool scripting** (build custom panels in scripts), and **macros-on-actions** breadth.
- The gap is breadth of bindings, not the language.

---

## 5. Critical Gaps (blockers for Max migration)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | Retopology / quad-draw tools | Blocking for organic + scanned cleanup | High |
| 2 | Smoothing groups & explicit normals | Blocking for automotive panel-line work | Medium |
| 3 | Production-quality UV editor (peel/pelt, overlap solve, density) | Blocking for multi-texture car interiors | High |
| 4 | External referencing (XRef) & robust instancing | Blocking for large track scenes | Medium |
| 5 | Advanced bevel/inset/bridge with shape options | High for hard-surface quality | Medium |
| 6 | MAXScript-parity scripting surface | High for pipeline automation | Medium |

## 6. Strategic Gaps (differentiators to chase)

- **USD pipeline** at production grade (import/export with layers, variants).
- **Render farm / distributed baking** to close the finalization gap.
- **Hair & cloth physics quality** — game-realistic wind/riding dynamics.
- **Particle grooming tools** for grass/spectators on tracks.
- **Template-based car builders maturity** — this is ksmodeler's unique edge; deepening it (from wizard to parametric live-tweak) widens the moat vs Max.

---

## 7. Where ksmodeler Already Wins

- **KN5 native + encrypted** — no exporter addon, no SDK mismatch.
- **Physics mesh generation** (convex hulls, primitive decomposition) — native, Max needs manual work.
- **LOD generation** integrated with validation.
- **Live preview + telemetry** — iterate car in-game without leaving the editor.
- **Modern real-time viewport** — Vulkan PBR beats Nitrous's legacy shading.
- **Car/track/character builders** — unique domain workflow 3ds Max doesn't have.
- **Modern scripting** — Python 3 + pybind11 is more approachable than MAXScript.

---

## 8. Recommended Roadmap (parity-prioritized)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Modeling hard-surface** | Close the modeling-topology gap | Retopo tools, smoothing groups, advanced bevel/inset/bridge, n-gon-tolerant pipeline |
| **P2 — UV production** | Reach Max UV quality | Peel/pelt unwrap, overlap resolution, packing optimizer, density heatmap |
| **P3 — Non-destructive** | Modifier stack maturity | Live stack preview, editable sub-object on stack, XRef external referencing |
| **P4 — Scripting** | API parity | Full scene-object graph binding, UI-scripting, action recorder export to Python |
| **P5 — Rendering** | Finalization | Production-quality stills, baking to AC shaders, distributed render queue |

---

## 9. Verdict

ksmodeler is **not yet a drop-in 3ds Max replacement** for a professional AC modding studio — the modeling-topology depth, UV editor and scripting breadth are the binding constraints. For an individual modder or small team producing car/track content *specifically for Assetto Corsa*, ksmodeler's integration advantages (KN5, physics, LOD, live preview) already outweigh 3ds Max's general-purpose depth, and the gap is narrowing fast as the P1–P3 roadmap lands.

**Recommendation:** Treat 3ds Max as the compatibility target for the P1–P5 roadmap above; prioritize retopology + smoothing groups + UV editor as the three must-have wins.
