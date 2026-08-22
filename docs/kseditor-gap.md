# ksEditor - Complete Feature Gap Analysis

> **Date:** 2026-08-22
> **Version:** ksEditor v1.16.x (ksModeler + ksliveryeditor)
> **Purpose:** Unified gap analysis covering all 12 competitor comparisons - 6 for ksModeler (3ds Max, Maya, Rhino, Modo, Softimage/XSI, Plasticity), 3 for ksliveryeditor (PhotoGIMP, ZBrush, Mudbox), and 3 for ksaudioeditor (Adobe Audition, GoldWave, Sony Sound Forge). Identifies cross-cutting gaps, shared roadmaps, and the strategic positioning of ksEditor as a whole against the DCC landscape.

---

## Table of Contents

1. Executive Summary
2. ksModeler Comparisons (2.1-2.6)
3. ksliveryeditor Comparisons (3.1-3.3)
3. ksaudioeditor Comparisons (3.4-3.6)
4. Cross-Cutting Gap Analysis
5. Unified Roadmap
6. Verdict

---

## 1. Executive Summary

ksEditor consists of three primary creative modules:

- **ksModeler** - Vulkan-powered 3D modeling, sculpting, rigging, animation, UV, materials, geometry nodes, and AC pipeline integration.
- **ksliveryeditor** - Qt6-based 3D texture paint, layer compositing, material mask painting, stencil/decal projection, and DDS export for AC liveries.
- **ksaudioeditor** - Full-featured DAW with 35+ specialized panels, VST2/3 hosting, node-based audio graph, FMOD Studio 1.08.12 bank import/export, AI engine sound synthesis, and surround mixing up to 7.1.4.

### Competitor Landscape at a Glance

| Competitor | Type | ksModeler Parity | ksliveryeditor Parity | Primary Gap | ksEditor Advantage |
|------------|------|------------------|-----------------------|-------------|-------------------|
| **3ds Max** | General DCC | ~65-70% | - | Retopo, UV editor, smoothing groups | KN5 native, physics mesh, live preview |
| **Maya** | General DCC | ~45-50% | - | Deformers, NURBS, dynamics, retarget | AC pipeline, node graph, free |
| **Rhino** | NURBS modeler | ~40-45% | - | Trim/blend, 3dm, tolerance model | End-to-end AC pipeline, game UV/PBR |
| **Modo** | Modeling DCC | ~60-65% | - | Falloffs, action centers, CAGE fusion | AC-native, alive vs EOL |
| **Softimage/XSI** | Legacy DCC | ~55-60% | - | ICE compounds, expression editor | Modern renderer, AC pipeline |
| **Plasticity** | CAD hard-surface | ~50% | - | Sketch-to-solid, shell/offset | Full DCC scope, AC integration |
| **PhotoGIMP** | 2D image editor | - | ~55-60% | Filters, masks, scripting | 3D projection paint, material masks |
| **ZBrush** | Digital sculptor | - | ~15-20% | Sculpting (out of scope), alphas | Texture-map paint, AC pipeline |
| **Mudbox** | Sculpt + paint | - | ~75-80% (paint only) | Smudge/blur, alpha tips | Material masks, DDS export |
| **Adobe Audition** | Audio DAW | ~50-55% | - | Spectral editing, noise reduction | FMOD bank, node graph, AI engine sounds |
| **GoldWave** | Audio editor | ~65-70% | - | Simplicity, batch wizard | FMOD bank, VST hosting, surround, 35+ effects |
| **Sony Sound Forge** | Mastering editor | ~55-60% | - | Mastering metering, DDP, noise restoration | FMOD bank, node graph, AI engine sounds |

### Where ksEditor Already Wins Across All Comparisons

1. **AC-native pipeline** - KN5 (incl. encrypted), physics mesh, LOD, live preview, DDS export: no competitor delivers this end-to-end.
2. **Modern Vulkan PBR viewport** - shader hot-reload, real-time rendering vs legacy viewports.
3. **Free and open source (MIT)** - vs 3ds Max (,800+/yr), Maya (,870/yr), ZBrush (/mo), Mudbox (/mo), Rhino ().
4. **Node-graph procedural modeling** - 50+ geometry node types, ICE-style particles, same paradigm as Grasshopper/Bifrost.
5. **Python 3 + Lua + JS scripting** - modern ergonomics vs legacy MAXScript/MEL/ZScript.
6. **Material mask painting** - per-channel paint/carbon/chrome/matte: entirely outside every competitor's scope.
7. **Car/track/character builders + wizards** - unique domain workflows no competitor has.

---

## 2. ksModeler Comparisons

### 2.1 ksModeler vs 3ds Max

> **Overall parity:** ~65-70% of 3ds Max for AC mod workflow.

**Critical gaps:**
- Retopology / quad-draw tools
- Smoothing groups and explicit normals
- Production-quality UV editor (peel/pelt, overlap, density)
- External referencing (XRef) and robust instancing
- Advanced bevel/inset/bridge with shape options

**Where ksModeler wins:** KN5 native + encrypted, physics mesh generation, LOD generation, live preview + telemetry, Vulkan PBR viewport, car/track/character builders, modern Python scripting.

**Roadmap:** P1 Retopo + smoothing groups, P2 UV production, P3 Non-destructive stack, P4 Scripting, P5 Rendering.

---

### 2.2 ksModeler vs Maya

> **Overall parity:** ~45-50% against Maya as a production DCC.

**Critical gaps:**
- Deformer stack (lattice/cluster/nonlinear)
- Weight-paint station (mirror/solve/smooth)
- NURBS round-trip (STEP/IGES/USD exact)
- Character retarget (HumanIK-equivalent)
- Graph editor maturity
- Retopology / quad-draw

**Where ksModeler wins:** AC-native everything, car/track/character builders, Vulkan PBR, node-graph procedural modeling, Python 3, free (MIT).

**Roadmap:** P1 Deformer + weight, P2 NURBS interop, P3 Retarget, P4 Graph editor, P5 FX.

---

### 2.3 ksModeler vs Rhino

> **Overall parity:** ~40-45% of Rhino's surface-modeling scope.

**Critical gaps:**
- Trim/untrim + boundary-curve-driven surface splitting
- 3dm import (exact NURBS, mesh fallback)
- Exact STEP/BREP round-trip
- Blend/fillet/chamfer surfaces on NURBS
- Scene tolerance/unit model
- NURBS surface offset / thicken

**Where ksModeler wins:** End-to-end AC pipeline, game-side completeness (UV, materials, rig, physics), robust mesh booleans (CGAL), modern viewport, real NURBS exist now, GeometryNodes.

**Roadmap:** P1 Interop (3dm + STEP), P2 Kernel (trim/blend), P3 Blend & match, P4 Precision, P5 Parametric bridge.

---

### 2.4 ksModeler vs Modo

> **Overall parity:** ~60-65% vs Modo 17.1 (discontinued Nov 2024).

**Critical gaps:**
- Generalized modeling falloffs (soft/spatial)
- Action centers / per-op transform origins
- CAGE-style live re-edit on boolean ops
- Pure-path-traced stills + basic AOVs
- Kit/smart-preset system
- .lxo scene + preset importer

**Where ksModeler wins:** AC-native everything, robust booleans (CGAL), geometry nodes, modern viewport/renderer, Python 3, alive vs EOL.

**Roadmap:** P1 Ergonomics (falloffs, action centers), P2 Non-destructive (CAGE re-edit), P3 Shading, P4 Content (kits, .lxo), P5 Render.

---

### 2.5 ksModeler vs Softimage/XSI

> **Overall parity:** ~55-60% vs XSI 2015 (discontinued 2015).

**Critical gaps:**
- ICE compound library system (save/share/version node-trees)
- Expression editor + wire-parameter correlation UI
- Interactive modeling-stack sub-object editing
- Strand/hair-from-ICE production quality
- Fluid simulation (ICE Fluid)
- Production renderer

**Where ksModeler wins:** Full AC pipeline, Vulkan PBR, CGAL booleans, Python 3, actively developed, MIT licensed.

**Roadmap:** P1 Compound economy, P2 Expressions, P3 Stack interactivity, P4 Data rescue (.scn/.exp/.emdl), P5 Strands and fluid.

---

### 2.6 ksModeler vs Plasticity

> **Overall parity:** ~50% in Plasticity's niche; ~200%+ everywhere else.

**Critical gaps:**
- 2D sketch planes (draw + extrude/revolve/sweep)
- Solid shell / offset / thicken
- Production fillet chains (multi-edge, radius order, rolling ball)
- Dynamic smooth-preview (solid bound to quad subdiv)
- B-rep / NURBS interchange (STEP/IGES round-trip)
- Edge-history booleans (edit source primitive, re-run)

**Where ksModeler wins:** Scope (full DCC), AC integration, robust booleans on mesh, geometry nodes, Vulkan PBR, Python scripting.

**Roadmap:** P1 Sketch-to-solid, P2 Solid ops (shell/offset/fillet), P3 Edge history, P4 CAD interop, P5 Ergonomics.

---

## 3. ksliveryeditor Comparisons

### 3.1 ksliveryeditor vs PhotoGIMP

> **Overall parity:** ~55-60% of PhotoGIMP's 2D image editing scope; 100% unique in 3D paint + AC pipeline.

**Critical gaps:**
- Layer masks (grayscale/alpha/selection)
- Curves / levels adjustment
- Selection refinement (grow/shrink/feather, color range)
- Scripting surface (Python bindings to paint canvas)
- Tablet/pressure sensitivity

**Where ksliveryeditor wins:** 3D projection paint on car model, material mask painting, stencil/decal projection, live PBR 3D viewport, DDS export with mip-chains, AC template system.

**Roadmap:** P1 Selection and mask, P2 Adjustments, P3 Brush dynamics, P4 Scripting, P5 Interop (GIMP handoff).

---

### 3.2 ksliveryeditor vs ZBrush

> **Overall parity:** ~15-20% vs ZBrush (sculpting is out of scope); 100% unique in game texture paint + AC pipeline.

**Critical gaps:**
- Alpha brush tips (grayscale brush textures)
- Stroke types (DragRect, DragDot, Spray)
- Texture-on-brush (brush pattern overlay)
- Tablet/pressure sensitivity

**Where ksliveryeditor wins:** Texture-map painting via UV projection, material mask painting, stencil/decal projection, DDS export, KN5/AC pipeline, layer-based compositing, PBR viewport, AC template system, free (MIT).

**Roadmap:** P1 Alpha brushes, P2 Tablet input, P3 Weathering tools, P4 Displacement/normal import, P5 Brush library.

---

### 3.3 ksliveryeditor vs Mudbox

> **Overall parity:** ~75-80% of Mudbox's paint-only capabilities; 100% unique in material masks + AC pipeline.

**Critical gaps:**
- Smudge / blur brush
- Alpha brush tips (grayscale brush textures)
- Stencil surface-aware wrap
- Tablet/pressure sensitivity
- Visibility painting (hide faces while painting)

**Where ksliveryeditor wins:** Material mask painting, DDS export with mip-chains, KN5/AC pipeline, layer groups, GIMP-style 2D canvas, AC template system, PBR viewport (game-shader-accurate), free (MIT), integrated into ksEditor.

**Note:** ksModeler already implements Mudbox-style multiresolution sculpting and sculpt layers (3DModelingQmlBridge.h:503-527).

**Roadmap:** P1 Brush depth (smudge, alphas), P2 Tablet input, P3 Stencil library, P4 Brush presets, P5 Interop bridge.

---

## 3.4 ksaudioeditor vs Adobe Audition

> **Overall parity:** ~50-55% of Adobe Audition as a general-purpose audio editor; 100% unique in game audio pipeline + FMOD integration.

**Adobe Audition** (Adobe, since 2003, formerly Cool Edit Pro) is the industry-standard *general-purpose DAW/audio editor*: multi-track recording/mixing, spectral frequency display, comprehensive effects rack (EQ, compression, reverb, noise reduction), batch processing, CD mastering, and tight integration with Premiere Pro for video post-production. It is the reference for podcast, broadcast, and general audio editing workflows.

**ksaudioeditor** is a game-audio-focused DAW: multi-track timeline, 35+ specialized panels (sidechain compressor, multiband compressor, transient designer, convolution reverb, tape emulator, guitar amp sim, vocal processor, harmonic generator, stereo enhancer), VST2/3 hosting, node-based audio graph, surround mixing up to 7.1.4, and full FMOD Studio 1.08.12 bank import/export. Its strength is game-deployable audio, not general audio editing.

**Critical gaps:**
- Spectral frequency display / spectral editing (Audition's signature feature)
- Noise reduction / restoration tools (adaptive noise reduction, sound removal, de-hum)
- Podcast/broadcast mastering chain (loudness normalization to specific standards)
- Premiere Pro integration (video timeline sync)
- CD mastering / DDP export
- Multitrack session templates

**Where ksaudioeditor wins:**
- **FMOD Studio bank import/export** - direct game audio middleware integration; Audition has none
- **Node-based audio graph** - visual signal flow for complex routing; Audition is rack-only
- **AI-assisted engine sound synthesis** - granular, sample-based, RPM-driven layering; entirely outside Audition's scope
- **Surround mixing 7.1.4** - Audition supports 5.1 only
- **Game-specific analysis** - loudness metering (EBU R128, ATSC A/85), true peak detection, phase correlation
- **Batch processing** - effect chains, format conversion, loudness normalization, dithering
- **VST2/3 hosting with parameter automation** - both have VST; ksEditor's is game-integrated
- **Free (MIT)** vs Audition ($22.99/mo Adobe CC)

**Roadmap:** P1 Spectral display, P2 Noise reduction, P3 Session templates, P4 Video sync, P5 Batch export presets.

---

## 3.5 ksaudioeditor vs GoldWave

> **Overall parity:** ~65-70% of GoldWave as a lightweight audio editor; 100% unique in game audio pipeline + FMOD integration.

**GoldWave** (GoldWave Inc., since 1993) is a *lightweight, affordable audio editor*: basic multi-track, effects (EQ, compression, reverb, noise reduction), batch processing, format conversion, and simple recording. It targets hobbyists, podcasters, and small-studio users who need a simple, fast audio editor without DAW complexity.

**ksaudioeditor** is significantly more powerful in DSP (35+ effects, VST hosting, node graph, surround) but lacks GoldWave's simplicity and some general-purpose features.

**Critical gaps:**
- Simplicity / ease of use (GoldWave's entire UX is simpler)
- Batch format conversion with simple wizard
- Voice activation / silence detection
- Audio restoration (noise gate, pop/click removal)
- Direct support for more consumer formats (WMA, AAC)

**Where ksaudioeditor wins:**
- **FMOD Studio bank import/export** - game audio pipeline
- **Node-based audio graph** - complex routing
- **35+ built-in effects** vs GoldWave's ~15
- **VST2/3 hosting** - extensible plugin ecosystem
- **Surround mixing 7.1.4** - GoldWave is stereo only
- **AI engine sound synthesis** - RPM-driven layering
- **Multi-channel recording** with punch-in/out, loop recording, take management
- **Spectral analysis** (FFT, 1/3 octave, oscilloscope, phase scope)
- **Loudness metering** (EBU R128, ATSC A/85)
- **Free (MIT)** vs GoldWave ($45 one-time)

**Roadmap:** P1 Simple batch wizard, P2 Voice activation, P3 Audio restoration, P4 Consumer format support, P5 Simplified mode/UX.

---

## 3.6 ksaudioeditor vs Sony Sound Forge

> **Overall parity:** ~55-60% of Sound Forge as a mastering/editor DAW; 100% unique in game audio pipeline + FMOD integration.

**Sony Sound Forge** (now Magix Sound Forge, since 1999) is a *professional audio editor/mastering tool*: high-precision waveform editing, comprehensive effects (EQ, compression, reverb, noise restoration), master bus processing, batch conversion, DDP export for CD mastering, and VST plugin support. It targets mastering engineers, broadcast professionals, and post-production audio editors. Known for its precise sample-level editing and mastering-grade metering.

**ksaudioeditor** is game-audio-focused and lacks mastering-specific features, but surpasses Sound Forge in DSP depth and game integration.

**Critical gaps:**
- Mastering-grade metering (LUFS histograms, loudness range over time, ITU-R BS.1770)
- Sample-level precise editing (pencil tool, zero-crossing snap)
- DDP export for CD mastering
- Noise restoration (adaptive noise reduction, spectral repair)
- Mastering presets / chains (vinyl, tape, broadcast, streaming)
- VST3 plugin hosting (Sound Forge has VST2/3 since Magix era)
- Time-stretch / pitch-shift quality (Sound Forge's elastique engine)
- Video timeline sync for post-production

**Where ksaudioeditor wins:**
- **FMOD Studio bank import/export** - game audio middleware integration
- **Node-based audio graph** - visual signal flow vs rack-only
- **35+ built-in effects** vs Sound Forge's ~20
- **AI engine sound synthesis** - granular, sample-based, RPM-driven layering
- **Surround mixing 7.1.4** - Sound Forge is stereo/5.1
- **Multi-track recording** with punch-in/out, loop, take management
- **Batch processing** - effect chains, format conversion, loudness normalization
- **Loudness metering** (EBU R128, ATSC A/85, true peak) - comparable to Sound Forge
- **Spectral analysis** (FFT, 1/3 octave, oscilloscope, phase scope)
- **Free (MIT)** vs Sound Forge ($60-$400 depending on edition)

**Roadmap:** P1 Mastering metering (LUFS histograms, BS.1770), P2 Sample-level editing, P3 Noise restoration, P4 DDP export, P5 Mastering presets.

---

## 4. Cross-Cutting Gap Analysis

### 4.1 Gaps That Appear Across Multiple Comparisons

| Gap | 3ds Max | Maya | Rhino | Modo | XSI | Plasticity | PhotoGIMP | ZBrush | Mudbox | Priority |
|-----|---------|------|-------|------|-----|------------|-----------|--------|--------|----------|
| **Retopology / quad-draw** | X | X | - | - | - | - | - | - | - | High |
| **Smoothing groups / explicit normals** | X | - | - | - | - | - | - | - | - | High |
| **UV editor depth** | X | X | - | X | - | - | - | - | - | High |
| **Tablet / pressure sensitivity** | - | - | - | - | - | - | X | X | X | High |
| **Alpha brush tips** | - | - | - | - | - | - | - | X | X | Medium |
| **Production renderer** | X | X | X | X | X | - | - | - | - | Medium |
| **Scripting breadth** | X | X | - | - | X | - | X | - | X | Medium |
| **Non-destructive stack depth** | X | - | - | X | X | - | X | - | - | Medium |
| **NURBS / CAD kernel** | - | X | X | - | - | X | - | - | - | High (Rhino) |
| **Deformer stack** | - | X | - | - | - | - | - | - | - | High (Maya) |
| **Falloffs / action centers** | - | - | - | X | - | - | - | - | - | High (Modo) |
| **ICE compound library** | - | - | - | - | X | - | - | - | - | High (XSI) |
| **Sketch-to-solid** | - | - | - | - | - | X | - | - | - | Medium |
| **Layer masks** | - | - | - | - | - | - | X | - | - | Medium |
| **Spectral editing** | - | - | - | - | - | - | - | - | - | Medium (audio) |
| **Noise reduction / restoration** | - | - | - | - | - | - | - | - | - | Medium (audio) |
| **Mastering metering (LUFS histograms)** | - | - | - | - | - | - | - | - | - | Low (audio) |

### 4.2 Universal Gaps (Affect the Entire ksEditor Product)

| Gap | Impact | Affected Modules |
|-----|--------|-----------------|
| **Tablet/pressure sensitivity** | Natural painting feel for both sculpt and paint | ksliveryeditor, ksmodeler sculpt |
| **Production renderer** | Final-quality stills for marketing/showroom | ksmodeler |
| **Retopology** | Mesh cleanup after boolean/sculpt operations | ksmodeler |
| **Smoothing groups** | Automotive panel-line quality | ksmodeler |
| **UV editor depth** | Multi-texture car interior quality | ksmodeler |
| **Scripting breadth** | Pipeline automation, batch processing | Both modules |
| **Spectral editing** | Audio restoration and precision editing | ksaudioeditor |
| **Noise reduction** | Clean-up of recorded audio samples | ksaudioeditor |

### 4.3 What No Competitor Can Match

These are ksEditor's decisive advantages that persist across every comparison:

| Feature | Why It Matters |
|---------|---------------|
| **KN5 native + encrypted export** | Direct game integration; every competitor needs exporters |
| **Physics mesh generation** | Convex hull, VHACD decomposition: native, no scripts |
| **LOD generation + validation** | Quadric error decimation with AC-specific rules |
| **Live AC preview + telemetry** | Iterate car in-game without leaving the editor |
| **Material mask painting** | Per-channel paint/carbon/chrome/matte: unique to AC |
| **DDS export with mip-chains** | Game-ready texture output |
| **Car/track/character builders** | Wizard-driven domain workflows |
| **Node-graph procedural modeling** | 50+ geometry node types |
| **ICE-style particle system** | Node-graph dataflow, same paradigm as Bifrost |
| **Free and open source (MIT)** | No subscription, no license management |

---

## 5. Unified Roadmap

### 5.1 Phase 1 - High-Impact, Cross-Cutting (0-6 months)

| Item | Affects | Effort | Impact |
|------|---------|--------|--------|
| Tablet/pressure sensitivity (Windows Ink) | ksliveryeditor, ksmodeler sculpt | High | High - natural feel for both paint and sculpt |
| Retopology / quad-draw | ksmodeler | High | High - blocks 3ds Max, Maya migration |
| Smoothing groups / explicit normals | ksmodeler | Medium | High - blocks automotive panel-line work |
| Layer masks (grayscale/alpha) | ksliveryeditor | Medium | High - compositing depth for liveries |
| Spectral frequency display | ksaudioeditor | Medium | Medium - audio restoration and editing |
| Noise reduction / restoration | ksaudioeditor | Medium | Medium - clean-up of recorded samples |

### 5.2 Phase 2 - Competitor-Specific Wins (6-12 months)

| Item | Target Competitor | Effort | Impact |
|------|-------------------|--------|--------|
| Modeling falloffs + action centers | Modo refugees | Medium | High - signature Modo workflow |
| ICE compound library system | XSI refugees | Medium | High - restores XSI's killer feature |
| Deformer stack (lattice/cluster/nonlinear) | Maya users | Medium | High - animation/rigging depth |
| 3dm import (exact NURBS + mesh fallback) | Rhino users | High | High - unblocks Rhino asset pipeline |
| Alpha brush tips + smudge/blur brush | ZBrush, Mudbox users | Low-Medium | Medium - paint quality |
| 2D sketch planes + shell/offset | Plasticity users | High | Medium - hard-surface speed |
| CAGE-style re-edit on boolean stack | Modo users | Medium | Medium - MeshFusion approximation |

### 5.3 Phase 3 - Depth and Polish (12-18 months)

| Item | Target Competitor | Effort | Impact |
|------|-------------------|--------|--------|
| Exact STEP/BREP round-trip | Rhino, Plasticity | Very High | High - class-A surface survival |
| Curves/levels adjustment layers | PhotoGIMP users | Medium | Medium - photo retouching |
| Expression editor + wire-parameter UI | XSI users | Medium | Medium - rigging depth |
| Production renderer + AOVs | 3ds Max, Maya, Modo | High | Medium - marketing stills |
| Bifrost-style fluid/smoke | Maya users | Very High | Medium - wet-weather FX |
| Brush dynamics engine | ZBrush, Mudbox | High | Medium - natural painting |
| Mastering metering (LUFS histograms, BS.1770) | Sound Forge users | Medium | Low - mastering workflow |
| Session templates + batch wizard | GoldWave users | Low | Low - simplicity and speed |
| DDP export for CD mastering | Sound Forge users | Medium | Low - mastering niche |

### 5.4 Phase 4 - Ecosystem and Interop (18+ months)

| Item | Target Competitor | Effort | Impact |
|------|-------------------|--------|--------|
| .lxo importer (Modo rescue) | Modo refugees | High | Goodwill |
| .scn/.exp/.emdl importers (XSI rescue) | XSI refugees | High | Goodwill |
| Grasshopper graph import | Rhino users | Very High | Parametric bridge |
| Kit/preset system | Modo users | Medium | Content ecosystem |
| USD production pipeline | All DCC users | High | Interoperability |

---

## 6. Verdict

ksEditor's competitive position is **unique in the DCC landscape**: it is the only tool that combines full 3D modeling/sculpting, 3D texture painting, and direct game-engine integration in a single free, open-source application.

### The Strategic Picture

ksModeler is **not trying to beat** 3ds Max, Maya, Rhino, Modo, XSI, or Plasticity at their own game. It is building a new category: **the AC-native DCC** - a tool where the distance from "first polygon" to "in-game car" is measured in minutes, not days. Every competitor comparison confirms this: the gap clusters are always in general-purpose DCC depth (retopo, deformers, NURBS, render), never in AC pipeline integration (KN5, physics, LOD, live preview, DDS).

ksliveryeditor is **not trying to beat** PhotoGIMP, ZBrush, or Mudbox at general image editing or sculpting. It is building a specialized 3D paint station where the distance from "design concept" to "game-ready livery texture" is zero context switches. The material mask painting system (paint/carbon/chrome/matte) is a feature that exists in no other tool on the market.

### The Migration Opportunity

Three competitors are dead or dying (Modo: EOL Nov 2024; XSI: EOL 2015; Softimage community still displaced). Two are overpriced for individual modders (3ds Max: ,800+/yr; Maya: ,870/yr). One is niche and overkill for AC work (ZBrush: sculpting, not painting). The migration window is open, and ksEditor's combination of **free + AC-native + modern stack** is the strongest value proposition in the sim-racing content space.

### Priority Actions

1. **Tablet pressure sensitivity** - the single highest-impact feature that affects both modules and every paint/sculpt comparison.
2. **Retopology + smoothing groups** - closes the gap with 3ds Max and Maya, the two most-used external tools in AC modding.
3. **3dm import** - unlocks the entire Rhino asset base for the AC track pipeline.
4. **ICE compound library** - restores XSI's most missed feature and attracts the displaced XSI community.
5. **Modeling falloffs + action centers** - absorbs Modo's ergonomic DNA and attracts displaced Modo artists.

ksEditor does not need to be everything to everyone. It needs to be **the best tool for making AC content**, and on that metric, it already wins across every comparison.

---

*ksEditor v1.16.x - Complete Feature Gap Analysis. Generated 2026-08-22.*
