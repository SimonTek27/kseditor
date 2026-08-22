# ksModeler + ksliveryeditor - Integration Roadmap

> **Date:** 2026-08-22
> **Version:** ksModeler + ksliveryeditor v1.16.x
> **Purpose:** Define a concrete plan to make ksModeler (3D modeling/sculpting module) and ksliveryeditor (paint/livery module) feel like one unified tool - seamless mesh-to-paint handoff, shared viewport, bidirectional data flow, and a single workflow from sculpt to livery to DDS export.

---

## 1. Current State - What Exists Today

### 1.1 Existing Connection Points

The two modules currently communicate through a **single-direction call chain** in CarEditor:

    CarEditor (modellingEditor)
      -> PaintEditor::instance()->setCarPath(path)
      -> PaintEditor::instance()->saveCurrentSkin()
      -> PaintEditor::instance()->loadPaintTexture(skinPath)
      -> PaintEditor::instance()->getSkinNames()
      -> PaintEditor::instance()->setCurrentSkin(skinName)

This is implemented in src/modules/modellingEditor/CarBuilder/CarEditor.cpp:427-468. The connection is **one-way** (CarEditor calls PaintEditor) and **synchronous** (no signals for cross-module event propagation).

### 1.2 What Is Missing

| Gap | Impact | Current Workaround |
|-----|--------|-------------------|
| No bidirectional signals between modules | Sculpt changes don't notify paint; paint changes don't update modeler viewport | Manual reload |
| No shared mesh reference | PaintEditor loads its own mesh copy; modeler has a separate copy | Duplicate memory, stale data |
| No paint mode toggle in ksModeler viewport | User must switch modules entirely to paint | Context switch, lost viewport state |
| No projection paint from ksModeler sculpt brushes | Sculpt layers and paint layers are independent stacks | Separate workflows |
| No real-time paint-on-sculpt preview | Cannot paint while sculpting multires levels | Two-pass workflow |
| No shared undo/redo history | Sculpt undo and paint undo are independent | Confusing state on cross-module operations |
| No paint-aware mesh export from ksModeler | Export KN5 without livery textures unless manually saved | Manual save before export |
| No sculpt-aware paint brush | Paint brushes don't respond to mesh curvature/normal from sculpt data | Flat painting on complex surfaces |

---

## 2. Integration Architecture - The Target

### 2.1 Shared Core: MeshPaintBridge

A new bridge class that both modules reference, holding the **single source of truth** for the car's mesh and texture state.

    MeshPaintBridge
           |
     +-----+-----------------+
     |                       |
  SceneMesh*             QImage*
  (shared ptr)          (shared tex)
     |                       |
  meshModified()       textureModified()
  topologyChanged()    skinChanged()
     |                       |
  +--v----------+    +------v--------+
  | ksModeler   |    | ksliveryeditor|
  | (sculpt/    |    | (paint)       |
  |  model)     |    |               |
  +-------------+    +---------------+

**Key principle:** Both modules operate on the same SceneMesh and QImage instances, not copies. Mutations propagate via signals.

### 2.2 Shared Data Model

New file: src/core/bridge/MeshPaintBridge.h

    class MeshPaintBridge : public QObject {
        Q_OBJECT
    public:
        static MeshPaintBridge* instance();

        // Mesh (single source of truth)
        SceneMesh* mesh() const;
        void setMesh(SceneMesh* mesh);

        // Texture (single source of truth)
        QImage* texture() const;
        void setTexture(const QImage& tex);

        // Skin management
        QString currentSkin() const;
        QStringList skinNames() const;
        void setCurrentSkin(const QString& name);

        // Projection paint interface (called by paint, reads from mesh)
        QVariantMap uvAt(float worldX, float worldY, float worldZ) const;
        QVector3D normalAt(float worldX, float worldY, float worldZ) const;
        float curvatureAt(float worldX, float worldY, float worldZ) const;

        // Sculpt paint interface (called by sculpt, reads from texture)
        QColor sampleTexture(float u, float v) const;
        void applyProjectionPaint(const QVector3D& hitPoint,
                                  const QColor& color,
                                  float radius, float strength);

    signals:
        void meshModified();
        void topologyChanged();
        void textureModified();
        void skinChanged(const QString& skinName);
        void projectionNeedsUpdate();
        void materialMaskChanged(int channel);
    };

---

## 3. Concrete Integration Features

### 3.1 Paint Mode in ksModeler Viewport

**Goal:** User presses P in the ksModeler 3D viewport to enter paint mode without switching modules.

**Implementation:**
- Viewport3DSystem gains a paintMode state toggle.
- When active, mouse events route through MeshPaintBridge::applyProjectionPaint() instead of sculpt/edit tools.
- The toolbar swaps to show paint tools (brush size, hardness, opacity, color) in a floating overlay.
- The layer panel from ksliveryeditor docks inside ksModeler's splitter when paint mode is active.
- Pressing Esc or Tab returns to sculpt/edit mode.

**Files affected:**
- src/core/mesh/Viewport3DSystem.h/cpp - paint mode state, input routing
- src/modules/modellingEditor/3DModelingQmlBridge.h/cpp - paint mode toggle, paint brush calls
- src/modules/modellingEditor/3DModeling_panels.cpp - paint toolbar overlay

### 3.2 Sculpt-Aware Paint Brushes

**Goal:** Paint brushes respond to mesh curvature and surface normals from the sculpt data.

**Implementation:**
- MeshPaintBridge::normalAt() and MeshPaintBridge::curvatureAt() sample the current sculpt mesh at the hit point.
- Paint brush strength is modulated by surface curvature (flatter areas get more paint, crevices get less - or inverse, configurable).
- Brush falloff follows surface curvature instead of screen-space radius, giving more natural paint on complex geometry.
- The paint brush cursor in the viewport shows curvature-aware size preview.

**Files affected:**
- src/core/bridge/MeshPaintBridge.h/cpp - curvature/normal sampling
- src/core/paint/PaintPainter.h/cpp - curvature-modulated brush
- src/modules/PaintEditor/PaintViewport.cpp - curvature-aware cursor

### 3.3 Live Texture Preview in ksModeler

**Goal:** When painting in ksliveryeditor, the ksModeler viewport (if open) updates in real-time.

**Implementation:**
- MeshPaintBridge::textureModified() signal connected to Viewport3DSystem::updatePaintTexture().
- The ksModeler viewport re-applies the paint texture to the car mesh material on each paint stroke (debounced at 60fps).
- No mesh reload required - texture is a GPU uniform update only.

**Files affected:**
- src/core/mesh/Viewport3DSystem.h/cpp - texture uniform update on signal
- src/core/bridge/MeshPaintBridge.cpp - signal emission from paint operations

### 3.4 Bidirectional Skin/Sculpt Sync

**Goal:** Changes in one module reflect in the other without manual reload.

**Implementation:**
- When ksModeler modifies mesh topology (boolean, remesh, subdivide), MeshPaintBridge::topologyChanged() fires.
- ksliveryeditor receives the signal, invalidates its UV projection cache, and re-projects existing paint onto the new topology (best-effort UV transfer).
- When ksliveryeditor changes skin, MeshPaintBridge::skinChanged() fires.
- ksModeler updates the viewport material to show the new skin texture.

**Files affected:**
- src/core/bridge/MeshPaintBridge.h/cpp - bidirectional signals
- src/modules/PaintEditor/PaintEditorModule.h/cpp - skin change signal propagation
- src/modules/modellingEditor/3DModelingQmlBridge.h/cpp - topology change signal propagation

### 3.5 Unified Export Pipeline

**Goal:** Export KN5 from ksModeler with livery textures automatically included.

**Implementation:**
- Before KN5 export, CarEditor::exportToAC() checks MeshPaintBridge::instance()->texture() for unsaved paint.
- If paint is dirty, prompt to save (or auto-save to current skin).
- The KN5 exporter reads the paint texture from MeshPaintBridge instead of loading from disk.
- DDS export from ksliveryeditor also reads from MeshPaintBridge, eliminating the file round-trip.

**Files affected:**
- src/modules/modellingEditor/CarBuilder/CarEditor.cpp - export pipeline integration
- src/modules/PaintEditor/PaintEditorModule.h/cpp - export reads from bridge

### 3.6 Shared Undo/Redo for Cross-Module Operations

**Goal:** Operations that span sculpt and paint (e.g., sculpt then paint the result) have unified undo.

**Implementation:**
- MeshPaintBridge holds a unified undo stack that wraps both sculpt and paint operations.
- Each undo entry records which module produced it and the before/after state of both mesh and texture.
- Undo from sculpt reverts geometry; undo from paint reverts texture; cross-module undo reverts both.
- The undo stack is serialized per-skin (survives skin switches).

**Files affected:**
- src/core/bridge/MeshPaintBridge.h/cpp - unified undo stack
- src/core/mesh/ - sculpt undo entries conform to bridge interface
- src/core/paint/ - paint undo entries conform to bridge interface

### 3.7 Material Mask Sync Between Sculpt and Paint

**Goal:** Sculpt layers can influence material mask channels, and paint can sculpt-influence material placement.

**Implementation:**
- Sculpt layer visibility/opacity feeds into material mask generation: a "carbon fiber" sculpt layer automatically generates the carbon material mask channel.
- Paint material mask painting (paint/carbon/chrome/matte) can be driven by sculpt curvature (e.g., chrome on flat surfaces, matte in crevices).
- MeshPaintBridge::materialMaskChanged() signal propagates changes to both modules.

**Files affected:**
- src/core/bridge/MeshPaintBridge.h/cpp - material mask generation
- src/modules/modellingEditor/3DModelingQmlBridge.h/cpp - sculpt layer to mask mapping
- src/modules/PaintEditor/PaintEditorWidget.cpp - mask-driven paint options

---

## 4. UI Integration - The Single-Window Experience

### 4.1 Mode Switcher

A top-level mode bar (replacing the current module-switching combo) with three modes:

    [ Sculpt ] [ Paint ] [ Layout ]

- **Sculpt mode** - ksModeler's full sculpt/model toolset. Paint layer panel docks on the right. Brush overlay shows sculpt tools.
- **Paint mode** - ksliveryeditor's full paint toolset. Sculpt layer panel docks on the right. Viewport shows paint cursor with curvature preview.
- **Layout mode** - ksModeler's object hierarchy, transform tools, UV editor. No paint overlay.

Switching modes does **not** reload data - MeshPaintBridge holds the shared state. Viewport camera position, zoom, and selection persist across mode switches.

### 4.2 Floating Paint Toolbar

When paint mode is active, a floating toolbar appears in the viewport area:

    [ Brush v ] [ Size: ====O==== ] [ Hardness: ====O==== ] [ Opacity: ====O==== ]
    [ Color: ## ] [ Layer: [dropdown] ] [ Material: [paint|carbon|chrome|matte] ]

This toolbar is part of 3DModeling_panels.cpp and reads/writes to MeshPaintBridge.

### 4.3 Unified Layer Panel

The right-side panel shows **both sculpt layers and paint layers** in a single scrollable stack:

    --- Sculpt Layers ---
      [+] High Detail    [eye] [lock] [opacity: 80%]
      [+] Base Shape     [eye] [lock] [opacity: 100%]

    --- Paint Layers ---
      [+] Sponsor Logos  [eye] [lock] [opacity: 100%]
      [+] Base Color     [eye] [lock] [opacity: 100%]
      [+] Material Mask  [eye] [lock] [opacity: 100%]

Each section has its own add/remove/reorder controls. Both sections are always visible regardless of mode.

---

## 5. Data Flow Diagrams

### 5.1 Paint Stroke Flow

    User paints in paint mode (ksModeler viewport)
      |
      v
    Viewport3DSystem detects paint input (paintMode == true)
      |
      v
    MeshPaintBridge::applyProjectionPaint(hitPoint, color, radius, strength)
      |
      +---> uvAt(hitPoint) --> get UV coords
      +---> normalAt(hitPoint) --> modulate brush falloff
      +---> curvatureAt(hitPoint) --> modulate brush strength
      |
      v
    MeshPaintBridge::setTexture(modifiedQImage)
      |
      +---> textureModified() signal
             |
             +---> Viewport3DSystem::updatePaintTexture() [ksModeler viewport]
             +---> PaintEditorWidget::refreshCanvas() [ksliveryeditor 2D canvas]
             +---> PaintDocument::pushUndo() [undo stack]

### 5.2 Sculpt Change Flow

    User sculpts in sculpt mode (ksModeler viewport)
      |
      v
    sculptBrush() modifies SceneMesh vertices
      |
      v
    MeshPaintBridge::meshModified() signal
      |
      +---> ksliveryeditor invalidates UV projection cache
      +---> ksliveryeditor re-projects existing paint (best-effort)
      +---> PaintViewport::refreshMesh() [update 3D paint preview]

### 5.3 Skin Switch Flow

    User selects skin in ksliveryeditor skin panel
      |
      v
    MeshPaintBridge::setCurrentSkin(name)
      |
      +---> skinChanged(name) signal
             |
             +---> Viewport3DSystem updates material texture
             +---> PaintEditorWidget refreshes layer list
             +---> CarEditor updates paintConfig

---

## 6. Implementation Roadmap

| Phase | Focus | Items | Effort |
|-------|-------|-------|--------|
| **P1 - Bridge core** | Shared data foundation | MeshPaintBridge singleton, shared SceneMesh + QImage, skin management | Medium |
| **P2 - Live preview** | Texture sync | textureModified signal, ksModeler viewport texture update, 60fps debounce | Low |
| **P3 - Paint mode** | Viewport integration | Paint mode toggle in Viewport3DSystem, floating toolbar, input routing | High |
| **P4 - Curvature brush** | Sculpt-aware paint | normalAt/curvatureAt sampling, curvature-modulated brush, cursor preview | Medium |
| **P5 - Bidirectional sync** | Topology + paint | topologyChanged signal, UV re-projection, skin change propagation | High |
| **P6 - Unified export** | KN5 + DDS pipeline | Export reads from bridge, auto-save dirty paint, eliminate file round-trip | Medium |
| **P7 - Unified undo** | Cross-module undo | Unified undo stack, serialized per-skin, cross-module operation records | High |
| **P8 - Material mask sync** | Sculpt-paint material bridge | Sculpt layer to mask mapping, curvature-driven material placement | Medium |

---

## 7. Files to Create/Modify

### New Files

| File | Purpose |
|------|---------|
| src/core/bridge/MeshPaintBridge.h | Bridge singleton - shared mesh + texture + signals |
| src/core/bridge/MeshPaintBridge.cpp | Bridge implementation |
| src/core/bridge/MeshPaintBridgeUndo.h | Unified undo stack for cross-module ops |

### Modified Files

| File | Changes |
|------|---------|
| src/core/mesh/Viewport3DSystem.h/cpp | Paint mode state, texture uniform update, input routing |
| src/modules/modellingEditor/3DModelingQmlBridge.h/cpp | Paint mode toggle, paint brush calls, topology signal |
| src/modules/modellingEditor/3DModeling_panels.cpp | Paint toolbar overlay, unified layer panel |
| src/modules/modellingEditor/CarBuilder/CarEditor.cpp | Export reads from bridge, dirty paint check |
| src/modules/PaintEditor/PaintEditorModule.h/cpp | Skin change signal to bridge, texture from bridge |
| src/modules/PaintEditor/PaintEditorWidget.h/cpp | Layer panel in unified stack, bridge connectivity |
| src/modules/PaintEditor/PaintViewport.cpp | Mesh from bridge, curvature-aware cursor |
| src/core/paint/PaintPainter.h/cpp | Curvature-modulated brush |
| CMakeLists.txt | Add bridge source files |

---

## 8. Benefits Summary

| Before Integration | After Integration |
|--------------------|--------------------|
| Switch module to paint | Press P in viewport |
| Manual reload after sculpt | Live texture update on sculpt |
| Duplicate mesh in memory | Single shared SceneMesh |
| Manual save before KN5 export | Auto-save dirty paint on export |
| Independent undo stacks | Unified cross-module undo |
| Flat paint on complex surfaces | Curvature-aware brush strokes |
| Sculpt and paint are separate passes | Sculpt and paint are one workflow |
| Material masks manual | Sculpt-driven material mask generation |

---

## 9. Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Performance: texture updates on every paint stroke | Debounce at 60fps, only update dirty region of texture (QImage::rect), GPU upload via QOpenGLTexture::updateSubImage |
| Complexity: unified undo stack is hard to get right | Start with P1-P2 (no undo), add P7 after core is stable; use command pattern with serialize/deserialize |
| Memory: shared QImage could be large (4K textures) | Use QImage copy-on-write; only clone on mutation; consider memory-mapped file for >4K |
| Topology change invalidates paint | Best-effort UV transfer using nearest-UV mapping; warn user on major topology changes; offer "bake before sculpt" option |
| Mode switching UI complexity | Start with simple keyboard shortcut (P/Tab/Esc); full mode bar in P3; keep module combo as fallback |
| Breaking existing workflows | All changes are additive; existing module-switching workflow continues to work; bridge is opt-in initially |

---

## 10. Priority Recommendation

**Start with P1 (Bridge core) + P2 (Live preview).** These two phases deliver the highest value with lowest risk:
- P1 eliminates the duplicate mesh problem and establishes the shared data model.
- P2 gives immediate visual feedback: paint in ksliveryeditor, see it in ksModeler viewport.

P3 (Paint mode in viewport) is the highest-visibility feature but requires the most UI work. Ship P1+P2 first, gather feedback, then build P3 as the flagship integration feature.

P7 (Unified undo) is the hardest to implement correctly and lowest priority for AC-specific workflows (livery artists rarely undo across sculpt/paint). Defer until the bridge is stable.
