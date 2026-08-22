# ksmodeler vs Autodesk Softimage (XSI) — Feature Gap Analysis

> **Comparer:** ksmodeler (ksEditor 3D Modeler module) vs Autodesk Softimage 2015 (final release; discontinued 2015-04)
> **Date:** 2026-08-20
> **Version:** ksmodeler v1.16.x
> **Purpose:** ksmodeler's architecture (ICEParticleSystem, WireParameterSystem, ConstraintSystem, FCurveSystem, non-linear modeling stack) is visibly XSI-inspired. This analysis measures how far ksmodeler carries the XSI legacy and where it still falls short.

---

## 1. Executive Summary

**Softimage | XSI** was the industry's most underrated DCC — famous for **ICE** (the node-based particle/simulation environment), the best keyframing UI of its era, true non-linear modeling, and an elegant constraint/expression system (wire parameters). Autodesk killed it in 2015, leaving a loyal user base (including many racing-game modelers) orphaned.

**ksmodeler** has deliberately revived the XSI mental model:

| XSI legacy feature | ksmodeler equivalent |
|--------------------|----------------------|
| ICE (Interactive Creative Environment) | `ICEParticleSystem` + `ParticleInstancing` + node graph |
| Wire parameters / expressions | `WireParameterSystem` |
| Constraint system | `ConstraintSystem` |
| F-Curve editor | `FCurveSystem` |
| Model/stack layers, non-linear modeling | `ModifierStack` + `BooleanStack` |
| Symmetry | `SymmetryQmlBridge` |

That shared DNA makes XSI the **most relevant historical comparison**: ksmodeler is essentially a modern re-implementation of XSI's conceptual core, minus the full studio-proven maturity.

**Overall parity:** ~55–60% vs XSI 2015 as a *studio DCC*. But XSI is dead — no autodesk support, no new features, aging against modern pipelines. For an XSI refugee, ksmodeler already restores most of the muse.

---

## 2. Context & Methodology

Inventory from `src/modules/modellingEditor/` and `src/core/` vs Softimage 2015 SDK/feature documentation and the XSI community (ICE users, riggers). Feature ratings account for the fact that XSI is frozen software: ksmodeler only needs to match *capability*, not exceed it, to be the superior choice today.

---

## 3. Feature-by-Feature Matrix

| Capability | ksmodeler | XSI 2015 | Notes |
|------------|-----------|----------|-------|
| **Core Modeling** | | | |
| Poly modeling | 🟡 | ✅ | XSI's modeling stack + live stack preview deep; ksmodeler basic |
| N-gon handling | 🟡 | ✅ | XSI excellent n-gon tolerance + conversion tools |
| Boolean ops | ✅ | 🟡 | ksmodeler CGAL more robust than XSI boolean on thin shells |
| Subdivision surfaces | ✅ | ✅ | XSI had best-in-class OpenSubdiv preview; ksmodeler Catmull-Clark/Loop OK |
| Retopology (relax/decimate/quad) | 🔴 | 🟡 | XSI had Retopo + relax; ksmodeler missing |
| **Non-Linear & Parametric** | | | |
| Modifier/op stack with live preview | 🟡 | ✅ | XSI's Modeling Op stack was the reference; ksmodeler has stack but weaker interactivity |
| Boolean stack re-edit | 🟡 | ✅ | BooleanStack exists; XSI re-edit of ops mature |
| ICE (particles + simulation) | 🟡 | ✅ | ksmodeler ICEParticleSystem exists; XSI ICE was *the* reference (compounds, fluids, strands) |
| ICE compounds library | 🔴 | ✅ | XSI had thousands of user compounds; ksmodeler has node graph |
| Wire parameters / expressions | 🟡 | ✅ | WireParameterSystem exists; XSI wire params + expression editor more expressive (annuals) |
| **Rigging & Animation** | | | |
| Skeletons / rig gen | ✅ | 🟡 | ksmodeler humanoid/quadruped gen; XSI manual but powerful |
| Constraints | 🟡 | ✅ | ConstraintSystem basic; XSI constraint stack rich |
| F-Curve editor | 🟡 | ✅ | FCurveSystem basic vs XSI's legendary Anim editor |
| Shape keys / morphology | ✅ | ✅ | ksmodeler MorphTargetEditor; XSI shape manager |
| **Simulation & Dynamics** | | | |
| Cloth | 🟡 | ✅ | ksmodeler ClothSystem; XSI Syflex-excellent cloth |
| Hair / fur | 🟡 | ✅ | ksmodeler HairSystem; XSI Hair + ICE strands stronger |
| Rigid body | 🟡 | ✅ | ksmodeler Bullet-based; XSI Reactor/PhysX weaker — ksmodeler may exceed |
| **Viewport & Render** | | | |
| Real-time viewport | ✅ | 🟡 | ksmodeler Vulkan PBR modern; XSI OpenGL viewport dated |
| Renderer | 🔴 | ✅ | XSI bundled mental ray (production); ksmodeler experimental raytracer only |
| **Scripting** | | | |
| Scripting languages | ✅ | 🟡 | ksmodeler Python/Lua/JS; XSI was PHP/Perl-ecosystem, legacy |
| Python SDK | 🟡 | ✅ | ksmodeler pybind11 active; XSI Python was best-of-era but dead |
| **Pipeline** | | | |
| AC/KN5 pipeline | ✅ | 🔴 | ksmodeler native; XSI needs exporters, none maintained |
| FBX / OBJ / GLB | ✅ | 🟡 | ksmodeler bidirectional GLB; XSI FBX via Autodesk interop (dead in 2026) |
| USD | 🟡 | 🔴 | ksmodeler ASCII USD; XSI none |
| Live game preview / telemetry | ✅ | 🔴 | ksmodeler native; XSI none |

---

## 4. Category Gap Analysis

### 4.1 ICE vs ksmodeler's Particle System

ICE was the crown jewel. XSI let artists visually wire particle **attributes, forces, collisions, strands, fluid, rigid** into a live graph with thousands of prebuilt **compounds**. ksmodeler has `ICEParticleSystem` + `ParticleInstancing` + a node graph editor, which is the right skeleton, but:

- **No compound library system** (packaged, shareable, versioned ICE-tree units).
- **No strand/hair rendering from ICE** at production quality.
- **No fluid adjacency** (`ParticlePointsGeometry` vs ICE Fluid).
- **No in-viewport node-graph debug on live particles** (ICE showed per-particle values in viewport).

Restoring a *compound economy* — let users write, save and share ICE-style compounds — would be the single most XSI-legacy feature ksmodeler could ship, and it compounds the modding value (grass, rain, sparks, smoke on tracks).

### 4.2 Wire Parameters & Expressions

- ksmodeler `WireParameterSystem` exists; XSI's wire map had multi-object wiring, expression trees, and cross-scene references.
- Gap: **expression editor with annual lookup tables** and **parameter correlation UI** (link sliders, remap curves).
- This matters for riggers who use XSI-style proxy-car kinematics (suspension travel → ride height, camber → tire IR).

### 4.3 Non-Linear Modeling Stack

- XSI's **Modeling Op layers** (click an op in the stack → draw/edit on original geometry → the op re-runs) were the standard.
- ksmodeler has a ModifierStack, but interactive stack editing (nested sub-object on live ops, stack re-parameter sweeps) is thinner. This is the same gap identified vs 3ds Max — it applies doubly here because XSI modellers *lived* in the stack.

### 4.4 Animation

- XSI's **Anim editor (F-curve)** was considered the cleanest curve editor in the industry, with ghosting, element channels and no-clutter curves.
- ksmodeler's FCurveSystem is serviceable for keyframe object animation but lacks graph-editor polish. For sim content, animation is mostly suspension/wheel rigs and trackside animatrons — a moderate but reachable bar.

### 4.5 What ksmodeler Exceeds

- **Modern renderer/viewport** — Vulkan PBR, shader hot-reload, SSR/PCSS path.
- **Kernel-grade booleans** (CGAL) — XSI booleans were weak on thin plates.
- **Scripting** — Python 3/pybind11 vs XSI's legacy unlock.
- **USD/GLB** modern interchange.
- **Full AC pipeline** — KN5, physics, LOD, live preview: XSI can't even open a KN5.
- **Maintenance** — XSI is frozen in 2015; ksmodeler moves.

---

## 5. Critical Gaps (vs XSI's legacy features — worth reviving)

| # | Gap | Impact | Effort |
|---|-----|--------|--------|
| 1 | ICE compound library system (save/share/version node-trees) | High — restores XSI's killer differentiator | Medium |
| 2 | Expression editor + wire-parameter correlation UI | High for riggers/proxy-car kinematics | Medium |
| 3 | Interactive modeling-stack sub-object editing | High for non-linear workflow | High |
| 4 | Strand/hair-from-ICE production quality | Medium — hair/grass on tracks | High |
| 5 | Fluid simulation (ICE Fluid) | Medium — sand/water/debris FX | Very High |
| 6 | Production renderer for XSI-style final stills | Medium | Very High |

## 6. Strategic Gaps

- **Per-object attribute channels** (like XSI's `pxr:attributes`) mapped to AC physics/AI channels.
- **Import legacy XSI files** (`.scn`/`.exp`/`.emdl`) for content rescue — huge goodwill win with XSI veterans.
- **ICE debug viewport overlay** — visualize per-point attributes live.

---

## 7. Where ksmodeler Already Wins (vs XSI)

- Full AC content pipeline (KN5, physics, LOD, live preview, pack) — completely unparalleled.
- Vulkan PBR viewport + hot-reload shaders.
- CGAL booleans beat XSI boolean on real game geometry.
- Python 3 + Node graph editor modern combination.
- Actively developed, cross-platform, MIT licensed.

---

## 8. Recommended Roadmap (XSI-legacy priority)

| Phase | Focus | Items |
|-------|-------|-------|
| **P1 — Compound economy** | ICE revival | Compound packaging + library, node-graph snippet sharing, compound docs |
| **P2 — Expressions** | Wire parameters | Expression editor, multi-object wiring, correlation/remap UI |
| **P3 — Stack interactivity** | Non-linear | Live sub-object editing on applied ops, op re-parameter traces |
| **P4 — Data rescue** | Migration | `.scn/.exp/.emdl` importers, XSI character/track converter |
| **P5 — Strands & fluid** | MCG-level sims | ICE strands for hair/grass, basic fluid volume |

---

## 9. Verdict

Softimage users are the most underserved community in DCC — their tool was sunset, and no modern tool speaks their language. **ksmodeler already speaks it** (ICE, wire params, constraints, F-curves, stack modeling) and pairs it with a modern renderer and the AC pipeline. The gap to XSI parity is real but narrow and *high-leverage*: it consists mostly of XSI's interaction depth (compound library, expression editor, live stack editing) rather than missing frameworks.

**Recommendation:** Position ksmodeler explicitly as *the spiritual successor to Softimage for sim-racing content* (document the XSI lineage in marketing docs). Prioritize the ICE compound library + expression editor + live stack editing — three features that would break the XSI-veteran reluctance to migrate and that no other modern tool offers either.