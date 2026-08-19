# ksModeler vs Mudbox — Gap Analysis e Roadmap

Data: 2026-08-14
Scope: confronto tra il modulo 3D Modeler di ksEditor (`ksModeler`) e Autodesk Mudbox (digital sculpting, texture painting, PBR, morph targets, layer-based workflow).

---

## 1. Stato attuale di ksModeler

Funzionalità già implementate (verificate nel codebase):

### Primitive e Creazione
- Box, Sphere, Cylinder, Cone, Torus, Plane
  (`Modeler.addPrimitiveCube/Sphere/Cylinder/Cone/Torus/Plane`)

### Tools e Selezione
- Select, Loop Select, Ring Select, Similar
- Trasforma: Move / Rotate / Scale (Gizmo con drag assiale)
- Mirror X/Y/Z, Align
- Proportional Edit con falloff (Smooth/Linear/Sharp/Root/Sphere/Constant)

### Editing Mesh
- Extrude, Inset, Bevel, Loop Cut, Knife, Weld
- Subsurf, Decimate, Remesh, Spin
- Boolean (union / difference / intersect), Boolean stack non-destructive
- Symmetry (X/Y/Z, offset, weld, merge)
- Flip / Recalc Normals
- Multi-Cut interattivo nel viewport, Edge Loop / Edge Ring (bridge + highlight)
- Vertex/Edge Slide, Quad Remesh non distruttivo (0–3, live preview)
- Sculpt (9 brush reali: Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate)
- Push/Pull faces, Offset faces, Fillet/Chamfer (raggio parametrico)

### Curve e NURBS
- Primitive curva: Line, Bezier, BSpline, Arc, Circle
- CV Editor interattivo nel viewport (selezione/drag CV con gizmo)
- Continuità curve C0/C1/C2
- Bridge/Offset curves, Extend surface, Slide CV, Curvature combs
- Superfici NURBS reali: Loft, Sweep, Revolve, Rail, Pipe (kernel Cox-de Boor)
- Persistenza curve/NURBS nel `.ks3d`

### Modificatori e Deformazioni
- Stack modificatori non-destructive (preview live, freeze/bake)
- Simple Deform (Twist / Bend / Stretch)
- Lattice, Cage Deform
- LOD, Collision Mesh, Render Optimizer
- Geometry Nodes (graph)
- Modifier Ex (Wireframe / Skin / Displace)
- Subdivision Cage (Catmull-Clark, preview live non distruttiva)

### Rigging e Skinning
- Scheletro (Humanoid / Biped / FKChain)
- Add/Remove Bone, Bind to Skeleton, auto-weights, smooth skinning
- Weight painting (paint/mirror/prune/normalize)
- FK / IK, pose apply
- Constraints (point/orientation/parent/aim, world-space, offset)

### Animazione
- Timeline base con playbar, play/pause, loop
- Keyframe pos/rot per oggetto, shape keys animati
- F-Curve / Dope Sheet editor con editing key
- Interpolazione (easing), selezione animazione
- Animation Layers (blend non distruttivo)
- NLA (clip/source su master timeline)
- Walk-cycle generator, import motion capture (BVH)

### UV e Texture
- Unwrap LSCM, Project (planar), Mark Seam
- Translate/Rotate/Scale UV, Auto UVs, Pack Atlas
- Texture painting, Material Editor PBR (albedo/metallic/roughness/normal/emissive/opacity)
- Materiali procedurali (marble/wood/cement/asphalt/grass/metal/carbon/plastic/rust/grunge)

### Viewport
- 4 viewport (Top / Front / Right / User) con toggle quad/single
- Modalità display: Shaded / Wireframe / Textured / X-Ray
- Grid texturizzato, assi colorati, gizmo
- Orbit / Pan / Zoom
- Culling per distanza, camera matching (focal/sensor)
- Raytracing CPU preview nel viewport, HDRI/IBL, tonemapping/exposure

### FX e Dinamica
- ICE Particle System: 27 nodi evaluator, node-graph visuale, bake cache, instancing
- Rigid body dynamics (Bullet: Box/Sphere/ConvexHull, kinematic/static/dynamic)
- Cloth / soft body (Verlet, collisioni, self-collision, fabric presets)
- Hair / Fur (strand dinamici Verlet, render ribbon cards)

### Import / Export
- Import: KN5, FBX, GLB/glTF, OBJ, STL, STEP, GPX, KML
- Export: KN5, FBX, GLB, OBJ, STL, STEP, Hidden Line SVG
- Scene: New/Open/Save, progetto `.ks3d` (binario + aux JSON)

### Gestione Scene
- Outliner con gerarchie (Group/Ungroup, world transform)
- Properties panel (pos/rot/scale, visibilità)
- Selection sets (multi-select, insiemi nominati)
- Factory system (template parametrizzati), scene factories
- Stats (totalVertices / totalTriangles)

### Altro
- Undo/Redo (CommandHistory), shortcut remappabili
- Python scripting, Command Palette
- Wizard (Track / Character / Car)
- Track builder, Car builder, Character builder
- Tool AC (Assetto Corsa): width/camber/smooth/terrain, AI Line, GPX/KML
- Sub-object selection (Vertex/Edge/Face/Object), radial menu, context menu
- Construction Planes (CPlane custom + snapping avanzato)
- Dimensioni live (distance/angle/radius), cut by plane

---

## 2. Gap rispetto a Mudbox

### 2.1 Sculpting e deformazione [PRIORITÀ 1]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| Multiresolution sculpting | Multires con livelli di sottodivisione, spostamento tra livelli | Subsurf globale, Sculpt (9 brush) | **GRANDE** — manca multires con livelli indipendenti |
| High-res brush engine | 100+ brush (folds,毛孔, crease, inflate, pinch, smear, flatten, grab, smooth, negate…) | 9 brush (Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate) | **MEDIO** — copertura estesa, mancano folds/pores/bulge/slash |
| Layer sculpting | Sculpt layers (blend, opacity, lock, visibility) | Nessuno | **GRANDE** — fondamentale per workflow Mudbox |
| Stencil painting | Projection painting da immagini con stencil | Texture painting base | **GRANDE** — manca stencil/projection |
| Vector displacement | Vector displacement maps | Nessuno | **GRANDE** — utile per micro-dettagli |
| Mesh extraction | Extract da parte selezionata (thickness, offset) | ✅ `extractFaces()` + bridge `extractSelectedFaces()` + UI in GapToolsPanel | **IMPLEMENTATO** — base per retopology e LOD |
| Smooth brush con pin | Pin/constraint su vertici durante sculpt | Smooth brush (non pin) | **MEDIO** — mancano pin/constraints |
| Face groups / groups | Face groups per masking/isolamento | Nessuno | **MEDIO** — utile per workflow Mudbox |
| Tweak mode | Modo tweak per spostamento leggero di vertici | Vertex Slide | **PARZIALE** |
| Mirror sculpting | Mirror con symmetry plane | Symmetry X/Y/Z | ✅ Presente |
| Grab brush con falloff | Grab con falloff e radius parametrico | Grab brush | **BASSO** |
| Crease brush con edge weighting | Crease con peso spigolo | Crease brush | **BASSO** |

### 2.2 Texture painting e materiali [PRIORITÀ 2]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| 3D texture painting | Pintura directa en 3D con stencil, projection, clone | Texture painting (base) | **GRANDE** — manca projection/stencil |
| Multi-layer painting | Painting layers (blend modes, opacity, lock, masks) | Nessuno | **GRANDE** — fondamentale per workflow artistico |
| PBR material painting | Paint directly into PBR channels (albedo, roughness, metallic, normal, displacement) | Material Editor PBR (albedo/metallic/roughness/normal/emissive) | **MEDIO** — ksEditor ha PBR ma mancano painting layers |
| Color picker avanzato | HSV, RGB, eyedropper, brush presets | Color picker base | **MEDIO** — mancano brush presets |
| Brush masking | Masking per area/vertex/color | Nessuno | **GRANDE** — utile per painting selettiva |
| Tiling textures | Tiling con preview live | Nessuno | **MEDIO** — utile per texturing |
| Export texture maps | Export PBR maps (2K/4K/8K) | Export texture maps | **PARZIALE** — esiste ma meno flessibile |
| Mudbox file format | .mud (proprietary) | .ks3d (binario + aux JSON) | **BASSO** — formato Mudbox è più pesante |
| Texture resolution | Fino a 8K per mappe | Risoluzione non limitata | ✅ Presente |
| Channel packing | Pack multiple channels in single texture | Nessuno | **MEDIO** — utile per ottimizzazione |

### 2.3 Morph targets e blend shapes [PRIORITÀ 3]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| Morph targets | Morph targets con blend, mirror, select by index | Shape keys animati | **MEDIO** — presenti ma meno avanzati |
| Sculpt layers → morph | Conversione automatica sculpt layer → morph target | Nessuno | **GRANDE** — workflow Mudbox specifico |
| Blend shape editing | Edit morph target con brush | Nessuno | **GRANDE** — manca editing diretto |
| Pose-based morph | Morph da pose skeleton | Nessuno | **MEDIO** — utile per facial animation |
| Morph target export | Export morphs per game engine | Nessuno (solo keyframe) | **MEDIO** — per pipeline game |

### 2.4 Retopology e retarget [PRIORITÀ 4]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| Retopology tools | Retopo con quad draw, grid fill, bridge, symmetry | Remesh (Decimate/Remesh) | **GRANDE** — mancano retopo tools dedicati |
| Retarget | Retarget skeleton/animation | Nessuno | **GRANDE** — utile per pipeline character |
| Mesh extraction | Extract con thickness da selezione | Nessuno | **GRANDE** — base per retopology |
| UV projection | Cylindrical/spherical/planar projection con调整 | Project (planar) | **PARZIALE** |
| Auto-retopo | Auto-retopology (quad-based) | Quad Remesh (non distruttivo) | **PARZIALE** — esiste ma meno controllato |

### 2.5 Animazione e deformazione [PRIORITÀ 5]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| Joint-based deformation | Skeleton con skinning | Skeleton (Humanoid/Biped/FKChain) | ✅ Presente |
| Weight painting | Weight painting con brush | Weight painting (paint/mirror/prune/normalize) | ✅ Presente |
| Pose sculpting | Sculpt in pose-specific | Nessuno | **GRANDE** — utile per character art |
| Blend shape animation | Morph target animation con curve | Shape keys animati | **PARZIALE** |
| Timeline | Timeline base con playback | Timeline con playbar, F-Curve/Dope Sheet | ✅ Presente |
| Animation layers | Nessuno (layer-based sculpting) | Animation Layers (blend non distruttivo) | ✅ Presente ksEditor |
| NLA | Nessuno | NLA (clip/source) | ✅ Presente ksEditor |
| Constraints | Nessuno | Point/Orient/Parent/Aim | ✅ Presente ksEditor |

### 2.6 Viewport e rendering [PRIORITÀ 6]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| PBR viewport | OpenGL con PBR, IBL, tonemapping | Raytracing CPU + PBR viewport | ✅ Presente |
| Stencil display | Display stencil in viewport | Nessuno | **MEDIO** — per projection painting |
| Matcap rendering | Matcap per preview scultura | Nessuno | **MEDIO** — utile per sculpting |
| Wireframe overlay | Wireframe su shaded | Shaded/Wireframe/Textured/X-Ray | ✅ Presente |
| Depth of field | DOF con focal point | Nessuno | **MEDIO** — per preview artistico |
| Ambient occlusion | AO real-time | AO (non menzionato esplicitamente) | **BASSO** |
| Silhouette display | Silhouette mode | Nessuno | **MEDIO** — per valutazione forma |
| Turntable | Turntable animation | Nessuno | **BASSO** |

### 2.7 Scene e workflow [PRIORITÀ 7]

| Feature | Mudbox | ksModeler | Gap |
|---------|--------|-----------|-----|
| Layer system | Sculpt/paint layers (fondamentale) | Nessuno layer | **GRANDE** — differenza chiave |
| Scene management | Scene con multiple mesh | Outliner con gerarchie | ✅ Presente ksEditor |
| Selection sets | Face/vertex sets per masking | Selection sets (multi-select) | **PARZIALE** |
| Undo/Redo | Undo storico | CommandHistory | ✅ Presente |
| Import/Export | OBJ, FBX, PSD, Mudbox | KN5, FBX, GLB, OBJ, STL, STEP | ✅ Presente ksEditor |
| File size | .mud (pesante, contiene texture/sculpt) | .ks3d (leggero) | ✅ Presente ksEditor |
| Python scripting | Limitato | Python scripting, Command Palette | ✅ Presente ksEditor |
| Hotkeys | Custom hotkeys | Shortcut remappabili | ✅ Presente ksEditor |
| Viewport navigation | Orbit/Pan/Zoom | Orbit/Pan/Zoom | ✅ Presente |
| Multi-view | Single view (fullscreen) | 4 viewport (Top/Front/Right/User) | ✅ Presente ksEditor |

---

## 3. Gap minori / placeholder noti

Rilevati durante l'analisi:

| Area | Problema |
|---|---|
| Multiresolution sculpting | Nessun sistema di livelli di sottodivisione indipendenti |
| Sculpt layers | Nessun layer per sculpting (fondamentale per Mudbox workflow) |
| Projection painting | Nessuna proiezione da immagini con stencil |
| Painting layers | Nessun sistema di layer per texture painting |
| Morph target editing | Nessun editing diretto di morph target con brush |
| Retopology tools | Nessun quad draw/grid fill/bridge per retopo |
| Mesh extraction | Nessuna estrusione da selezione con thickness |
| Face groups | Nessun sistema di group per masking/isolamento |

---

## 4. Priorità consigliate

### Fase 1 — Core sculpting (gap più sentiti)
1. **Multiresolution sculpting** — livelli di sottodivisione indipendenti con spostamento tra livelli
2. **Sculpt layers** — layer per sculpting con blend/opacity/lock/visibility
3. **Extended brush engine** — aggiungere brush mancanti (inflate, pinch, smear, negate, folds,毛孔)
4. **Face groups** — gruppi per masking/isolamento durante sculpting

### Fase 2 — Texture painting avanzato
5. **Projection painting** — stencil da immagini con proiezione 3D
6. **Painting layers** — layer per texture con blend modes/opacity/lock/masks
7. **Brush masking** — masking per area/vertex/color durante painting
8. **Channel packing** — pack multipli canali PBR in singola texture

### Fase 3 — Morph e retopology
9. **Morph target editing** — editing diretto di morph target con brush
10. **Sculpt → morph conversion** — conversione automatica sculpt layer → morph target
11. **Mesh extraction** — extract da selezione con thickness/offset
12. **Retopology tools** — quad draw, grid fill, bridge per retopo manuale

### Fase 4 — Workflow e ottimizzazione
13. **Tiling textures** — tiling con preview live
14. **Matcap rendering** — matcap per preview scultura
15. **Pose sculpting** — sculpt in pose-specific per character art
16. **DOF/Silhouette** — depth of field e silhouette mode per preview artistico

---

## 5. Definizione di "done" per la roadmap

### Fase 1 — Core sculpting
#### Multiresolution sculpting
- [ ] Multiresolution manager (livelli 0–6, spostamento tra livelli)
- [ ] Subdivision uniforme (Catmull-Clark) con preservazione forma
- [ ] Scultura indipendente per livello (nessuna interferenza)
- [ ] UI: pannello Multires con livelli, toggle, freeze/bake

#### Sculpt layers
- [ ] SculptLayer model (nome, visibilità, opacity, lock, blend mode)
- [ ] Aggiungi/rimuovi/rinomina layer, assegna brush
- [ ] Blend tra layer (additive, subtractive, replace)
- [ ] Persistenza layer nel `.ks3d`

#### Extended brush engine
- [x] Brush mancanti: Inflate, Pinch, Smear, Negate ✅ IMPLEMENTED (mode 5–8 in `MeshOperations::sculptBrush` + SculptPanel)
- [ ] Brush mancanti: Folds, Pores, Bulge, Slash
- [ ] Parametri brush: radius, strength, falloff (custom curve)
- [ ] Pin/constraint su vertici durante sculpt
- [ ] UI: brush presets, parametri in-testa

#### Face groups
- [ ] FaceGroup model (nome, colore, visibilità)
- [ ] Assegna/rimuovi facce da group
- [ ] Masking per group (lock, hide, isolate)
- [ ] UI: pannello Face Groups

### Fase 2 — Texture painting avanzato
#### Projection painting
- [ ] Stencil da immagini (load PNG/JPG, resize, rotate, opacity)
- [ ] Projection 3D su mesh (raycast + UV mapping)
- [ ] Clone tool (source point → destination)
- [ ] UI: stencil overlay, projection brush

#### Painting layers
- [ ] PaintLayer model (nome, visibilità, opacity, lock, blend mode)
- [ ] Blend modes: Normal, Multiply, Screen, Overlay, Soft Light
- [ ] Masks per layer (vertex mask, face mask)
- [ ] Persistenza layers nel `.ks3d`

#### Brush masking
- [ ] Vertex mask (paint mask con brush)
- [ ] Face mask (selezione facce per masking)
- [ ] Symmetry mask (mirror mask)
- [ ] UI: mask overlay, toggle mask

#### Channel packing
- [ ] Pack albedo+roughness+metallic in RGB+A
- [ ] Pack normal+height in RG+B
- [ ] Export con risoluzione configurabile (1K–8K)
- [ ] UI: export dialog con preview

### Fase 3 — Morph e retopology
#### Morph target editing
- [ ] MorphTargetEditor (selezione morph target, brush editing)
- [ ] Paint morph target con Draw/Smooth/Grab/Flatten/Crease
- [ ] Mirror morph target
- [ ] UI: morph target list, blend slider

#### Sculpt → morph conversion
- [ ] Conversione automatica sculpt layer → morph target
- [ ] Opzioni: flatten, bake, merge
- [ ] UI: convert button nel pannello sculpt layers

#### Mesh extraction
- [x] Extract da selezione (thickness, offset, smooth) ✅ IMPLEMENTED — `MeshOperations::extractFaces(mesh, faces, thickness, closeCaps)` + test `testExtractFaces`
- [x] Cap chiusura (bordi aperti) ✅ IMPLEMENTED — `closeCaps` opzionale (shell con rim walls)
- [x] UI: extract dialog con preview — bottoni "Extract Faces"/"Extract Solid" in `GapToolsPanel.qml`

#### Retopology tools
- [ ] Quad draw (click per vertex, auto-face)
- [ ] Grid fill (fill hole con griglia)
- [ ] Bridge (connessione edge loops)
- [ ] UI: retopo mode con tools

### Fase 4 — Workflow e ottimizzazione
- [ ] Tiling textures con preview live
- [ ] Matcap rendering per preview scultura
- [ ] Pose sculpting (sculpt in pose-specific)
- [ ] DOF/Silhouette mode per preview artistico

---

## 6. Confronto filosofico: Mudbox vs ksModeler

| Aspetto | Mudbox (DCC) | ksModeler (Tool) |
|---------|--------------|------------------|
| **Pubblico** | Character artists, texture artists | Automotive / Assetto Corsa / track building |
| **Focus** | Scultura ad alta risoluzione, texture painting | Modeling, animation, FX, AC pipeline |
| **Multires** | Core feature (livelli indipendenti) | Subsurf globale (non indipendente) |
| **Layers** | Sculpt/paint layers (fondamentale) | Animation layers, nessun sculpt layer |
| **Brush** | 100+ brush specializzati | 9 brush generici |
| **Painting** | Projection/stencil, multi-layer | Texture painting base |
| **Morph** | Morph targets avanzati, editing con brush | Shape keys animati |
| **Retopo** | Tools dedicati (quad draw, grid fill) | Remesh (Decimate/Quad Remesh) |
| **File** | .mud (pesante, completo) | .ks3d (leggero, modulare) |
| **Target** | Character art, film, games | Automotive, AC/EVO content |

### Strategia consigliata
Mudbox è il DCC specializzato in scultura e texture painting: replicarne ogni singola feature non è l'obiettivo. La strategia ottimale è:

1. **Adottare il workflow Mudbox più richiesto** — Multiresolution sculpting, sculpt layers, projection painting
2. **Mantenere il focus AC** — KN5, AI line, track/car builder restano il differenziale
3. **Espandere il brush engine** sui casi più comuni (inflate, pinch, smear, negate) — ✅ fatto: 9 brush in `MeshOperations::sculptBrush`; restano folds/pores/bulge/slash e falloff custom
4. **Layer system per sculpt/paint** — fondamentale per workflow artistico
5. **Morph targets avanzati** — editing con brush, conversione sculpt→morph
6. **Retopology tools** — quad draw, grid fill per pipeline character

---

## 7. Note implementative

### Multiresolution sculpting
- Modello: `MultiresManager` con `vector<SubdivisionLevel>` (livello 0–6)
- Ogni livello contiene: vertex positions, edge creases, face normals
- Subdivision: Catmull-Clark con preservazione forma (limit surface)
- Scultura: applica deformazione al livello corrente, propaga ai livelli superiori
- UI: slider livelli, toggle freeze/bake, preview live

### Sculpt layers
- Modello: `SculptLayer` con `QVector<float>` per peso per vertice
- Blend: additive (somma pesi), subtractive (sottrai), replace (sostituisci)
- Opacity: moltiplica peso per 0–1
- Lock: impedisce modifiche al layer
- Persistenza: salva nel `.ks3d` (aux JSON)

### Projection painting
- Load stencil: `QImage` da file, resize/rotate con transform
- Projection: raycast da viewport → UV coordinate → sample stencil → apply color
- Clone: source point (UV) → destination (UV), copy con blend
- UI: stencil overlay (opacity, transform), projection brush

### Painting layers
- Modello: `PaintLayer` con `QImage` per ogni canale PBR
- Blend: Normal (alpha blend), Multiply, Screen, Overlay, Soft Light
- Masks: vertex mask (`QVector<bool>`), face mask (`QVector<bool>`)
- Persistenza: salva nel `.ks3d` (aux JSON)

### Morph target editing
- Modello: `MorphTarget` con `QVector<QVector3D>` (offset per vertice)
- Editing: applica brush a morph target selezionato
- Mirror: copia offset da lato sinistro a destro (e viceversa)
- UI: morph target list, blend slider 0–1, edit mode

### Mesh extraction
- ✅ IMPLEMENTED — `MeshOperations::extractFaces(mesh, faces, thickness, closeCaps)`: selezione `QVector<int>` (indici facce selezionate)
- ✅ Thickness: offset lungo normali vertice (via `shell()`) quando `thickness > 0` → estratto "solido" con rim walls
- Smooth: Laplacian smoothing su bordi aperti (non implementato)
- ✅ Cap: chiusura opzionale con `closeCaps` (rim walls watertight)
- ✅ UI: bottoni "Extract Faces" / "Extract Solid" in `GapToolsPanel.qml` (bridge `Modeler.extractSelectedFaces(thickness, closeCaps)`)
- Nota: Smoothness lungo i bordi dell'estrazione non ancora esposto

### Retopology tools
- Quad draw: click per vertex, auto-face (3–4 vertex vicini)
- Grid fill: fill hole con griglia regolare (parametri: rows, columns)
- Bridge: connessione edge loops (parametri: segments, twist)
- UI: retopo mode con tools, snap to high-res mesh
