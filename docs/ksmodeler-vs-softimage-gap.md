# ksModeler vs Softimage (XSI) — Gap Analysis e Roadmap

Data: 2026-08-07
Scope: confronto tra il modulo 3D Modeler di ksEditor (`ksModeler`) e Autodesk Softimage (XSI).

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
- Boolean (union / difference / intersect)
- Symmetry (X/Y/Z, offset, weld, merge)
- Flip / Recalc Normals

### Modificatori e Deformazioni
- Simple Deform (Twist / Bend / Stretch)
- Lattice, Cage Deform
- LOD, Collision Mesh, Render Optimizer
- Geometry Nodes (graph)
- Modifier Ex (Wireframe / Skin / Displace)

### Rigging e Skinning
- Scheletro (Humanoid / Biped / FKChain)
- Add/Remove Bone, Bind to Skeleton, auto-weights, smooth skinning
- Weight painting (paint/mirror/prune/normalize)
- FK / IK, pose apply

### UV e Texture
- Unwrap LSCM, Project (planar), Mark Seam
- Translate/Rotate/Scale UV, Auto UVs, Pack Atlas
- Texture painting, Material Editor PBR (albedo/metallic/roughness/normal/emissive/opacity)
- Materiali procedurali (marble/wood/cement/asphalt/grass/metal/carbon/plastic/rust/grunge)

### Animazione
- Timeline base con playbar, play/pause, loop
- Keyframe pos/rot per oggetto, shape keys animati
- Interpolazione (easing), selezione animazione

### Viewport
- 4 viewport (Top / Front / Right / User) con toggle quad/single
- Modalità display: Shaded / Wireframe / Textured / X-Ray
- Grid texturizzato, assi colorati, gizmo
- Orbit / Pan / Zoom

### Import / Export
- Import: KN5, FBX, GLB/glTF, OBJ, STL
- Export: KN5, FBX, GLB, OBJ, STL
- Scene: New/Open/Save, progetto ks3d
- `.ks3d`: formato binario con sezioni fisse (header/stringhe/materiali/mesh/texture/nodi) + trailer opzionale "aux JSON" (offset/size nei campi `reserved[0/1]` dell'header) che persiste curve, F-curve, stack modificatori/boolean e sistemi ICE, keyed per nome oggetto (gli id vengono riassegnati al load)

### Gestione Scene
- Outliner (scene model, objectCount, selectedCount)
- Properties panel (pos/rot/scale, visibilità)
- Stats (totalVertices / totalTriangles)
- Camera e luce aggiungibili

### Altro
- Undo/Redo (CommandHistory), shortcut remappabili
- Python scripting, Command Palette
- Wizard (Track / Character / Car)
- Track builder, Car builder, Character builder

---

## 2. Gap rispetto a Softimage (XSI)

### 2.1 Modeling non-destructive  [PRIORITÀ 1]
- **Stack di modificatori non-destructive** — in XSI tutto è nello stack
  (subdiv, displace, mirror, deform) editabile in ogni momento senza
  "freeze". ksModeler ha modificatori ma applicati distruttivamente.
- **Subdivision Cage** ✅ — edit della cage con preview subdiv live.
- **Boolean non-destructive** — in XSI il Boolean è un factory con preview
  live e parametri editabili; ksModeler ha boolean apply-and-forget.

### 2.2 Curve e NURBS  [PRIORITÀ 2]
- Niente curve/NURBS: mancano loft, sweep, revolve, surface, rail,
  extrude along path. Fondamentale per flusso automotive.
- Continuità delle curve (CV) ✅ — C0/C1/C2 per curva selezionata.

### 2.3 Tools di editing raffinati  [PRIORITÀ 3]
- **Multi-Cut / Edge Slide / Vertex Slide** — in ksModeler Knife e Loop Cut
  sono placeholder UI (impostano `activeTool` ma non chiamano il bridge).
- Edge loop/ring tools avanzati ✅ — selezione loop/ring su mesh a quadri con highlight nel viewport, bridge; quad remesh ✅ (convertitore triangoli→quad-dominant non distruttivo con densità 0–3 e **live preview** interattivo nel viewport).

### 2.4 Animazione editoriale  [PRIORITÀ 4]
- **F-Curve / Dope Sheet editor** ✅ — `FCurveSystem` (`FCurveSystem.h/.cpp`) + 19 punti di accesso fcurve nel bridge (`fcurveAdd/Remove/...`, editor curve/curve editor); `FCurveEditorPanel.qml` (overlay `fcurveOverlay`, ribbon "F-Curves" cmd `fcurve`, entry `resources.qrc`).
- **Animation Layers** ✅ — blend layer non-destructive in stile XSI: ogni
  animazione ha `QVector<AnimationLayer>` (nome, enabled, weight 0..1, keyframe
  assoluti) blendati sulla base pose in `applyPoseToBones` (slerp per le
  rotazioni). API bridge: `animationAddLayer/RemoveLayer/RenameLayer/
  SetLayerEnabled/SetLayerWeight/LayerCount/LayerList/LayerKeyframes/
  AddLayerKeyframe` + `addKeyframeForSelectedObjectToLayer`. UI: barra
  "LAYERS" nella `AnimationTimeline.qml` (nome, on/off, weight slider, %
  display, record pose, rimuovi). Persistenza in aux JSON `animations`
  (durata + keyframe base + layer con relative keyframe), round-trip in
  `.ks3d` (prima le animazioni non venivano salvate). Segnale
  `animationLayersChanged`.
- **Constraints** ✅ — point/orientation/parent/aim (world-space): `ConstraintSystem` (`ConstraintSystem.h/.cpp`, enum `ConstraintType`, struct `ConstraintDef` con offset/offsetRot/enabled), bridge `constraintAdd/Remove/SetEnabled/SetOffset/Evaluate/List` + timer 20 Hz `constraintEvaluateAll` quando attivo; UI `ConstraintsPanel.qml` (overlay `constraintsOverlay`, ribbon "Constraints" cmd `constraints`, entry `resources.qrc`).
- **Nonlinear animation** ✅ — NLA clip/source su master timeline: ogni `NLAClip {name, sourceAnim, start, duration, timescale, loop, enabled, weight}` riferisce un'animazione esistente; `applyNLAPose()` valuta i clip attivi che coprono `nlaTime` (loop/fmod per il loop, altrimenti clamp) e fonde lerp/slerp col weight del clip (payload: `computeAnimationPose` condivisa = base pose + blend layer). API bridge `nlaAddClip/RemoveClip/SetClipRange/SetClipTimescale/SetClipLoop/SetClipEnabled/SetClipWeight/ClipList/Play/Pause/Stop/SetTime/Time/Playing/Duration`; segnali `nlaChanged`/`nlaTimeChanged`; timer NLA 30 Hz. UI `NLAPanel.qml` (strips sul timeline, editor clip: start/durata/timescale/loop/enabled/weight, transport); ribbon "NLA" cmd `nla` + overlay `nlaOverlay`. Persistenza aux JSON top-level `nlaClips` (read/write `.ks3d`), pulito in `newProject`.
- **Walk-cycle generator** ✅ — `generateWalkCycle(animName, duration, amplitude)`: keyframe procedurali loopable su skeleton Humanoid (thigh/shin/foot .L/.R, upper_arm/forearm, root/spine/chest/head): swing gambe sfasato 180°, piega ginocchio, flessione piede, braccia opposte alle gambe, bob+twist del torso, contro-rotazione testa; rigenera pulendo i keyframe esistenti; UI nella barra "WALK CYCLE" di `AnimationTimeline.qml` (nome/durata/ampiezza).
- Import motion capture ✅ — `importBVH(path, animName)` (bridge): parser Biovision BVH (`BvhImporter.h/.cpp`) per gerarchia + canali motion; mappa i joint BVH sullo skeleton Humanoid esistente per nome (Left/Right→`.L/.R`, UpLeg/Leg/Foot/Arm/ForeArm/Spine/Neck/Head…) e, se non trova corrispondenze, ricostruisce uno skeleton dalla gerarchia BVH; genera keyframe per ogni frame (rotazioni in ordine di canale BVH convertite in euler Qt, posizioni dal root); crea/aggiorna l'animazione (durata = frameCount·frameTime); ribbon "Mocap" cmd `ac_mocap` + `mocapImportDialog`.

### 2.5 FX e Dynamics  [PRIORITÀ 5]
- **ICE** (Interactive Creative Environment) — sistema particle/effetti
  node-based ✅ (27 evaluators: Emitter Point/Sphere/Mesh/Circle, Forces, Collisions, Filtri, operatori, Prop, Output; bake cache, graph editor UI, instancing, persistenza `.ks3d`).
- Rigid/soft body dynamics ✅ — rigid body sim con Bullet (Box/Sphere/ConvexHull, kinematic/static/dynamic) + **cloth soft body** ✅ (`ClothSystem`, Verlet) + **collisioni cloth** ✅ (solid mesh objects via triangoli world-space con spatial grid, self-collision pairwise) + **hair/fur** ✅ (`HairSystem`: strand da superficie mesh campionata per area, dinamica Verlet pinnata, render a ribbon cards) + **fabric presets** ✅ (Cotton/Silk/Denim/Leather/Rubber/Wool/Satin); rimane hair styling avanzato e tessuti procedurali.

### 2.6 Rendering  [PRIORITÀ 6]
- Integrazione renderer esterno (in XSI mental ray).
- **IBL/HDRI** ✅ — `Modeler.environmentHDR` (Q_PROPERTY String, `setEnvironmentHDR`): i 4 `SceneEnvironment` dei viewport passano a `backgroundMode: SkyBox` + `lightProbe` (Texture `.hdr`/`.exr`) quando impostato; tab ribbon "Render" (HDRI Env / Clear HDRI) + `hdrImportDialog`.
- **Tonemapping / Exposure** ✅ — `tonemappingMode` (0 None / 1 Linear / 2 ACES / 3 HejlDawson / 4 Filmic, mappati sui valori `SceneEnvironment.TonemappingMode`) e `tonemapExposure` (0.05–8, default 1.0) nel bridge; i 4 `SceneEnvironment` dei viewport applicano `tonemappingMode`+`exposure`; controlli nella sezione Rendering di `ViewportPanel.qml` (ComboBox + slider).
- **Raytracing in viewport** ✅ — preview CPU ray-traced (sole + ambient + ombre dure + speculare) sostituibile al viewport shaded.
- **Render esterno stile mental ray** ✅ — `rayTraceRenderToFile(path, w, h, samples)`: path tracing CPU (renderer `RayTraceRenderer::renderFinal`): sole come area light (ombre morbide, 4° spread), un bounce diffuso indiretto (emisfero cosine-weighted), max depth 3, anti-aliasing `samples²` raggi/pixel con jitter, tonemap filmico (`1−e⁻x`) + gamma 2.2, esportazione PNG; ribbon "Render" → "Render File" cmd `ac_renderfile` + `renderFileDialog` (1280×720, 8 samples).
- ksModeler ha solo PBR base (shaded viewport).

### 2.7 Workflow e Scene  [PRIORITÀ 7]
- **Selection sets** ✅ — insiemi nominati di oggetti: `m_selectionSets` (nome set → nomi oggetto) nel bridge, `createSelectionSet`/`addSelectionToSet`/`recallSelectionSet`/`clearSelectionSet`/`deleteSelectionSet`/`renameSelectionSet`; multi-select reale (Shift/Ctrl+click nell'outliner → `toggleSelectObject`; `selectObject`/`deselectAll`/`selectAll` ora aggiornano i flag `isSelected`; `m_sceneModel->refresh()` dopo i cambi di selezione per l'highlight live); pannello `SelectionSetsPanel.qml` (overlay `selectionSetsOverlay`, ribbon "Selection Sets"); persistenza in `.ks3d` aux JSON (`selectionSets`).
- **Families / rig hierarchy** ✅ — `SceneObject` ha già parent/child + `worldTransform` + serializzazione gerarchica (`children`); aggiunti `groupSelected(name)`, `ungroupSelected()`, `reparentObject(childId, parentId)`, `parentObjectId(id)` nel bridge con **preservazione del world transform** (decomposizione TRS da `worldParent⁻¹·worldChild`); accessori world `worldPosition()`/`worldRotationEuler()`/`worldScale()` su `SceneObject`; nuovi ruoli `objectWorldPosition/objectWorldRotation/objectWorldScale/objectDepth/childCount` in `SceneObjectListModel`; i 4 viewport renderizzano ora in **world transform** (i figli seguono i padri); outliner con indentazione per profondità + bottoni Group/Ungroup. Resta: factory-based scene, families XSI-style (gruppi con master).
- **Viewport culling avanzato** ✅ — Quick3D fa già frustum culling per-Model; aggiunto culling per distanza per-viewport: `cullingEnabled`/`cullDistance` nel bridge, `cullVisible(model, camPos)` in `page_ksModeler.qml`, pannello `ViewportPanel.qml` (overlay `viewportOverlay`, ribbon "Viewport").
- **Camera matching** ✅ — `cameraFocalLength`/`cameraSensorWidth` (+ `cameraFov` verticale calcolato, formula 36/24mm 3:2) nel bridge; la PerspectiveCamera del viewport usa `fieldOfView: Modeler.cameraFov`; `matchCameraToSelection()` allinea l'orbita a un Camera object selezionato; controlli lens nel pannello Viewport + ribbon "Match Camera".
- **Factory-based scene** ✅ — template parametrizzati stile XSI: `factoryNames()/factoryCreate(name,p0,p1,p2)/factorySaveFromSelection(name)/factoryDelete(name)/factoryParams(name)/factoryUserCount()` nel bridge; factory built-in (Box/Sphere/Cylinder/Cone/Torus/Plane con size, Camera, Light, Transform Group) + factory utente che catturano selezione (mesh serializzata + materiale + scala) e vengono istanziate da `factoryCreate`; pannello `FactoryPanel.qml` (overlay `factoryOverlay`, ribbon "Factories"); persistenza user factories in `.ks3d` aux JSON (`factories`), pulito in `newProject`/restore.

---

## 3. Gap minori / placeholder noti

Rilevati durante l'analisi:

| Area | Problema |
|---|---|
| Snapping | ✅ Risolto: `setSnapEnabled`/`setSnapIncrement` nel bridge; `translateSelected`/`rotateSelected` quantizzano a incremento; toggle status bar + preset "Inc" |
| Export | ✅ STL: `importSTL`/`exportSTL` esposti nel bridge e inclusi in `importFile`/`exportFile`; filtri dialog aggiornati. Restano non esposti DAE/DXF/KS3D (KS3D già gestito da `saveFile`/`loadFile`) |
| Mark Seam | ✅ Risolto: `markSeamFromClosestEdge`/`clearSeams`/`seamEdgeCount` nel bridge; toggle `seamMode` nel viewport (ribbon "Mark Seam", click sugli spigoli); seam memorizzate per oggetto (`m_seamEdges`) e merge in `unwrapUVs` (LSCM: angle-based + seam manuali) |
| Sculpt | ✅ Risolto: `SculptPanel.qml` (9 brush reali `sculptBrush` — Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate — radius/strength) + drag nel viewport. I bottoni legacy in `modeler_Tools.qml` (Draw/Smooth/...) impostano solo `activeTool` ma il pannello è codice morto (ModelerStudio mai istanziato) |
| Tool AC | ✅ Risolto: tab ribbon "AC" nella pagina attiva + pannello `ACPanel.qml` (overlay `acToolsOverlay`) con width/camber/smooth/terrain e i 6 tool reali; FileDialog per import/export GPX/KML e export AI line |

---

## 4. Priorità consigliate

1. **Stack modificatori non-destructive** — base del workflow XSI, abilita
   undo-friendly editing di tutto il resto. ✅
2. **Curve / NURBS modeling** (loft/sweep/revolve) — critico per automotive. ✅
3. **F-Curve / Dope Sheet editor** — per animazione reale. ✅
4. **Boolean con preview live** + boolean non-destructive. ✅
5. **Multi-Cut / Edge Slide reali** — Loop Cut, Knife, Vertex/Edge Slide, Multi-Cut interattivo. ✅
6. **Sculpt interattivo nel viewport** (pick → brush) — 9 brush + Multi-Cut. ✅
7. **Constraints** (point/orientation/aim/parent) per il rigging avanzato. ✅
8. **ICE-like particle system** — node-graph completo: 19 evaluators, bake cache, mesh emitter/collision. ✅
9. **Tool AC** — ribbon "AC" + pannello (width/camber/terrain), generate Track Mesh (ribbon UV+camber), AI Line + export formato AC, import/export GPX/KML con dialog. ✅
10. **Selection sets (2.7)** — multi-select + insiemi nominati persi nel `.ks3d`. ✅
11. **Families / rig hierarchy + culling + camera matching + factories (2.7)** — gerarchie con world transform, culling per distanza, lens simulation, factory templates. ✅

---

## 5. Definizione di "done" per la roadmap

### Stack modificatori non-destructive
- [x] Modello dati `ModifierStack` in C++ con lista ordinata di modificatori
- [x] Ogni modificatore ha parametri editabili + flag enable/visible
- [x] Preview in viewport senza freeze (evaluate() riapplica i modifier al base mesh)
- [x] Applicazione "freeze/bake" che converte il risultato in mesh statica
- [x] UI: pannello Stack `ModifierStackPanel.qml` (Add/Remove/Reorder/Enable/Params/Freeze/Clear)
- [x] Persistenza stack nel `.ks3d` (aux JSON: base mesh + mods con parametri, ri-evaluated al load)
- [x] Undo/redo delle operazioni sullo stack (`ModifierStackCommand` snapshot/restore; `undo()/redo()` del bridge ora delegati a `CommandHistory`)

### Curve / NURBS
- [x] Rappresentazione curva (CV, knot, degree) in C++ (`CurveData`/`CurveSystem`)
- [x] Primitive curva: Line, Arc, Circle, Bezier (+ BSpline)
- [x] Operatori: Loft, Sweep, Revolve, Rail (+ tessellazione polilinea)
- [x] Conversione curva → mesh poligonale (tassellazione + ribbon `curveToMesh`)
- [x] Editing CV nella viewport con gizmo (marcatori sfera selezionabili in vpUser, gizmo Move sul CV selezionato, `translateSelectedCV`/`curveSelectedCV`/`curveCvEdit` nel bridge)
- [x] Persistenza curve nel formato `.ks3d` (aux JSON, key per nome oggetto; oggetti Spline ora serializzati)
- [x] UI: `CurvePanel.qml` (primitive, lista CV, edit CV, To Mesh/Loft/Revolve/Sweep/Rail)

### F-Curve / Dope Sheet
- [x] Modello dati `FCurve` (`FCurveSystem.h/.cpp`: keys {frame,value,interpolation,inTangent,outTangent,locked}, canali per proprietà, evaluation Hermite/Ease/Linear/Step)
- [x] Editor F-Curve con pan/zoom (wheel), selezione/multiselezione key (Shift), drag key
- [x] Dope Sheet (righe = canali, colonne = frame) con scrub ruler e drag diamond
- [x] Interazione: add (doppio click / tasto Add), remove, move, easing presets (Linear/Step/Cubic/EaseIn/EaseOut/EaseInOut)
- [x] Collegamento a `Modeler.animationTime` (scrub + `fcurveApplyToObject` live) e playback dedicato `fcurvePlayPause` (timer indipendente dalle bone animations)
- UI: `FCurveEditorPanel.qml` (overlay `fcurveOverlay`, ribbon "F-Curves")
- Note: dati F-Curve persistiti nel `.ks3d` (aux JSON, key per nome oggetto); tangent handle editing non ancora esposto (presets sufficienti)

### Boolean non-destructive  [PRIORITÀ 4]
- [x] Modello dati `BooleanStack` in C++ (`BooleanStack.h/.cpp`) con lista ordinata di operazioni {operation, operandId, operandName, enabled} + base mesh catturata al primo add
- [x] Operazioni Union / Difference / Intersection / Xor in sequenza, editabili (cycle type, enable/disable, reorder, remove)
- [x] Preview live in world-space (base + operands trasformati da `worldTransform`, risultato riportato in local) senza bake
- [x] Live update automatico al move/rotate/scale di base o operands (`SceneObject::transformChanged` ora emesso nei setters + subscribe nel bridge)
- [x] Bake "Apply" (converte il risultato in mesh statica, droppa lo stack) e "Clear" (ripristina la base mesh)
- UI: `BoolOpPanel.qml` riscritto come editor di stack (overlay `boolOpOverlay`, ribbon "Bool Ops")
- Note: `SceneObject::setPosition/setRotationEuler/setScale` ora emettono `transformChanged()` (aggiunta sicura al core); operands cancellati → op saltata; stack persistito nel `.ks3d` (aux JSON: base mesh + ops, operands risolti per nome)

### Tools di editing raffinati  [PRIORITÀ 5]
- [x] **Loop Cut reale** — `MeshOperations::loopCut(mesh, axis, factor, slide)`: edge-splitting sul piano perpendicolare all'asse locale scelto, re-tassellazione poligoni (quad→2 quad, n-gon→fan), insert di edge loop
- [x] **Loop Cut & Slide** — il parametro `slide` (-1..1) muove il loop appena inserito lungo gli edge di supporto (sostituisce il placeholder `subdivideFaces`)
- [x] **Knife reale** — `knifeCutSelected(objectId)` taglia lungo la diagonale del bounding box (sostituisce la chiamata hardcoded `knifeCut(0,0,0,1,1,1)`)
- [x] Vertex/Edge Slide su elemento selezionato (sub-object picking: `findClosestVertex/Edge` + `vertexSlide/edgeSlide` in `MeshOperations`, bridge, UI panel con factor slider)
- [x] Multi-Cut interattivo in viewport (screen-space pick) — `knifeCutWorld` + `cutMode`/`cutStart` marker in vpUser
- UI: `CutToolsPanel.qml` (asse/factor/slide slider + knife bbox, overlay `cutToolsOverlay`, ribbon "Cut Tools"); bottoni Loop Cut/Knife del pannello Tools ora chiamano le op reali

### Sculpt interattivo  [PRIORITÀ 6 — completato]
- [x] **Sculpt Brush reale** — `MeshOperations::sculptBrush(mesh, center, radius, strength, mode, drag, previousCenter)` con 9 brush in-place: 0 Draw (push lungo normale), 1 Smooth (media dei vicini via adjacency), 2 Grab (trascina vertici con falloff), 3 Flatten (verso piano attraverso il centro), 4 Crease (incavo), 5 Inflate (push lungo normale vertice mediata, boost al centro), 6 Pinch (attrae verso il centro brush), 7 Smear (trascina la superficie col movimento del cursore, `center - previousCenter` con fallback `drag`), 8 Negate (inverso di Draw). Ricalcola normals+bounds; ritorna il numero di vertici toccati; test `testExtendedSculptBrushes`
- [x] **Bridge** — `sculptBrush(objectId, wx, wy, wz, radius, strength, mode, dx, dy, dz, px, py, pz)`: converte hit point mondo→locale (`worldTransform().inverted()`), raggio diviso per scala media (handle non-uniforme approx), drag via `inv.mapVector`, precedente centro stroke mondo→locale per Smear, scrive la mesh + `sceneChanged()`
- [x] **UI** — `SculptPanel.qml` (9 bottoni brush, slider radius/strength, overlay `sculptOverlay`, ribbon "Sculpt" cmd `sculpt`); `page_ksModeler` userMouseArea gestisce il drag: `vpUser.pick()` → `scenePosition` (fallback `position`) → chiama il brush a ogni movimento con delta per Grab e precedente posizione per Smear
- [x] **Multi-Cut interattivo** — `knifeCutWorld(objectId, sx,sy,sz, ex,ey,ez)` nel bridge: converte i 2 punti world→local (`worldTransform().inverted()`) e chiama `MeshOperations::knifeCut`; in `page_ksModeler` `cutMode`/`cutStart` + marker rosso (`cutStartMarker` sfera in vpUser): 1° clic piazza l'inizio, 2° clic taglia; toggle nel pannello Cut Tools
- Note: sculpt funziona sul pannello dedicato (il pannello Tools in `ModelerStudio.qml` non è caricato dalla pagina attiva); sottogruppo legacy `activeTool` in `modeler_Tools.qml` non toccato; serve mesh densa (Subdivide) per dettaglio fine

### Curve / NURBS  [PRIORITÀ 2 — completato]
- [x] **Curve primitives** — Line, Bezier, BSpline, Arc, Circle (`addCurve` + `CurvePrimitives`)
- [x] **CV editor** — lista CV, add/remove/update, campos X/Y/Z per CV selezionato (`curveUpdateCV`, `curveAddCV`, `curveRemoveCV`)
- [x] **Editing CV nel viewport** — checkbox "Edit CVs in viewport" nel `CurvePanel` (`setCurveCvEdit`), marcatori sfera `cvN` pickable in vpUser (`curveCvPositions`), clic = selezione CV (`curveSelectedCV`), gizmo Move agganciato al CV (`translateSelectedCV`, `gizmoPosition` segue il CV); snap grid rispettato
- [x] **To Mesh** — `curveToMesh` (ribbon width + segments)
- [x] **Revolve** — `curveRevolve` (profile + angle + steps, asse Y)
- [x] **Loft** — `curveLoft` (2+ curves, segments)
- [x] **Sweep** — `curveSweep` (profile + path, segments)
- [x] **Rail** — `curveRail` (2 rails + profile, segments)
- UI: `CurvePanel.qml` (primitives, CV list editor, surface buttons, overlay `curveOverlay`, ribbon "Curve Editor" cmd `curves`)

### Vertex/Edge Slide  [PRIORITÀ 5b — completato]
- [x] **MeshOperations** — `findClosestVertex(mesh, worldTransform, worldPoint)`, `findClosestEdge(...)`, `vertexSlide(mesh, vertexIndex, targetWorld, worldTransform)`, `edgeSlide(mesh, v0, v1, factor)` in `MeshOperations.h/.cpp`
- [x] **Bridge** — `findClosestVertex/Edge`, `vertexSlide`, `edgeSlide` Q_INVOKABLE in `3DModelingQmlBridge.h/.cpp`
- [x] **UI** — `VertexEdgeSlidePanel.qml` (mode Vertex/Edge, "Pick" button, edge slide factor slider [-1,1], overlay `vertexEdgeSlideOverlay` 320×500, ribbon "Vert/Edge Slide" cmd `vertexedgeslide`); viewport User pick delega al panel quando overlay visibile e `picking=true`; entry `resources.qrc`

---

### Prossimi passi (roadmap rimanente)

### NLA (Animazione editoriale 2.4)  [PRIORITÀ 4 — completato]
- [x] Clip/source non lineari: struct `NLAClip {name, sourceAnim, start, duration, timescale, loop, enabled, weight}` + `QVector<NLAClip> m_nlaClips`; `applyNLAPose()` valuta i clip che coprono `m_nlaTime` sulla master timeline e fonde lerp/slerp col weight
- [x] Core riusato: `computeAnimationPose(anim, time, out)` estratta da `applyPoseToBones` (base pose + blend layer) e riusata da timeline editor e NLA
- [x] Bridge: `nlaAddClip/RemoveClip/SetClipRange/SetClipTimescale/SetClipLoop/SetClipEnabled/SetClipWeight/ClipList/Play/Pause/Stop/SetTime/Time/Playing/Duration`; segnali `nlaChanged`/`nlaTimeChanged`; timer 30 Hz
- [x] UI: `NLAPanel.qml` (strips sul timeline master, editor clip con start/durata/timescale/loop/enabled/weight, transport play/pause/stop + scrubber); overlay `nlaOverlay` + ribbon "NLA" cmd `nla`; entry `resources.qrc`
- [x] Persistenza aux JSON top-level `nlaClips` (round-trip `.ks3d`), pulito in `newProject`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Animation Layers (Animazione editoriale 2.4)  [PRIORITÀ 4 — completato]
- [x] Blend layer non-destructive (XSI-style): `AnimationLayer {name, enabled, weight, keyframes}` + `QVector<AnimationLayer>` per animazione; blending in `applyPoseToBones` (lerp posizione + slerp rotazione, applicato in sequenza)
- [x] Bridge: `animationAddLayer/RemoveLayer/RenameLayer/SetLayerEnabled/SetLayerWeight/LayerCount/LayerList/LayerKeyframes/AddLayerKeyframe` + `addKeyframeForSelectedObjectToLayer`; segnale `animationLayersChanged`
- [x] UI: barra "LAYERS" in `AnimationTimeline.qml` (nome, on/off, weight %, record, rimuovi)
- [x] Persistenza animazioni (base + layer) in aux JSON `.ks3d` top-level `animations` — prima le animazioni non venivano salvate
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### ICE-like particle system  [PRIORITÀ 8 — completato]
- [x] **Core node-graph** — `ICEParticleSystem.h/.cpp`: `ICEParticleNodeType::Type` (emitter/force/collision/filter/operator/property/output/flow), `ICEParticleGraph` (QMap nodes + connections, topological sort `getEvalOrder`, serializzazione toJson/fromJson), `ICEParticleState` (SoA: positions/velocities/accelerations/ages/lifetimes/sizes/colors/masses/ids, allocate/kill/compact), `ICEParticleEvaluator` (setGraph, step, registerNodeType)
- [x] **Node evaluators (27)** — `ICE.EmitterPoint` (con `velocity`), `ICE.EmitterSphere`, `ICE.EmitterMesh` (emissione area-weighted dalla superficie, campionamento triangolo per area), `ICE.EmitterCircle` (disco sul piano XZ, raggio, velocità radiale + spread verticale), `ICE.ForceGravity`, `ICE.ForceWind`, `ICE.ForceTurbulence`, `ICE.ForceDrag`, `ICE.ForceVortex` (tangenziale attorno ad asse), `ICE.ForceAttractor` (repulsore se strength<0, caduta 1/d), `ICE.CollisionPlane`, `ICE.CollisionSphere`, `ICE.CollisionMesh` (raycast Möller–Trumbore con **spatial hash** uniforme per broadphase), `ICE.FilterAge`, `ICE.FilterVelocity`, `ICE.FilterRandom` (kill con probabilità 0–1), `ICE.FilterPosition` (AABB min/max, kill all'interno o all'esterno con `killInside`), `ICE.OpAdd` (integrazione), `ICE.OpMultiply`, `ICE.OpLerp`, `ICE.OpVectorMath` (add/scale/reflect/set), `ICE.OpCurve` (falloff curva su velocità/accelerazione lungo la vita, smooth fade), `ICE.PropColor`, `ICE.PropSize`, `ICE.PropLifetime`, `ICE.PropMass`, `ICE.OutputPoints`, `ICE.OutputRibbons`/`ICE.OutputMesh` (alias di OutputPoints)
- [x] **Bridge** — `iceCreate` (default Emitter→Gravity→Integrate→Output), `iceRemove`, `iceAddNode` (defaults per tipo), `iceRemoveNode`, `iceSetNodeProperty`, `iceConnect`, `iceGetGraph`, `iceGetPositions`, `iceGetColors`, `iceGetSizes`, `iceGetAliveCount`, `icePlayPause`, `iceStopAll`, `iceSetCollisionObject` (triangle soup world-space via `updateCollisionTriangles` ogni tick), `iceSetEmitterObject` (idem per emitter mesh), `iceBake(frames)` (simula offline, cache snapshot per frame), `iceScrubToFrame(frame)` (ripristina snapshot), `iceCacheLength`, `iceClearCache`; timer per-step 16ms (~60fps) con `iceChanged(int)` + `iceParticlesUpdated(int,int)`; cleanup dtor/newProject
- [x] **Rendering** — `ParticlePointsGeometry.h/.cpp` (`QmlParticlePointsGeometry` : QQuick3DGeometry, `PrimitiveType::Points`, stride pos+colore, registrata `ksEditor.Modeler` `ParticlePointsGeometry`); modello `icePoints` in vpUser consuma posizioni+colori+sizes per-particella (`iceGetColors/iceGetSizes`) aggiornati su `onIceParticlesUpdated`
- [x] **UI** — `ICEPanel.qml`: Create/Play/Pause/Remove System, alive count, lista nodi selezionabili, editor properties (numero e vec3 X/Y/Z), **Bake cache** (TextField frames + scrub Slider + Clear), Collision Object ComboBox, Emitter Object ComboBox, 27 tipi di nodo (tra cui Emitter Circle, Filter Random/Box, Curve Falloff, Set Lifetime/Mass, Output Ribbons/Mesh); overlay `iceOverlay` 320×600 (ScrollView) + ribbon "ICE" cmd `ice`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

**Priorità 8**
- [x] Persistenza dei sistemi ICE in `.ks3d` (aux JSON: grafo + collision/emitter object per nome)
- [x] Rendering come sfere istanced (instancing) — `QmlParticleInstancing` (`ParticleInstancing.h/.cpp`, registrata `ParticleInstancing` in `ksEditor.Modeler`): il modello `iceSpheres` in vpUser usa `source: "#Sphere"` + `instancing` con per-particella position/scale(size)/color; toggle "Render as spheres (instanced)" in ICEPanel → `Modeler.iceSpheresEnabled` (default on); il vecchio rendering a punti resta disponibile (`ParticlePointsGeometry`)
- [x] RNG deterministico per-sistema — `ICEParticleEvaluator::m_rng` (`std::mt19937`, seed via `setSeed`) al posto dei `static std::mt19937` condivisi in `evalEmitterMesh`/`evalEmitterSphere`/`evalForceTurbulence`; il bridge semina ogni sistema con `0x1CE0000u ^ objectId` (`iceCreate` e restore dal `.ks3d`)
- [x] UI node-graph visuale in `ICEPanel.qml` — Canvas 2D: nodi (rounded rect, titolo+tipo), connessioni bezier rosse, porta input gialla/output rossa; drag nodo = sposta (`iceSetNodePosition`), drag dalla porta rossa a un altro nodo = crea connessione (`iceConnect`), pulsante "Clear links" (`iceRemoveConnection`); click nodo seleziona e alimenta il property editor esistente

### Tool AC (Assetto Corsa)  [roadmap — completato]
- [x] **Tab ribbon "AC"** nella pagina attiva (`page_ksModeler.qml`): gruppo Track Tools (AC Tools / Track Mesh / AI Line / Smooth / Terrain) + gruppo Geo (Import/Export GPX/KML, Export AI); comandi `ac_trackmesh`, `ac_ailine`, `ac_smooth`, `ac_terrain`, `ac_gpximport`, `ac_kmlimport`, `ac_gpxexport`, `ac_kmlexport`, `ac_aiexport`, `ac_actools`
- [x] **Pannello `ACPanel.qml`** (overlay `acToolsOverlay`, stile come gli altri panel): slider Track Width (2–30), Camber (−0.06…0.06), Smooth iterations, Terrain size/height; pulsanti Generate Track Mesh / AI Line / Smooth / Terrain / Import-Export GPX·KML·AI; status "Points / Length"
- [x] **FileDialog** dedicati in `page_ksModeler.qml`: `gpxImportDialog`, `kmlImportDialog`, `gpxExportDialog`, `kmlExportDialog`, `aiExportDialog` (pattern FileDialog esistente); import chiama `importGPX`/`importKML` + `trackPointCount` nel messaggio
- [x] **Generate Track Mesh** (`3DModelingQmlBridge.cpp:3794`) riscritto su `MeshData` (niente più `geometry::Mesh3D`): ribbon chiuso con UV (u = lunghezza cumulativa normalizzata, v = 0/1) e camber (`m_trackCamber`, solleva lato esterno); `meshDataToSceneMesh` ora copia anche normal+uv (beneficio per tutto il codebase)
- [x] **Camber/Width** — `setTrackWidth`/`setTrackCamber` + getter `trackWidth`/`trackCamber` nel bridge (pattern `setTrackWidth` esistente)
- [x] **Export AI line** — `exportAILine(path)` scrive il formato testo AC `ai/*.ai` (`x,y,z,width,speed_m_s`), speed calcolata per punto dalla curvatura (rettilinei ~72 m/s, tornanti ~12 m/s)
- [x] **Export GPX/KML** — `exportGPX`/`exportKML` (inverso della conversione flat-earth degli import, origine = primo punto importato in `m_originLat`/`m_originLon`)
- [x] Import GPX/KML impostano `m_originLat`/`m_originLon` al primo punto (per un export coerente)
- Build Release OK + smoke test pulito (processo vivo)

### Selection sets (Workflow/Scene 2.7)  [roadmap — completato]
- [x] **Multi-select reale** — `selectObject(id)` ora aggiorna i flag `isSelected` (solo l'oggetto cliccato), `deselectAll` li azzera, `toggleSelectObject(id)` li alterna (Shift/Ctrl+click nell'outliner); `selectAll()` invariato; `m_sceneModel->refresh()` dopo ogni cambio di selezione per highlight live (il modello emette begin/endResetModel)
- [x] **Bridge** — `selectedObjectNames()`, `selectionSetNames()`, `selectionSetMemberCount()`, `createSelectionSet(name)` (cattura selezione corrente), `addSelectionToSet(name)`, `recallSelectionSet(name)` (seleziona i membri), `clearSelectionSet(name)`, `deleteSelectionSet(name)`, `renameSelectionSet(old,new)`; `QMap<QString, QSet<QString>> m_selectionSets`; segnale `selectionSetsChanged()`
- [x] **Persistenza** — aux JSON `.ks3d` chiave top-level `selectionSets` (nome set → array di nomi oggetto); letto in `restoreAuxMetadata`, pulito in `newProject`/restore
- [x] **UI** — `SelectionSetsPanel.qml` (TextField nome, Save Selection / Add Selected / Recall / Clear Members / Delete / Rename, ListView set con conteggio membri, status "Selected: N"); overlay `selectionSetsOverlay` + ribbon "Selection Sets" cmd `selectionSets`; entry `resources.qrc`
- [x] **Outliner** — click con Shift/Ctrl = toggle multi-select; "Select All" ora chiama `Modeler.selectAll()`; `Connections` su `selectionChanged`/`sceneChanged` per aggiornare `selectedCount`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Families / rig hierarchy (Workflow/Scene 2.7)  [roadmap — completato]
- [x] **World transform accessors** — `SceneObject::worldPosition()/worldRotationEuler()/worldScale()` (recompute del world matrix se dirty; euler ZYX estratto dalle colonne normalizzate per world scale)
- [x] **Bridge** — `groupSelected(name)` (crea Node gruppo e vi reparenta tutti i selezionati top-level), `ungroupSelected()` (sposta i figli del gruppo al nonno, elimina i nodi vuoti), `reparentObject(childId, parentId)` (con check anti-ciclo), `parentObjectId(id)`, `selectedTopLevelObjects()`; helper `reparentPreservingWorld` (decomposizione TRS da `worldParent⁻¹·worldChild` — la world transform del figlio resta invariata)
- [x] **SceneObjectListModel** — ruoli `objectWorldPosition/objectWorldRotation/objectWorldScale/objectDepth/childCount` (per viewport e outliner)
- [x] **Viewport** — i 4 Repeater renderizzano con world transform: i figli seguono i padri nella gerarchia (spostando un gruppo si muovono tutti i membri)
- [x] **Outliner** — indentazione per profondità (`objectDepth * 14`), bottoni Group/Ungroup (barra bottom), Delete invariato
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Viewport culling avanzato (Workflow/Scene 2.7)  [roadmap — completato]
- [x] **Bridge** — `cullingEnabled`/`cullDistance` (Q_PROPERTY + segnale `cullingChanged`), default OFF / 200 u
- [x] **QML** — funzione `cullVisible(model, camPos)` in `page_ksModeler.qml` (rispetta `objectVisible`, poi distanza dalla camera del viewport); i 4 Repeater la usano con la propria camera (`camTop/camUser/camFront/camRight`); il frustum culling Quick3D resta attivo per gli oggetti off-screen
- [x] **UI** — `ViewportPanel.qml` (CheckBox distanza + slider Distance 1–1000, status ON/OFF); overlay `viewportOverlay` + ribbon "Viewport" cmd `viewport`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Camera matching (Workflow/Scene 2.7)  [roadmap — completato]
- [x] **Bridge** — `cameraFocalLength` (5–200 mm, default 35) e `cameraSensorWidth` (4–72 mm, default 36, formato 3:2) con segnale `cameraChanged`; `cameraFov()` = FOV verticale da `2·atan((sensorH/2)/focal)`; `matchCameraToSelection()` allinea l'orbita (camTarget) al `Camera` object selezionato
- [x] **Viewport** — la PerspectiveCamera usa `fieldOfView: Modeler.cameraFov`
- [x] **UI** — slider Focal/Sensor + label FOV live + Reset Lens + bottone "Match Viewport to Selected Camera" nel `ViewportPanel.qml`; ribbon "Match Camera" cmd `camera_match`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Scene Factories (Workflow/Scene 2.7)  [roadmap — completato]
- [x] **Bridge** — `factoryNames()` (built-in + user), `factoryType(name)`, `factoryCreate(name, p0, p1, p2)` (built-in: Box size / Sphere r / Cylinder r,h / Cone r,h / Torus R,r / Plane w,h / Camera / Light / Transform Group; user: deserializza mesh + applica materiale + scala), `factorySaveFromSelection(name)` (cattura tipo + mesh serializzata + baseColor/metallic/roughness/opacity + scale), `factoryDelete(name)`, `factoryParams(name)` (info per la UI), `factoryUserCount()`; segnale `factoryChanged()`
- [x] **Persistenza** — user factories nel `.ks3d` aux JSON chiave top-level `factories` (tipo, nome oggetto, colore/mat, scala, mesh); lette in `restoreAuxMetadata`, pulite in `newProject`/restore
- [x] **UI** — `FactoryPanel.qml` (ListView factory con tipo colorato, campo size per built-in, Create, Save Selection as Factory, Delete User Factory, conteggio user); overlay `factoryOverlay` + ribbon "Factories" cmd `factories`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Tonemapping / Exposure (Rendering 2.6)  [roadmap — completato]
- [x] **Bridge** — `tonemappingMode` (0 None / 1 Linear / 2 ACES / 3 HejlDawson / 4 Filmic) e `tonemapExposure` (0.05–8, default 1.0); segnale `renderingChanged()`
- [x] **Viewport** — i 4 `SceneEnvironment` applicano `tonemappingMode: Modeler.tonemappingMode` + `exposure: Modeler.tonemapExposure`
- [x] **UI** — sezione Rendering in `ViewportPanel.qml`: ComboBox tonemap + slider Exposure
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Animation Layers (Animazione editoriale 2.4)  [roadmap — completato]
- [x] **Core** — struct `AnimationLayer {name, enabled, weight 0..1, keyframes}` e `QVector<AnimationLayer> layers` in ogni `Animation`; blending non-destructive in `applyPoseToBones`: dopo la base pose ogni layer enabled con weight>0 interpola i propri keyframe assoluti e fonde posizione (lerp) e rotazione (slerp) col proprio weight, applicati in sequenza
- [x] **Bridge** — `animationAddLayer(anim, name)` (ritorna indice), `animationRemoveLayer`, `animationRenameLayer`, `animationSetLayerEnabled`, `animationSetLayerWeight` (clamp 0..1), `animationLayerCount`, `animationLayerList` (nome/enabled/weight/keyframeCount), `animationAddLayerKeyframe` (time,bone,x,y,z,rx,ry,rz), `animationLayerKeyframes`, `addKeyframeForSelectedObjectToLayer(anim, layerIndex)`; helper `animationIndexByName`; segnale `animationLayersChanged()`
- [x] **UI** — barra "LAYERS" in `AnimationTimeline.qml` (visibile quando c'è un'animazione): lista orizzontale layer con nome, conteggio keyframe, CheckBox on/off, Slider weight con %, bottone record pose (selezione), bottone rimuovi; "Add layer" con nome auto "Layer N"
- [x] **Persistenza** — aux JSON top-level `animations` nel `.ks3d` (durata + keyframe base + array layer con nome/enabled/weight + keyframe): le animazioni prima non venivano salvate; lette in `restoreAuxMetadata` (clear + currentAnimation=-1), pulite in `newProject`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Subdivision Cage (Modeling non-destructive 2.1)  [roadmap — completato]
- [x] **Bridge** — `subdivCageEnabled`/`subdivCageLevel` (Q_PROPERTY + segnale `subdivCageChanged`), accessor `setSubdivCageEnabled`/`setSubdivCageLevel`; `applySubdivCage()`: all'attivazione cattura la cage originale come snapshot `SceneMesh::toJson()` per oggetto (`m_cageOrigins`), ripristina la cage e applica Catmull-Clark `MeshOperations::subdivide(level)` via `sceneMeshToMeshData`/`meshDataToSceneMesh` (i modifier restano intatti); alla disattivazione ripristina le cage salvate e svuota la mappa; `m_sceneModel->refresh()` dopo ogni cambio
- [x] **UI** — sezione SUBDIV CAGE in `ViewportPanel.qml`: CheckBox "Smooth on" + slider Level 1–4 (indicatore live del livello)
- [x] **Cleanup** — pulito in `newProject` (flag + cage map)
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Curve continuity (Curve e NURBS 2.2)  [roadmap — completato]
- [x] **Bridge** — `curveSetContinuity(objectId, 0|1|2)` e `curveContinuityOf(objectId)`: C1 muove ogni CV interno al punto medio dei vicini (tangenti condivise, segmenti collineari); C2 applica rilassamento binomiale (Laplaciano `(P[i-1]+2P[i]+P[i+1])/4`) per continuità di curvatura; re-tassellazione `writeCurveMesh` + `sceneChanged`/`curveChanged`; continuità per curva in `m_curveContinuities`, pulita in `newProject`/restore
- [x] **UI** — sezione "Continuity" in `CurvePanel.qml`: bottoni C0/C1/C2 (highlight rosso della modalità attiva) + nota "C1 = shared tangents, C2 = curvature continuity"
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Dynamics / Rigid body (FX e Dynamics 2.5)  [roadmap — completato]
- [x] **Core** — `RigidBodySystem.h/.cpp` (pimpl Bullet opaco, nessun include Bullet nei header bridge): world Bullet per oggetto, shape Box (da bounds) / Sphere (da radius) / ConvexHull (da vertici geometry), stati kinematic/static/dynamic, `addBody/removeBody/removeAll/shapeTypeOf/setBodyKinematic/setBodyMass/sync/step/reset/clearAll`; `sync()` porta pose cinematiche/statiche dagli SceneObject, `step()` scrive la pose dei corpi `dynamic` (mass>0, non kinematic) tornando sugli oggetti
- [x] **Bridge** — `dynAddBody(dobjectId, shapeType, mass, kinematic)`, `dynRemoveBody`, `dynRemoveAll`, `dynBodies()` (objectId/objectName/shapeType), `dynSetKinematic`, `dynSetMass`, `dynPlay/dynPause` (timer ~60 Hz `dynTick`), segnali `dynChanged`; `newProject` pulisce
- [x] **UI** — `DynamicsPanel.qml` (combo shape, massa, kinematic checkbox, Aggiungi/Rimuovi/Clear, lista corpi, Play/Pause); registrato in `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Raytracing viewport (Rendering 2.6)  [roadmap — completato]
- [x] **Core** — `RayTraceRenderer.h/.cpp`: CPU ray tracer con BVH (split mediano su asse maggiore, foglia ≤4 tri, Möller–Trumbore), sole + ambient, ombre dure (shadow ray con qualsiasi-hit), speculare Blinn-Phong (da metalness/roughness), ground grid procedurale con ombra + nebbia, sky gradient
- [x] **Bridge** — `rayTraceEnabled` (Q_PROPERTY + `rayTraceFrameChanged`), `rayTraceSetCamera(eye,target,fov)`, `rayTraceSetSize(w,h)`, `rayTraceFrame()` (revision per cache-bust); `rayTraceTick()` (timer ~6 fps) raccoglie le mesh della scena in world-space (`rtObjectMatrix` + rotazione normale) e aggiorna `m_rayTraceFrame`
- [x] **Image provider** — `RayTraceImageProvider.h` (QQuickImageProvider) registrato `image://raytrace` sull'engine nel singleton `Modeler` (main.cpp); QML: `Image { source: "image://raytrace/frame?rev=N" }`
- [x] **UI** — overlay `rtOverlay` nel viewport User (Image a tutta area + badge "Raytrace preview"), sincronizzazione camera via `updateRayTraceCamera()` (orbita → eye sferica + target + fov) su `cameraChanged`/`rayTraceEnabledChanged`; CheckBox "CPU preview" nella sezione RAYTRACE VIEWPORT di `ViewportPanel.qml`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Cloth / soft body (FX e Dynamics 2.5)  [roadmap — completato]
- [x] **Core** — `ClothSystem.h/.cpp` (Verlet): snapshot del mesh come rest pose, network di spring da archi unici delle facce (`springs`), pinning (0=nessuno / 1=riga superiore per Y max / 2=tutti), integrazione sub-stepped (4 sotto-passi) con gravità (scalata da stiffness) + vento, proiezione dei vincoli di distanza (k da stiffness), collisione col pavimento y=0, `writeBack` aggiorna posizioni E normali del SceneMesh; `setGravity/setStiffness/setDamping/setWind`, `reset` (ripristina rest), `clearAll`
- [x] **Bridge** — `clothAdd(objectId, pinMode)`, `clothRemove`, `clothCount`, `clothList` (objectId/objectName/pinMode/springs), `clothPinModeOf`, `clothSpringCount`, `clothSetGravity/Stiffness/Damping/Wind`, `clothPlay/Pause` (timer ~60 Hz `clothTick`), `clothReset`, `clothRemoveAll`, `clothRunning`; segnale `clothChanged`; pulito in `newProject`
- [x] **UI** — `ClothPanel.qml` (combo oggetto + pin mode + "Make Cloth", lista cloth con pin/springs/rimuovi, slider Stiffness/Damping/Wind per il selezionato, Play/Pause/Reset/Remove All, gravità Y); ribbon "Cloth" cmd `cloth` + overlay `clothOverlay`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Cloth collisions (FX e Dynamics 2.5)  [roadmap — completato]
- [x] **Core** — `ClothSystem`: collisione con oggetti solidi (triangoli world-space `ClothColliderTri` con normale, `setColliders`) risolta in ogni sub-step: broadphase uniform grid sui triangoli (chiave hash `gridKey`), nearest-point su triangolo (Ericson) per ogni particella, push-out lungo la normale se `signedDistance < collisionRadius` (raggio derivato dalla lunghezza media delle spring rest in `addCloth`); **self-collision** opzionale: grid sulle particelle, separazione pairwise delle coppie vicine sotto `collisionRadius`; toggle per-cloth `setCollisionEnabled`/`setSelfCollision`
- [x] **Bridge** — `clothSetCollisionObjects(QVariantList ids)` (registra i solidi, `refreshClothColliders()` ricostruisce i triangoli world-space con `worldTransform`, salta gli oggetti cloth), `clothCollisionObjects()`, `clothSetCollision(id, bool)`/`clothCollision(id)`, `clothSetSelfCollision(id, bool)`/`clothSelfCollision(id)`; il timer cloth richiama `refreshClothColliders()` a ogni tick così i solidi in movimento seguono; `m_clothColliderIds` pulito in `newProject`
- [x] **UI** — `ClothPanel.qml`: sezione "COLLIDERS (solid objects)" (combo oggetti + Add/Clear + lista con rimozione) e toggles "Collide" / "Self collision" per il cloth selezionato; overlay ingrandito a 720px
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Hair / Fur (FX e Dynamics 2.5)  [roadmap — completato]
- [x] **Core** — `HairSystem.h/.cpp`: grow di `strandCount` ciocche di `segments` punti dalle facce del mesh di superficie (campionamento area-weighted: CDF cumulativa su triangoli, punto barycentrico random, normale interpolata dai vertici, jitter di direzione); dinamica Verlet pinnata (radice bloccata, stiff verso la direzione di rest, `1.02×` limit lunghezza, anti-clipping sopra la superficie, gravità+vento); `buildMesh` fonde ribbon cards sottili (`CurvePrimitives::polyline` + `CurveSurfaces::curveRibbon`) per tutte le ciocche in un unico `MeshData`
- [x] **Bridge** — `hairAdd(objectId, strands, segments, length)`, `hairRemove`, `hairCount`, `hairList`, `hairSetLength/Stiffness/Wind`, `hairPlay/Pause` (timer 60 Hz `hairTick`), `hairRemoveAll`, `hairRunning`; crea un SceneObject dedicato `<name>_hair` per il render (`m_hairObjects` surfaceId→hairSceneId, `rebuildHairMesh`); segnale `hairChanged`; pulito in `newProject`
- [x] **UI** — `HairPanel.qml` (combo superficie + slider Strands/Length + "Grow", lista hair, slider Length/Stiffness/Wind per il selezionato, Simulate/Pause/Remove All); ribbon "Hair/Fur" cmd `hair` + overlay `hairOverlay`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Walk-cycle generator (Animazione editoriale 2.4)  [roadmap — completato]
- [x] **Bridge** — `generateWalkCycle(animName, duration, amplitude)`: crea/aggiorna l'animazione (pulisce i keyframe esistenti per idempotenza), mappa i bone Humanoid per nome (`thigh/shin/foot .L/.R`, `upper_arm/forearm/hand`, `root/spine/chest/neck/head`, con varianti _L/_R/left/right e upper_leg/lower_leg/hip/knee/ankle/shoulder/elbow), genera 24 keyframe loopable (t=0..durata): thigh swing ±32°·A con fase 180° tra le gambe, ginocchio 58°·A flesso a metà swing, piede −22°·A·sin, braccia opposte alle gambe, gomito 34°·A, bob torso `0.05·A·cos(2θ)` sul root, pitch/twist spine+chest, contro-rotazione testa; `statusMessage` con esito; emette `sceneChanged`/`animationNameChanged`/`animationTimeChanged`
- [x] **UI** — barra "WALK CYCLE" in `AnimationTimeline.qml`: TextField nome (default nome animazione o "Walk"), slider Durata (0.5–4s, default 1.2), slider Ampiezza (0.2–1.5), bottone Generate (rossi)
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Edge loop / ring tools (Tools di editing raffinati 2.3)  [roadmap — completato]
- [x] **Core** — `MeshOperations::findEdgeLoop(mesh, v1, v2)` e `findEdgeRing(mesh, v1, v2)`: loop = serie di edge i cui membri consecutivi sono edge opposti di quad (crossing; cammina avanti/indietro dal seed con `quadFacesContainingEdge`/`oppositeEdgeInQuad`, guard anti-loop); ring = serie di edge che condividono un vertice marciando attorno ai quad adiacenti (`stepVertex`); ritornano `QVector<Edge>`
- [x] **Bridge** — `edgeLoop(objectId, v0, v1)` e `edgeRing(objectId, v0, v1)`: converte in edge world-space `[[ax,ay,az],[bx,by,bz]]` (`edgeListToWorld` con `worldTransform`), `statusMessage` col conteggio; pick riusa `findClosestEdge`
- [x] **UI** — `EdgeLoopPanel.qml` (target oggetto selezionato, Pick Edge nel viewport User, bottoni Edge Loop (rosso)/Edge Ring/Clear, status); highlight in vpUser come cilindri sottili lungo gli edge (loop rossi, ring gialli) via Repeater + helper `edgeMid/edgeLen/edgeRot` (quaternione Y→dir → euler ZYX); ribbon "Edge Loop" cmd `edgeloop` + overlay `edgeLoopOverlay`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Quad remesh (Tools di editing raffinati 2.3)  [roadmap — completato]
- [x] **Bridge** — `quadRemesh(objectId, level)`: non distruttivo, snapshot del mesh originale in `m_remeshOrigins` (objectId → mesh JSON); ripristina l'originale, applica `MeshOperations::subdivide(md, level)` (0–3) poi `MeshOperations::quadrangulate` (merge di triangoli adiacenti in quad); `quadRemeshClear(objectId)` / `quadRemeshClearAll()` ripristinano l'originale; pulito in `newProject`
- [x] **UI** — `QuadRemeshPanel.qml` (target oggetto selezionato, slider Densità 0–3 con descrizione, Apply (rosso), Restore Original, status); ribbon "Quad Remesh" cmd `quadremesh` + overlay `quadRemeshOverlay`; entry `resources.qrc`
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Quad remesh live preview (Tools di editing raffinati 2.3)  [roadmap — completato]
- [x] **UI** — `QuadRemeshPanel.qml`: checkbox "Live preview" (default on): lo slider Densità ri-applica il remesh automaticamente via `Timer` debounce 120ms (`applyRemesh()`), preview interattivo nel viewport; bottone Apply manuale ferma il timer e ri-applica, Restore ripristina l'originale
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Fabric presets (FX e Dynamics 2.5)  [roadmap — completato]
- [x] **Bridge** — `clothPreset(objectId, name)` e `clothPresetNames()`: applica parametri fisici da materiale in stile XSI (Cotton/Silk/Denim/Leather/Rubber/Wool/Satin) settando stiffness/damping/wind (e gravità per Leather/Satin); `clothChanged`
- [x] **UI** — `ClothPanel.qml`: ComboBox "Fabric" nel pannello parametri del cloth selezionato, specchia i valori del preset sugli slider
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Mocap import (Animazione editoriale 2.4)  [roadmap — completato]
- [x] **Core** — `BvhImporter.h/.cpp`: parser Biovision BVH (gerarchia ROOT/JOINT/End Site con OFFSET e CHANNELS, sezione MOTION con `Frames:`/`Frame Time:` e righe di dati); espone joints (nome, parent, offset, canali, channelOffset), frameCount/frameTime/channelCount e `frame(f)`
- [x] **Bridge** — `importBVH(path, animName)`: mappa i joint BVH sui bone esistenti per nome (funzione `mapBvhBoneName`: Left/Right→`.L/.R`, UpLeg→thigh, Leg→shin, Foot→foot, Arm→upper_arm, ForeArm→forearm, Spine/Neck/Head/Chest/Hips→centro) e, se <2 match, ricostruisce lo skeleton dalla gerarchia BVH (posizioni = somma degli offset); genera keyframe per frame (rotazioni composte nell'ordine dei canali `cq * q` → euler Qt, posizioni per i canali X/Y/Zposition); crea/aggiorna l'animazione, la imposta come corrente; `statusMessage` con conteggio
- [x] **UI** — ribbon "Mocap" cmd `ac_mocap` + `mocapImportDialog` (filtro `*.bvh`)
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)

### Render file esterno (Rendering 2.6)  [roadmap — completato]
- [x] **Core** — `RayTraceRenderer::renderFinal(cam, w, h, samples)`: path tracing CPU, samples² raggi/pixel con jitter (AA), sole come area light (sample `sampleSunDir` spread 4° → ombre morbide), un bounce diffuso indiretto (`randomCosHemisphere`, cos-weighted) con max depth 3, tonemap filmico `1−e⁻x` + gamma 2.2; `traceRay` ricorsivo + `backgroundVec`
- [x] **Bridge** — `rayTraceRenderToFile(path, w, h, samples)`: riusa `buildRTTriangles()` (estratta da `rayTraceTick`, collezione mesh world-space con normali), renderizza e salva PNG, `statusMessage` con esito
- [x] **UI** — ribbon "Render" → "Render File" cmd `ac_renderfile` + `renderFileDialog` (SaveFile `*.png`, 1280×720, 8 samples, aggiorna la camera raytrace prima del render)
- Build Release OK + smoke test pulito (processo vivo, stderr vuoto)
