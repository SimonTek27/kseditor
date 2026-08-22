# ksmodeler vs Plasticity — Feature Gap Analysis

> **Comparer:** ksmodeler (ksEditor 3D Modeler module) vs Plasticity Studio 24 (2026)
> **Date:** 2026-08-20
> **Version:** ksmodeler v1.16.x
> **Purpose:** Assess whether Plasticity's CAD-shell hard-surface modeling workflow can be absorbed into, or is threatened by, ksmodeler — and what ksmodeler lacks compared to Plasticity's modeling speed.

---

## 1. Executive Summary

**Plasticity** is a focused, single-purpose *hard-surface* modeler built on a solid-modeling (Parasolid-class) kernel, optimized for fast, keyboard-driven boolean/bevel/offset workflows used heavily by concept and props artists. It intentionally has **no** UVs, texturing, rigging, animation, particles or rendering.

**ksmodeler** is a full DCC: entire modeling pipeline plus UV, materials, rigging, simulation, and the Assetto Corsa export chain.

The comparison is not apples-to-apples. Plasticity wins in **modeling ergonomics and kernel fidelity** for hard-surface; ksmodeler wins everywhere else by being a complete DCC. The realistic framing: **does ksmodeler need to match Plasticity's modeling head** to keep AC modders from leaving for a Plasticity → ksmodeler round-trip workflow?

**Answer:** partially. ksmodeler's CGAL booleans are robust, but it lacks Plasticity's signature destructive-modification *speed*: tool-class sketch-driven workflows, fillets/chamfer hybrid chains, and clean offset/shell features on CAD-grade topology.

**Overall parity:** ~50% in Plasticity's niche; ~200%+ everywhere else (by scope).

---

## 2. Context & Methodology

Inventory from `src/modules/modellingEditor/` (BooleanOps/CGAL, ModifierStack, WizardSystem, GeometryNodes, ModelingOps) vs Plasticity's documented feature set and Steam/Dev thread coverage. Note: Plasticity moved from Parasolid to an internal kernel in 2024 → precision comparison uses capability, not kernel name.

---

## 3. Feature-by-Feature Matrix

| Capability | ksmodeler | Plasticity | Notes |
|------------|-----------|------------|-------|
| **Kernel & Precision** | | | |
| CSG boolean engine | ✅ | ✅ | ksmodeler CGAL-BSP robust; Plasticity kernel-class booleans (Loft/offset friendly) |
| Kernel-precision (exact arithmetic) | 🟡 | ✅ | CGAL robust but manifold-repair edge cases differ; Plasticity solid-modeling kernel exact |
| **Modeling Workspace** | | | |
| Keyboard-first / minimal UI | 🟡 | ✅ | ksmodeler has shortcuts but a full IDE UI; Plasticity lean, gesture-driven |
| Sketch / 2D outline driven | 🔴 | ✅ | Plasticity sketch + extrude/lathe core workflow; ksmodeler lacks 2D sketch plane tools |
| Extrude / revolve / sweep | 🟡 | ✅ | ksmodeler via wizards; Plasticity first-class tools |
| Loft surfaces | 🟡 | ✅ | Loft wizard exists; Plasticity loft/hull quality higher |
| Shell / offset / thickening | 🔴 | ✅ | Missing; Plasticity offset gives nested shell topology |
| **Fillets & Chamfers** | | | |
| Fillet tool | 🟡 | ✅ | ksmodeler basic edge bevel; Plasticity radius/order/rolling support superior |
| Chamfer + fillet hybrid chains | 🔴 | ✅ | Missing in ksmodeler |
| Solid→solid face interactions | 🔴 | ✅ | Missing |
| **Topology / Edit** | | | |
| Dynamic quad / re-mesh | 🔴 | ✅ | Plasticity subdiv-D preview & mesh re-gen; ksmodeler has subdivision modifier but not dynamic born-from-solid |
| Mirror / array / pattern | ✅ | ✅ | ksmodeler symmetry + array wizard; Plasticity pattern tools |
| Manageable n-gons / polygroups | 🟡 | ✅ | ksmodeler has FaceGroupSystem; Plasticity treats quads-from-solid cleanly |
| **Beyond Modeling** | | | |
| UV editing | ✅ | 🔴 | ksmodeler full UV; Plasticity none |
| Texture / material system | ✅ | 🔴 | ksmodeler node graph; Plasticity none (material slots only) |
| Rigging / animation | ✅ | 🔴 | ksmodeler full; Plasticity none |
| Sculpting | ✅ | 🔴 | ksmodeler dynamic tessellation brushes; Plasticity none |
| Simulation (cloth/hair/particles/rigid) | ✅ | 🔴 | ksmodeler multiple systems; Plasticity none |
| Rendering / viewport (Vulkan PBR) | ✅ | 🟡 | ksmodeler Vulkan PBR; Plasticity viewport good but no render |
| Scripting | ✅ | 🔴 | ksmodeler Python/Lua/JS; Plasticity limited automation |
| **Export / Pipeline** | | | |
| KN5 / AC pipeline | ✅ | 🔴 | ksmodeler native; Plasticity exports generic formats |
| FBX / OBJ / GLB | ✅ | ✅ | Both |
| STEP / IGES / CAD interchange | 🔴 | ✅ | ksmodeler imports STEP/IGES as mesh (CADFormatDetector) but no true NURBS exchange; Plasticity native |
| NURBS / patches | 🔴 | ✅ | Plasticity true NURBS-capable; ksmodeler no NURBS editing |

---

## 4. Category Gap Analysis

### 4.1 Hard-Surface Modeling Speed

Plasticity's core value is *round-trip speed*: artists block a shape from sketches, boolean it, fillet it, shell it, and iterate prototypes in minutes. ksmodeler's equivalent workflow is primitive-stack + booleans + bevel + modifiers, which is **slower and less predictable** for hard-surface:

- Missing **2D sketch tools** (draw on plane → extrude/revolve) — the single biggest gap.
- Missing **shell/offset/thicken** — essential for prop guns, rims, engine blocks (double-walled panels).
- **Fillet runs** on non-manifold or thin-bridge topology often fail or require cleanup in ksmodeler; Plasticity's solid kernel handles blend chains.
- No **dynamic subdiv preview** (solid → smooth quad mesh live) — Plasticity shows smooth proxy instantly; ksmodeler has a subdiv modifier but not tied to solid boundaries.

### 4.2 Kernel Fidelity

- ksmodeler uses CGAL boolean code. For game meshes (which tolerate small cracks) it is generally fine, and it is *more robust than 3ds Max ProBoolean* on thin plates.
- Plasticity's solid kernel keeps surfaces as B-rep solids to the end; boolean/offset operations remain exact, and history is edge-based. ksmodeler converts everything to mesh, so **offsets/shells/bridges require heroics** that a B-rep would do natively.
- Consequences: **rim/paneling workflows** (offset inner rim, split surfaces) are the highest-value gap.

### 4.3 Where the Comparison Favors ksmodeler

- Complete DCC: artists do modeling, UVs, textures, rig, and export — zero context swaps.
- **AC-native pipeline**: KN5 direct, physics mesh gen, LOD, live telemetry preview. Plasticity users must round-trip through an FBX/OBJ and a separate DCC.
- **Robust booleans on multires/point clouds**, CGAL-backed.

---

## 5. Critical Gaps (if ksmodeler wants Plasticity's niche)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | 2D sketch planes (draw + extrude/revolve/sweep) | Blocking for Plasticity-like speed | High |
| 2 | Solid shell / offset / thicken | Blocking for hard-surface double-wall parts | High |
| 3 | Production fillet chains (multi-edge, radius order, rolling ball) | Blocking for guns/rims/engine blocks | Medium |
| 4 | Dynamic smooth-preview (solid bound → quad subdiv) | High-value differentiator | Medium |
| 5 | B-rep / NURBS interchange (STEP/IGES round-trip) | Blocking for CAD-collab tracks | Very High |
| 6 | Edge-history booleans (edit source primitive, re-run) | High for non-destructive iteration | Medium |

## 6. Strategic Gaps

- **Pattern/repeat booleans with instanced edits** (hole arrays on rims/air intakes).
- **Measure/analysis overlays** (distance, radius, deviation from CAD).
- **Manifold-aware export sanity checks** mapped to AC validation rules.

---

## 7. Where ksmodeler Already Wins

- **Scope**: one tool for model → material → physics → LOD → pack.
- **AC integration**: KN5 native, collision mesh, live preview.
- **Robust boolean engine** for mesh-CSG: strong on thin game-engine geometry.
- **Geometry nodes**: Plasticity has no node-based procedural modeling; ksmodeler's 50+ node graph is a genuine differentiator for parametric/instanced asset generation (tire treads, kerbs, barriers).
- **Vulkan PBR viewport & Python scripting** — Plasticity has neither render nor real API.

---

## 8. Recommended Roadmap (Plasticity-relevant)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Sketch-to-solid** | Hard-surface speed | 2D sketch planes, extrude/revolve/lathe from sketch, sketch-on-face |
| **P2 — Solid ops** | Kernel-like ops on mesh | Shell/offset/thicken, fillet chains with radius ordering |
| **P3 — Edge history** | Non-destructive | Edge-based boolean history (re-edit source), dynamic smooth preview |
| **P4 — CAD interop** | Round-trip | STEP/IGES true import with topology, USD Layer rail |
| **P5 — Ergonomics** | Plasticity head | Packed tool palette mode, gesture/knuckle hotkeys, minimal chrome UI mode |

---

## 9. Verdict

Plasticity and ksmodeler occupy different layers; Plasticity is a **precision hard-surface hammer**, ksmodeler is a **full workshop**. For the AC modder doing rim/bodykit/rig gadgetry, Plasticity's sketch+boolean+fillet+offset loop is currently faster and higher-fidelity. For anything that must end up in a game — UVs, materials, physics, LOD, KN5 — ksmodeler already wins outright.

The strategic move is **P1–P3**: give ksmodeler sketch-to-solid + shell/offset + fillet chains. That closes the only place Plasticity provides real value to the AC community, without surrendering ksmodeler's integration dominance.

**Recommendation:** Do not chase Plasticity feature-for-feature; adopt its *workflow* (sketch-first, edit-last, minimal chrome) as a UI mode while preserving ksmodeler's full-DCC scope. Prioritize sketch tools, offset/shell, and fillet chains.