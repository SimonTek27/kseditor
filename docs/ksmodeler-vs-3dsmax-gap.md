# ksModeler vs 3ds Max — Gap Analysis e Roadmap

Data: 2026-08-14
Scope: confronto tra il modulo 3D Modeler di ksEditor (`ksModeler`) e Autodesk 3ds Max (poly-modeling, splines, modifier stack, animazione/rigging, rendering).

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

## 2. Gap rispetto a 3ds Max

### 2.1 Modifier Stack e non-destructive modeling  [PRIORITÀ 1]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Modifier Stack editor completo | Stack con lista, riordino, toggle, collasso, parametri editabili | Stack non-destructive con freeze/bake | **MEDIO** — lo stack esiste; manca l'editor visivo con parametri in-testa |
| Subdivision surface (TurboSmooth/OpenSubdiv) | TurboSmooth, OpenSubdiv con crease, preview adaptive | Subsurf + Subdivision Cage (Catmull-Clark) | **MEDIO** — mancano crease e pinned vertices |
| Modificatori procedurali 100+ | Bend, Twist, Taper, FFD, Wave, Ripple, Noise, Melt, Shell, Lathe, Extrude, Bevel, Edit Poly, Push, Relax, Spherify, XForm… | Simple Deform, Lattice, Wireframe, Skin, Displace | **GRANDE** — copertura ridotta |
| Modifier Ex / Geometry Nodes | Modifier stack per qualsiasi sub-object | Geometry Nodes (graph) | **PARZIALE** |
| Symmetry modifier non distruttivo | Symmetry con weld, threshold, gizmo | Symmetry operativa (distruttiva) | **MEDIO** |
| Shell modifier | Spessore automatico su mesh aperta | ✅ `MeshOperations::shell` + `applyShell` | **COMPLETATO** |
| FFD lattice non distruttivo | FFD 2x2/3x3/4x4 con lattice editabile | Lattice (base) | **MEDIO** |

### 2.2 Poly Modeling e Graphite  [PRIORITÀ 2]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Edit Poly con sub-object modes | Vertex/Edge/Border/Polygon/Element pieno | Sub-object + Border/Element | **MIGLIORATO** — manca solo UI toolbar 5/6 |
| Connects multiple edge loops | Connect con numero/carico (slide, crease) | Loop Cut singolo | **MEDIO** |
| Chamfer con crease | Chamfer su vertici/spigoli con open/crease | Fillet/Chamfer su edge | **MEDIO** |
| Bridge (edge/face) | Bridge polygon con targeting | ✅ `bridgeEdges` + `bridgeFaces` | **COMPLETATO** |
| Target Weld / collapse | Merge mirato di vertici | Weld (merge globale) | **BASSO** |
| Quadify Mesh | Conversione mesh → quad, con Quadify | Quad Remesh (non distruttivo) | **BASSO** |
| Soft Selection con falloff | Soft selection su sub-object, splash, edge distance | Proportional Edit | **MEDIO** |
| Graphite Modeling ribbon | 100+ tool tab organizzati per ribbon | Tools panel + ribbon limitati | **GRANDE** — organizzazione e copertura |
| Paint Deformation | Brush di push/pull/relax/ok su mesh | Sculpt brush (Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate) | **PARZIALE** — copertura estesa a 9 brush |
| Autosmooth / smoothing groups | Smoothing groups (32) su Edit Poly | ✅ `autoSmooth` + `smoothGroupsAuto` | **COMPLETATO** |
| NURMS / Edit Poly smoothing | Smoothness per sub-object | Subsurf globale | **BASSO** |

### 2.3 Spline / Shape  [PRIORITÀ 3]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Editable Spline pieno | Vertex/Segment/Spline sub-modes, fillet/chamfer su vertici | Curve NURBS + CV editor | **MEDIO** — workflow diverso (spline vs NURBS) |
| Renderable spline | Spline con spessore, generate mapping | Curve To Mesh | **BASSO** |
| Lathe | Revolve da profilo spline | Curve Revolve | ✅ Presente |
| Extrude shape | Estensione lineare spline | Nessuno | **BASSO** |
| Loft (compound object) | Loft con multiple shapes + deformations (scale/rot/twist) | Curve Loft / Rail | **MEDIO** |
| Bevel / Bevel Profile | Raccordo lungo spline con profilo | Bevel mesh | **MEDIO** |
| Text spline | Spline da testo (SHIFT) | Nessuno | **MEDIO** |
| Edit Spline con region | Regioni sel/similar per editing veloce | Nessuno | **BASSO** |
| Spline section (sezione) | Sezione spline da intersezioni | cutByPlane (mesh) | **BASSO** |

### 2.4 Boolean e compound objects  [PRIORITÀ 4]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| ProBoolean / ProCutter | Boolean robusto con operations list, UV, open | Boolean stack non-destructive | **PARZIALE** — manca cutter/morph |
| Compound objects (Morph, Scatter, Connect, ShapeMerge, Terrain, BlobMesh) | Full compound set | Nessuno (solo boolean) | **GRANDE** |
| Instance / Reference objects | Referenze live con hierarchy | InstanceReference (live) | ✅ Presente |
| Array / Clone | Clone con instance, array tools | Linear/Radial/Grid array | ✅ Presente |

### 2.5 Animazione e rigging  [PRIORITÀ 5]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Track View (Curve Editor + Dope Sheet) | Editor completo con controllers, tangenti | F-Curve / Dope Sheet editor | **PARZIALE** |
| Controllers (PRS, LookAt, Noise, Spring, Attachment) | Sistema controller plugin-based | Keyframe diretti | **GRANDE** |
| Animation Layers | Layers non distruttivi per animazione | Animation Layers | ✅ Presente |
| Motion Mixer / NLA | Mixer biped/CAT con clip e pesi | NLA (clip/source) | **PARZIALE** |
| Constraints estesi | Position/Path/LookAt/Attachment/Orientation/Link | Point/Orient/Parent/Aim | **MEDIO** |
| Wire Parameters / Parameter Wiring | Wiring di parametri tra oggetti | Nessuno | **GRANDE** |
| Reaction Manager | Risposte condizionali tra parametri | Nessuno | **MEDIO** |
| Motion Capture (contour, filter) | Filtri e cleanup mocap | Import BVH | **PARZIALE** |
| State Sets / Animation State | Multi-state rig | Nessuno | **MEDIO** |
| Bake sim al keyframe | Bake di qualsiasi sim su curve | Bake cache ICE | **PARZIALE** |

### 2.6 Character rigging  [PRIORITÀ 6]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Biped (nativo) | Rig biped completo (footstep, layers, pose) | Skeleton Humanoid/Biped + walk-cycle | **PARZIALE** |
| CAT (Character Animation Toolkit) | Rig CAT con layer, muscle, pompon | FKChain | **GRANDE** |
| Skin modifier | Envelope, weights, cross-sections, painting | Auto-weights + weight painting | **PARZIALE** |
| Skin Wrap / Physique | Deformazioni aggiuntive | Nessuno | **MEDIO** |
| Vertex maps / Morpher | Shape keys avanzati con weights | Shape keys animati | **PARZIALE** |
| Skinning con lattice / wiring | Deformazione personalizzata | Lattice | **BASSO** |
| Mirror per pesi | Mirror weights | Weight mirror | ✅ Presente |

### 2.7 Materiali e texture  [PRIORITÀ 7]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Physical Material / Standard | PBR fisico con clearcoat, subsurface, emission | PBR (albedo/metallic/roughness/normal/emissive) | **MEDIO** |
| Material node editor (Slate) | Grafo nodi materiali | Nessuno (editor a pannelli) | **GRANDE** |
| Multi/Sub-Object material | Materiali per sub-object | Nessuno | **MEDIO** |
| Maps procedurali (Noise, Falloff, Gradient, Cellular) | Node-based maps | Materiali procedurali (marble/wood/…) | **PARZIALE** |
| Bump / Normal map stacking | Stack di mappe | Normal map singola | **BASSO** |
| UVW mapping modifiers | Box/Planar/Spherical/Cylindrical, Unwrap UVW | Project planar, Unwrap LSCM | **PARZIALE** |
| Viewport Canvas | Texture painting nel viewport | Texture painting | ✅ Presente |
| Physical camera / exposure | Camere fisiche con ISO/shutter/aperture | Camera matching (focal/sensor) | **MEDIO** |

### 2.8 Rendering e lighting  [PRIORITÀ 8]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Render engine pluggable (Arnold/V-Ray/Scanline) | Render engine third-party | Path tracer CPU interno | **MEDIO** |
| Light Lister / photometric lights | Luci fotometriche con IES, Light Lister | Luci base | **MEDIO** |
| Shadow types | Area/soft/raytrace shadows | Ombre dure + sole area | **MEDIO** |
| Light fixtures (Geometric + Light) | Assemblee luce | Nessuno | **BASSO** |
| Exposure control (photographic) | Fotografico, logarithmic | Tonemapping ACES/Filmic | **PARZIALE** |
| Fog / atmosphere / environment effects | Volume fog, exposure, sky | Sky box + HDRI | **MEDIO** |
| Render elements / compositing | Separazione pass (Z-depth, AO, diffuse…) | Nessuno | **GRANDE** |
| Batch render | Sequenze e frames con stato | Render file singolo | **MEDIO** |
| ActiveShade / interactive render | Preview interattiva | Raytrace preview viewport | **PARZIALE** |
| Camera tracking (Match Photo) | Calibrazione da foto | Nessuno | **MEDIO** |
| Bake materials (to texture) | Bake di lighting/occlusion su texture | Nessuno | **MEDIO** |

### 2.9 FX e dinamica  [PRIORITÀ 9]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Particle Flow | Grafo eventi particle (non solo nodi) | ICE (node-graph 27 evaluator) | **PARZIALE** |
| Space Warps (Wave, Ripple, Wind, Gravity…) | Warps come modificatori di campo | Forces ICE (gravity/wind/turbulence) | **PARZIALE** |
| MassFX / ragdoll | Physics con constraints e ragdoll | Rigid body (Bullet) | **MEDIO** — manca ragdoll/constraints |
| Hair & Fur | Styling, groom, render realistico | Hair/Fur (strand Verlet) | **MEDIO** — manca styling avanzato |
| Cloth / Garment Maker | Cucitura abiti, presets | Cloth (Verlet + presets) | **PARZIALE** |
| Fluids (Alembic, Phoenix-style) | Sim fluidi | Nessuno | **GRANDE** |
| Morph / Melt per distruzione | Deformazioni dinamiche | Nessuno | **BASSO** |

### 2.10 Scene, data e workflow  [PRIORITÀ 10]

| Feature | 3ds Max | ksModeler | Gap |
|---------|---------|-----------|-----|
| Layers | Layer system con visibilità/gelo | ✅ `addLayer/setLayerVisible/assignSelectionToLayer` | **COMPLETATO** (visibilità + colore; manca freeze) |
| Containers | Sistemi container (asset incapsulati) | Group/Ungroup | **MEDIO** |
| XRef | Riferimenti esterni a scene/oggetti | Nessuno | **MEDIO** |
| Scene Explorer | Tabella filtrabile multi-colonna | Outliner | **PARZIALE** |
| Asset Tracking | Gestione file esterni | Assets Library (editor separato) | **PARZIALE** |
| MAXScript / Python | Scripting completo integrato | Python scripting | **PARZIALE** |
| Data exchange (Alembic, USD) | Alembic, USD, FBX, glTF, DXF/DWG | FBX, glTF, OBJ, STEP, STL, KN5 | **MEDIO** — manca Alembic/USD |
| Snapshot / clone | Clone in-place, snapshot anim | Array tools | ✅ Presente |
| Undo across tools | Undo globale storico (MacroScript) | CommandHistory | **PARZIALE** |
| Preset manager | Salvataggio preset per tool | Factory system | **PARZIALE** |
| Hotkey/menu/quad customization | UI totalmente customizzabile | Shortcut remappabili, radial menu | **PARZIALE** |
| Viewport shading modes | Full shading, wireframe, real-time, isoline | Shaded/Wireframe/Textured/X-Ray | **PARZIALE** |

---

## 3. Gap minori / placeholder noti

Rilevati durante l'analisi:

| Area | Problema |
|---|---|
| Border/Element selection | ✅ Implementati (`SelectionMode::Border=5`, `Element=6`, `borderEdges`, `faceElements`, `selectBorderUnderCursor`) |
| Bridge poligonale | ✅ Implementati (`bridgeEdges`/`bridgeFaces` + `bridgeSelectedLoops`/`bridgeSelectedFaces`) |
| Shell/Thicken | ✅ Implementato (`MeshOperations::shell` + `applyShell` con rim walls) |
| Controllers | Nessun sistema di controllers/expressions |
| Wire Parameters | Nessun wiring di parametri tra oggetti |
| Layer system | ✅ Implementato (`addLayer/removeLayer/setLayerVisible/setLayerColor/assignSelectionToLayer`) |
| Export Alembic/USD | Formati mancanti per pipeline film/games |
| Render elements | Nessuna separazione di pass di rendering |

---

## 4. Priorità consigliate

### Fase 1 — Core modeling (gap più sentiti)
1. **Layer system** — organizzazione scena standard in Max ✅ Implementato
2. **Shell modifier** — spessore automatico su mesh aperte (utility enorme) ✅ Implementato
3. **Bridge poligonale** — bridge di edge/face (non solo curve) ✅ Implementato
4. **Border/Element sub-object modes** — completare la selezione sub-object ✅ Implementato (manca UI toolbar 5/6)
5. **Smoothing groups** — autosmooth 32 gruppi in stile Max ✅ Implementato (manca editor per faccia)

### Fase 2 — Modifier stack esteso
6. **Modificatori procedurali aggiuntivi** — Taper, Wave, Ripple, Noise, Push, Relax, Shell, Lathe, Melt ✅ Implementato (Taper/Ripple/Noise/Push/Relax/Melt/Lathe nuovi classi Deform/Generate; Wave dal WaveModifierEx; tutti registrati in ModifierStack + MeshModifier e nei combo QML)
7. **FFD lattice parametrizzato** — FFD 2x2/3x3/4x4 con lattice editabile ✅ Coperto (LatticeExModifier nello stack + control points draggabili via setLatticeControlPoint)
8. **Crease e pinned vertices su Subsurf** — controllo TurboSmooth-style ✅ Implementato (creare edge weight 0..1 via OpenSubdiv; pin = sharp su spigoli incidenti; UI nel pannello stack)
9. **Symmetry modifier non distruttivo** — con threshold weld ✅ Coperto (MirrorModifier non distruttivo + SymmetryManager bridge)

### Fase 3 — Animazione/rigging avanzato
10. **Controllers / expressions** — sistema di controllers (Noise, Spring, LookAt) ✅ **Implementato** (`ControllerSystem`: Noise = value noise 1D deterministico su base±amp, Spring = molla smorzata `base + amp*e^(-d·t)·sin(2πft+φ)`, LookAt = aim constraint (+Z verso target, round-trip decompose/euler), Attachment = segue un vertice della mesh target + offset world→local; canali position/rotation/scale/visibility/opacity/metallic/roughness via `SceneParamAccess`; timer 20Hz nel bridge; persistenza nel `auxJson` `.ks3d`; pannello QML ControllersPanel)
11. **Constraints estesi** — Path, Attachment, Link, Spring ✅ **Implementato** (ConstraintSystem: Point/Orientation/Aim/Path/Attachment/Parent/Link/Spring, 10/10 test)
12. **Wire Parameters** — wiring tra parametri di oggetti diversi ✅ **Implementato** (`WireParameterSystem`: binding driver→driven con mappa affine `value = driver*scale + offset`, keyed per drivenId, skip |Δ|<1e-6 anti ping-pong, reentrancy guard, timer 20Hz, persistenza `.ks3d`, pannello QML WireParametersPanel)
13. **Skin Wrap / morpher avanzato** — deformazioni aggiuntive sullo skinning ✅ **Implementato** (`SkinWrapSystem`: per-vertex bind al triangolo cage più vicino (closest-point-on-triangle Ericson + barycentriche esatte) + offset world; helper puri `captureGeometry/applyGeometry`; rebind esplicito; persistenza `.ks3d`; pannello QML SkinWrapPanel)

### Fase 4 — Rendering e pipeline
14. **Render elements** — Z-depth, AO, diffuse, normal pass ✅ **Implementato** (`RayTraceRenderer::RenderPass` Color/Depth/AmbientOcclusion/Diffuse/Normal: depth normalizzato sul far-plane (vicino=chiaro, sfondo=nero), AO a 16 raggi cosine-hemisphere su raggio scena, albedo flat = colore base, normal world-space `N*0.5+0.5`; preview ~6fps con selettore pass nell'overlay raytrace + `rayTraceRenderToFile(path,w,h,samples,pass)`; `buildRTTriangles` ora inietta metallic/roughness reali dell'oggetto; test `test_ray_trace` 7/7)
15. **Export Alembic / USD** — interop con pipeline film/games
16. **Material node editor** — grafo nodi materiali (Slate-style) ✅ **Implementato** (`NodeMaterialEditor` riattivato: grafo di ~26 tipi nodo con `toJson/fromJson` round-trip fedele (id, posizioni, valori socket Float/Color/Float3/Immagine, texturePath, connessioni derivate dai socket), fix `updateLinks` per flag `isConnected` sugli output dopo delete; bridge `matNodeEditorGraph/AvailableTypes/CreateNode/DeleteNode/Connect/Disconnect/MoveNode/SetSocketValue/SetTexture/Clear/GenerateShader` + segnale `matNodeGraphChanged`, persistenza `.ks3d` in aux `materialNodeGraph`; pannello QML `MaterialNodePanel.qml` Slate-style con canvas pan/zoom, palette nodi, drag nodi, connessioni bezier via Canvas, inspector con slider/color/vector edits e dialoghi texture, generazione GLSL + push al materiale corrente; ribbon "Node Editor"; test `test_node_material_editor` 8/8)
17. **Light Lister + IES photometric** — luci fotometriche con profili IES ✅ **Implementato** (`LightSystem` = `LightDef` per oggetto `Type::Light`: type Directional/Point/Spot/Area, colore, intensity, range, cone+penumbra, profilo IES con curva verticale normalizzata; parser `parseIESFile` IESNA LM-63 (skip header/TILT=NONE, media su angoli orizzontali, 91 campioni 0-90°); il `RayTraceRenderer` ora illumina da `setLights()` con shadow/cone/IES/attenuazione quadratica + speculare, fallback al sole classico senza luci; bridge `lightCreate/lightSet*/lightList/lightRemove`, persistenza `.ks3d` keyed per nome; pannello QML `LightPanel.qml` (ribbon "Light Lister"); test `test_light_system` 5/5 + 3 nuovi casi luce in `test_ray_trace`)
18. **Bake to texture** — baking lighting/occlusion su mappe ✅ **Implementato** (`TextureBaker` standalone: bake UV-space per-texel di AO (rasterizzazione triangoli in spazio UV, 16 raggi cosine-hemisphere su raggio = 15% diagonale scena, fallback planar-projection quando senza UV), Normal world-space con fallback +Z senza normali, Diffuse = colore base, Roughness/Metallic/Height/Emission; bridge `bakeObject/bakePreview/bakeClearResult/bakeTypeName` + proprietà `bakeRevision` con segnale `bakeResultChanged`, preview via `BakeImageProvider` `image://bake/result?rev=`; pannello QML `BakePanel.qml` (ribbon "Bake"); test `test_bake_texture` 8/8)

---

## 5. Definizione di "done" per la roadmap

### Fase 1 — Core modeling
#### Layer system
- [x] Modello dati `Layer` in C++ (nome, visibilità, freeze, colore)
- [x] Bridge: `layerCreate/Remove/Rename/SetVisible/SetFreeze/AssignObject/ObjectLayers`
- [x] Persistenza layer nel `.ks3d` (aux JSON) — `LayerDef` (name/visible/color) + current index + mappa oggetto→layer (per nome, con re-index dopo removeLayer), restore con risoluzione idByName e applicazione visibilità
- [x] UI: pannello Layers (aggiungi/rimuovi/rinomina, toggle, assegnazione oggetto)

#### Shell modifier
- [x] `MeshOperations::shell(mesh, thickness, direction)` — estrusione lungo normali
- [x] Chiusura bordi e gestione concavità
- [x] Bridge `applyShell(objectId, thickness)`
- [x] UI: sezione "Shell" nel panel modificatori

#### Bridge poligonale
- [x] `MeshOperations::bridgeEdges(mesh, v0a, v0b, v1a, v1b, segments)` — connessione tra 2 loop di spigoli
- [x] `bridgeFaces(mesh, faceA, faceB, segments)` — connessione tra facce
- [x] Bridge: `bridgeEdgeLoops(objectId, loopA, loopB)`
- [x] UI: sezione "Bridge" con selezione dei due loop

#### Border/Element sub-object modes
- [x] Enum `SelectionMode` esteso: Border=5, Element=6 (`All=4` occupava già lo slot 4)
- [x] `findClosestBorder` / `findClosestElement` con raycast
- [x] UI: bottoni 1-5/6 nella toolbar sub-object (V/E/F/Obj/B/El) + voce context-menu E Modes Border/Element + routing click viewport (`subobjectMode()===5/6` → `selectBorderUnderCursor`/`selectElementUnderCursor`)

#### Smoothing groups
- [x] `MeshOperations::autoSmooth(mesh, angle)` — 32 gruppi da angolo diedro
- [x] Editor smoothing groups per faccia — `smoothGroupSetFace(objectId, faceIndex, groupId)` / `smoothGroupForFace` / `smoothGroupAssignSelected(groupId)` (su `selectedSubFaces()`) / `smoothGroupClear`
- [x] Persistenza nel `.ks3d` — `m_smoothGroups` serializzato per nome oggetto, ripristinato con idByName; resettate in `newProject()`

### Fase 2 — Modifier stack esteso
- [x] `MeshOperations::taper/wave/ripple/noise/push/relax/lathe/melt` — modificatori procedurali (Taper, Ripple, Noise, Push, Relax, Melt, Lathe nuovi; Wave=WaveModifierEx; tutti registrati nello stack)
- [x] Esposizione di tutti i modificatori implementati (Skin, Shrinkwrap, CageDeform, LatticeEx, Curve, UVProject, Surface/VolumeSmooth) nei combo del Modifier Stack Panel
- [x] Crease + pinned vertices in `SubdivisionModifier` — crease edges (weight 0..1) e pinned vertices (spigoli incidenti sharp=1.0) alimentano `SubdivisionSurface::subdivideWithCreases`; UI nel Modifier Stack Panel (Crease Selected Edges / Pin Selected Verts / Clear) + `readParameters/writeParameters` per la persistenza nel `.ks3d`
- [x] FFD con lattice 2x2/3x3/4x4 parametrizzato — coperto da `LatticeExModifier` (uDivs/vDivs/wDivs qualsiasi risoluzione) già nello stack + control points editabili nel viewport (`applyLattice`, `setLatticeControlPoint`)
- [x] Symmetry non distruttivo con threshold — coperto da `MirrorModifier` non distruttivo con `tolerance` weld (mirror + merge) + `SymmetryManager`/`SymmetryQmlBridge` per axis/offset/clip/merge

### Fase 3 — Animazione/rigging avanzato
- [x] `ControllerSystem` — controllers Noise/Spring/LookAt/Attachment (canali transform/material via SceneParamAccess; timer 20Hz; test `test_controller_system`; persistenza `.ks3d`)
- [x] Constraints estesi (Path, Attachment, Link, Spring) — ConstraintSystem completo (10/10 test)
- [x] Wire Parameters — `WireParameterSystem` binding parametro→parametro con mappa affine (test `test_wire_parameters`; persistenza `.ks3d`)
- [x] Skin Wrap — `SkinWrapSystem` bind per-vertice al cage più vicino (test `test_skin_wrap`; persistenza `.ks3d`)

### Fase 4 — Rendering e pipeline
- [x] Render elements (Z-depth, AO, diffuse, normal) — `RayTraceRenderer::render(cam,w,h,pass,samples)` + selettore pass overlay + export AOV da `rayTraceRenderToFile` (test `test_ray_trace` 7/7)
- [ ] Export Alembic / USD
- [x] Material node editor — `NodeMaterialEditor` + `MaterialNodePanel.qml` (test `test_node_material_editor` 8/8)
- [x] Light Lister + IES photometric — `LightSystem` + `LightPanel.qml` (test `test_light_system` 5/5, luci multi-light nel tracer)
- [ ] Bake to texture → ✅ `TextureBaker` + `BakePanel.qml` (test `test_bake_texture` 8/8)

---

## 6. Confronto filosofico: DCC generalista vs tool specializzato

| Aspetto | 3ds Max (DCC) | ksModeler (Tool) |
|---------|--------------|------------------|
| **Pubblico** | Generalista (games, arch-viz, VFX) | Automotive / Assetto Corsa / track building |
| **Pipeline** | Aperta (plugin, SDK, formati universali) | Chiusa ma integrata con AC (KN5, AI line, GPX/KML) |
| **Modifier stack** | 100+ modificatori | Copertura selezionata (~15) |
| **Rendering** | Third-party (Arnold/V-Ray) | Path tracer interno + PBR viewport |
| **Animazione** | Controller-based, completo | Keyframe + layer + NLA, su misura AC |
| **Rigging** | Biped/CAT/Skin/Physique | Skeleton Humanoid/Biped + weight paint |
| **FX** | Particle Flow, MassFX, Hair&Fur, Cloth | ICE, rigid body, cloth, hair |
| **Costo** | $1.785/anno (subscription) | Open source / integrato in ksEditor |
| **Target** | Contenuti per qualsiasi motore | Content pipeline per AC/EVO |

### Strategia consigliata
3ds Max è il DCC generalista per eccellenza: replicarne ogni singola feature non è l'obiettivo. La strategia ottimale è:

1. **Adottare i workflow 3ds Max più richiesti** — Layer system, Shell, Bridge poligonale, smoothing groups
2. **Mantenere il focus AC** — KN5, AI line, track/car builder restano il differenziale
3. **Espandere lo stack modificatori** sui casi più comuni (Taper/Wave/Noise/Push/Relax/Lathe)
4. **Interop formati** — Alembic/USD per pipeline più ampie
5. **UX** — smooth sub-object editing (Border/Element) e controller-based animazione come ponte verso l'uso professionale

---

## 7. Note implementative

### Layer system
- In Max i layer sono la gerarchia di organizzazione primaria (visibilità/gelo per layer)
- Modello: `QMap<QString, LayerDef>` + campo `layerName` in `SceneObject`
- La visibilità/gelo va applicata nel bridge e rispettata dai 4 viewport e dall'outliner

### Shell / spessore
- Estrusione lungo normali vertici, con divisione per facce adiacenti a concavità
- Algoritmo base: offset vertici lungo normale mediata, retopologia dei bordi aperti
- Utilizzabile anche per solidificare superfici da NURBS

### Bridge poligonale
- Serve pick di due loop di spigoli (riusa `findEdgeLoop`)
- Costruzione di quads tra i due loop con distribuzione uniforme e `segments` intermedi
- Beneficio immediato per modellazione di tubi/condotti e chiusura di fori

### Smoothing groups
- In Max gli smoothing groups sono 32 bit per faccia; ksModeler può usarne un subset
- Autosmooth: assegna lo stesso gruppo alle facce con angolo diedro < threshold
- Necessario per import/export KN5 con shading a facce (in AC i smoothing groups contano)
- **ICE Particle System** — nodi grafici per emitter/forze/colore/variation; `Branch/Switch/Loop` evaluati; `iceSetCollisionObject/iceSetEmitterObject`; bake cache; test `test_ice_particle` 7/7

### Export Alembic/USD
- OpenUSD o Alembic via plugin opzionale (non come dipendenza core)
- Priorità bassa rispetto ai gap di modeling ma necessaria per interop con tool esterni

### Render elements
- Il `RayTraceRenderer` già genera il frame completo; separare i pass (albedo, normal, depth, AO) è un'estensione naturale senza nuovo architettura
- Depth: distanza `h.t` normalizzata su `farDist = |eye−target|·0.5 + 2·diagonale scena`, vicino = chiaro; AO: 16 raggi cosine-hemisphere (raggio = 0.15·diagonale), spazio aperto = bianco; Normal: world-space `N·0.5+0.5`; sfondi/ground procedurali esclusi dai pass AOV (rispettivamente nero/bianco/grigio neutro)
- Il selettore pass (Color/Depth/AO/Diffuse/Normal) vive nell'overlay raytrace; l'export (`rayTraceRenderToFile` con `pass>0`) sovracampiona anche i pass AOV

### Light Lister / IES
- Modello dati `LightSystem::LightDef` keyed per `SceneObject::Type::Light`; direzione luce = asse locale -Z trasformato in world (convenzione forward del codebase); luci non illuminano i viewport Quick3D/Vulkan (costanti hard-coded), solo il path tracer — gap residuo documentato
- IESNA LM-63: parser testuale che salta header e `TILT=NONE`, rifiuta `TILT=INCLUDE`; curva verticale normalizzata a 91 campioni (media sugli angoli orizzontali); il tracer campiona la curva con interpolazione lineare e `iesIntensity` come moltiplicatore
- Fallback: senza luci registrate il tracer mantiene il sole/ambiente classico (nessuna regressione sul look precedente e sui test legacy)
