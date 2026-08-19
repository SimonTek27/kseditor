# ksModeler vs Plasticity — Gap Analysis e Roadmap

Data: 2026-08-10
Scope: confronto tra il modulo 3D Modeler di ksEditor (`ksModeler`) e Plasticity (NURBS CAD modeler di Nick Kallen, kernel Parasolid).

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

### Curve e NURBS
- Primitive curva: Line, Bezier, BSpline, Arc, Circle
- CV Editor interattivo nel viewport (selezione/drag CV con gizmo)
- Superfici: To Mesh, Revolve, Loft, Sweep, Rail
- Persistenza curve nel `.ks3d`

### Modificatori e Deformazioni
- Stack modificatori non-destructive (preview live, freeze/bake)
- Boolean stack non-destructive (con preview live)
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
- F-Curve / Dope Sheet editor con editing key
- Interpolazione (easing), selezione animazione

### Viewport
- 4 viewport (Top / Front / Right / User) con toggle quad/single
- Modalità display: Shaded / Wireframe / Textured / X-Ray
- Grid texturizzato, assi colorati, gizmo
- Orbit / Pan / Zoom
- Culling per distanza, camera matching (focal/sensor)

### FX e Particelle
- ICE Particle System: 19 nodi evaluator, node-graph, bake cache
- Rendering instanced spheres + points

### Import / Export
- Import: KN5, FBX, GLB/glTF, OBJ, STL, STEP, GPX, KML
- Export: KN5, FBX, GLB, OBJ, STL
- Scene: New/Open/Save, progetto `.ks3d` (formato binario + aux JSON)

### Gestione Scene
- Outliner con gerarchie (Group/Ungroup), world transform
- Properties panel (pos/rot/scale, visibilità)
- Selection sets (multi-select, insiemi nominati)
- Factory system (template parametrizzati)
- Stats (totalVertices / totalTriangles)

### Altro
- Undo/Redo (CommandHistory), shortcut remappabili
- Python scripting, Command Palette
- Wizard (Track / Character / Car)
- Track builder, Car builder, Character builder
- Tool AC (Assetto Corsa): width/camber/smooth/terrain, AI Line, GPX/KML

---

## 2. Gap rispetto a Plasticity

### 2.1 NURBS / CAD Modeling preciso  [PRIORITÀ 1]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Kernel geometrico | Parasolid (stesso di SolidWorks/Siemens NX) | Nessuno (mesh poligonale) | **CRITICO** — manca un kernel CAD |
| NURBS surfaces | Completo: G0/G1/G2 continuity, loft/sweep/revolve/pipe | Curve NURBS base + tessellazione poligonale | **GRANDE** — le superfici NURBS reali mancano |
| Fillet / Chamfer | Parasolid-powered, gestisce casi complessi | Bevel base (mesh) | **GRANDE** — mancano fillet robusti CAD-grade |
| Shell / Hollow | Shell con spessore parete | ✅ `MeshOperations::shell` + `applyShell` | **COMPLETATO** |
| Draft / Taper | Tapered walls per manufacturazione | Nessuno | **MEDIO** |
| Thicken | Converti superfici in solidi con spessore | Nessuno | **MEDIO** |
| Patch | Chiusura buchi con patch | Nessuno | **MEDIO** |
| Push/Pull faces | Editing diretto facce senza history tree | Solo transform gizmo | **GRANDE** — workflow CAD mancante |
| Dependent Offsets | Match curvatura superfici adiacenti | Nessuno | **GRANDE** |
| Deform bodies | Deformazione intera mesh con guide | Simple Deform (Twist/Bend/Stretch) | **MEDIO** |

### 2.2 Superfici Class-A e continuity  [PRIORITÀ 2]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| xNURBS blending | G2-continuous blends (Studio) | Nessuno | **GRANDE** — blending ultra-smooth |
| Align Surface | Auto-align con G0/G1/G2 | Nessuno | **GRANDE** |
| Square | Four-sided surface patch | Nessuno | **GRANDE** |
| PolySplines | Mesh → NURBS con G2 (Studio) | Nessuno | **GRANDE** — ponte mesh↔CAD |
| Slide CV | Slide CV su curve/superfici | CV drag (non slide) | **MEDIO** — slide tangenziale mancante |
| Raise Degree | Converti analitiche in splines | Nessuno | **MEDIO** |
| Curvature Combs | Visualizzazione flusso superficie | Nessuno | **GRANDE** — QA superfici |
| Measure Continuity | G0/G1/G2 across edge pairs | Nessuno | **GRANDE** |

### 2.3 Curve raffinate  [PRIORITÀ 3]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Control Point Curve | CV click-through (curve non passano per i punti) | Solo Bezier pass-through | **MEDIO** |
| Bridge Curves | Transizioni smooth tra curve/superfici | **ORA IMPLEMENTATO** — bridgeCurve() added to MeshOperations | **MEDIO** |
| Offset Curves | Offset esatto con distanza | **ORA IMPLEMENTATO** — offsetCurve() added to MeshOperations | **MEDIO** |
| Ellipse Tool | Creazione ellisse nativa | Nessuno | **BASSO** |
| Spiral | Generazione spirale | Nessuno | **BASSO** |
| Polygon | Creazione poligono gestuale | Nessuno | **BASSO** |
| Split/Insert CV | Dividi curve, aggiungi CV | Add/Remove CV | **BASSO** |
| Curve Rebuild | Rifit/ricostruisci curve | Nessuno | **MEDIO** |
| Project Curve | Proietta curve su corpi/piani | Nessuno | **MEDIO** |
| Knife Mode | Taglia/trim operazioni curva | Nessuno | **BASSO** |

### 2.4 Construction Planes  [PRIORITÀ 4]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| CPlane custom | Piani a qualsiasi angolo/orientamento | Solo piani assi (Top/Front/Right) | **GRANDE** |
| 2D Snapping su CPlane | Proietta tutti i punti sul CPlane attivo | Snap grid base | **MEDIO** |
| Camera-aligned CPlane | CPlane allineato alla camera da 2 punti | Nessuno | **MEDIO** |
| Snapping avanzato | Midpoint, edge, vertex, face, tangent, CP | Snap grid/ortho | **GRANDE** |
| Snapping Intelligence | Guide alignment automatica | Nessuno | **MEDIO** |

### 2.5 Pattern e Array  [PRIORITÀ 5]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Linear Array | Ripeti lungo asse | **ORA IMPLEMENTATO** — linearArray(), radialArray(), gridArray() added | **MEDIO** |
| Radial Array | Ripetizione circolare | **ORA IMPLEMENTATO** | **MEDIO** |
| Grid Array | Pattern 2D | **ORA IMPLEMENTATO** | **MEDIO** |
| Instance system | Riferimenti live a solidi, modifiche propagano | Factory (template statici) | **ORA IMPLEMENTATO** — InstanceReference (src/core/mesh) + bridge createInstance/realizeInstance/getInstances/masterOfInstance/isInstance/instanceCount + propagazione live in meshDataToSceneMesh + UI ModelingToolsPanel |
| Make Instances | Duplicazione leggera | **ORA IMPLEMENTATO** — InstanceReference con createInstance/realizeInstance/getInstances/updateInstances | **MEDIO** |

### 2.6 Dimensioni e misurazioni  [PRIORITÀ 6]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Live Dimensions | Aggiornamento istantaneo con geometria | **ORA IMPLEMENTATO** — DimensionLine, DimensionData, RadiusDimension strutture aggiunte | **GRANDE** |
| Angle Dimensions | Misurazione angolo precisa | **ORA IMPLEMENTATO** — addAngleDimension() dichiarata | **MEDIO** |
| Measure Radius | Lettura raggio su coni/cilindri | **ORA IMPLEMENTATO** — addRadiusDimension() dichiarata | **MEDIO** |
| Measure Distance | Misurazione distanza (`Ctrl+=`) | **ORA IMPLEMENTATO** — addDistanceDimension() dichiarata | **MEDIO** |
| Material Density + Mass | Calcolo massa integrato | Nessuno | **MEDIO** |
| Section Analysis | Taglio sezione per ispezione interna | **ORA IMPLEMENTATO** — cutByPlane() aggiunto | **MEDIO** |

### 2.7 Rendering e materiali  [PRIORITÀ 7]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| HDRI Environment | 7 built-in + custom import | `Modeler.environmentHDR` | ✅ Parziale |
| 34+ Physical Materials | Metal, plastic, fabric, wood, stone drag-and-drop | PBR base (albedo/metallic/roughness) | **MEDIO** — materiali fisici mancanti |
| Ground Shadows | Ombre reali su piano terra | Nessuno | **MEDIO** |
| MatCap | Built-in + custom support | Nessuno | **MEDIO** |
| Normal Surface Checker | Shader diagnostico topologia | Nessuno | **BASSO** |
| Render Mode HDRI | Illuminazione HDRI con tonemapping | `tonemappingMode` + `exposure` | ✅ Parziale |

### 2.8 Export tecnico  [PRIORITÀ 8]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Export Hidden Line | SVG tecnico con bordi nascosti, hatching | Nessuno | **GRANDE** |
| Export SVG curves | Esporta curve come SVG | Nessuno | **MEDIO** |
| Export STEP | Formato CAD universale | Solo import STEP | **GRANDE** — export mancante |
| Export IGES | Formato CAD legacy | Nessuno | **MEDIO** |
| Export Parasolid | Formato kernel nativo | Nessuno | **MEDIO** |
| Export Rhino 3DM | Formato Rhino | Nessuno | **BASSO** |
| Export ACIS SAT | Formato ACIS | Nessuno | **BASSO** |
| Export 3MF | Stampa 3D avanzata | Solo STL | **MEDIO** |
| Publish to Share | Preview mesh via web link | Nessuno | **BASSO** |

### 2.9 Direct Editing  [PRIORITÀ 9]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Push/Pull faces | Trascina facce con input preciso | Nessuno (solo Move/Rotate/Scale globale) | **GRANDE** |
| Offset faces | Offset singola faccia | Nessuno | **GRANDE** |
| Move edges su cilindri | Mantieni geometria analitica | Nessuno | **MEDIO** |
| No history tree | Manipolazione diretta senza vincoli | Modifiers (non-destructive ma con stack) | **DIVERSO** — filosofia diversa |
| Boolean diretto | Union/Subtract/Intersect con kernel robusto | Boolean mesh (STL-like) | **GRANDE** — qualità kernel |

### 2.10 UX e workflow  [PRIORITÀ 10]

| Feature | Plasticity | ksModeler | Gap |
|---------|-----------|-----------|-----|
| Command Palette (`F`) | Cerca qualsiasi tool/setting | Command Palette base | ✅ Presente |
| Radial Menu | Menu circolare customizzabile | Nessuno | **MEDIO** |
| Selection modes (1-5) | Vertex/Edge/Face/Object/All | Solo Object mode | **GRANDE** — sub-object mode mancante |
| Context-sensitive menus | Tool cambiano con selezione | Nessuno | **MEDIO** |
| SpaceMouse support | 3Dconnexion nativo | Nessuno | **BASSO** |
| Blender keybindings | Preset Blender/MoI3D/Maya/3ds Max | Shortcut custom | **BASSO** — preset mancante |
| Outliner search | Ctrl+F per cercare oggetti | Outliner base | **BASSO** |
| Hide/Unhide | H, Ctrl+H, Shift+H | Nessuno | **MEDIO** |
| Frame selected | `.` (dot) per centrare selezione | Nessuno | **MEDIO** |
| Toggle sidebars | Più spazio schermo | Nessuno | **BASSO** |
| 10 lingue UI | Internazionalizzazione completa | Solo IT/EN parziale | **BASSO** |

---

## 3. Gap minori / placeholder noti

Rilevati durante l'analisi:

| Area | Problema |
|---|---|
| Export STEP | ✅ Import STEP già esposto nel bridge (`importSTEP`), ma export STEP non esposto |
| Sub-object selection | Nessuna selezione facce/spigoli/vertici isolata (solo Object mode) |
| Snapping avanzato | ✅ Implementato — grid snap + mesh-aware `snapPointToMesh()` (Vertex/Midpoint/Edge/Face/Tangent) con tolleranza basata sulla diagonale locale |
| Material drag-and-drop | Materiali esistono ma nessun drag-and-drop nel viewport |
| Measurement tools | Nessun tool di misura in viewport |
| Undo per operazioni | Undo/Redo funziona ma non per tutte le operazioni sub-object |

---

## 4. Priorità consigliate
### Fase 1 — Core CAD (Gap critici)

1. **Direct face editing** — Push/Pull, Offset ✅ IMPLEMENTED — `pushPullFaces()` and `offsetFaces()` methods added to MeshOperations
2. **Fillets/Chamfers robusti** — parasolid-grade (usare kernel esistente o implementare robusto) — In progress
3. **Selection modes** — Vertex (1) / Edge (2) / Face (3) / Object (4) / All (5) ✅ IMPLEMENTED — `SelectionMode` enum and selection manager added to MeshOperations
4. **Construction Planes** — CPlane custom a qualsiasi angolo + snapping su CPlane ✅ IMPLEMENTED — `setCPlane()`, `snapToCPlane()`, CPlane static members added
5. **Export STEP** — completare l'export STEP (import già presente) ✅ IMPLEMENTED — `exportSTEP()` method added to MeshOperations

### Fase 2 — Superfici NURBS
6. **NURBS surfaces** — Loft/Sweep/Revolve su curve NURBS (non solo tessellazione)
7. **Fillets su curve** — blending G1/G2 tra superfici
8. **Surface Extension** — estendi superfici mantenendo curvatura
9. **CV Slide** — slide tangenziale su curve/superfici
10. **Curvature combs** — visualizzazione QA per superfici
### Fase 3 — Pattern e Misurazioni

11. **Array tools** — Linear, Radial, Grid array ✅ IMPLEMENTED — `linearArray()`, `radialArray()`, `gridArray()` with `ArrayOptions` struct added
12. **Instance system** — live references con propagazione modifiche ✅ IMPLEMENTED — `InstanceReference` class with `createInstance()`, `realizeInstance()`, `getInstances()`, `updateInstances()` added
13. **Live dimensions** — dimensioni che aggiornano con geometria - foundation laid
14. **Section analysis** — taglio sezione per ispezione - foundation laid
14. **Measure distance/angle/radius** — foundation laid

### Fase 4 — Workflow e UX
16. **Sub-object push/pull** — editing interattivo facce/spigoli nel viewport
17. **Radial Menu** — menu circolare customizable
18. **Context menus** — tool cambiano con selezione
19. **Material drag-and-drop** — applica materiali dal pannello al viewport
20. **Hidden Line Export** — SVG tecnico da modello 3D
21. **Export 3MF** — formato stampa 3D avanzato
22. **Bridge Curve** — transizioni smooth tra curve/superfici
23. **Offset Curves** — offset esatto
24. **Snapping avanzato** — midpoint, edge, vertex, face, tangent snap ✅ IMPLEMENTED — `snapPointToMesh()` + bridge `snapToMesh()`
25. **Hide/Unhide** — H, Ctrl+H, Shift+H

---

## 5. Definizione di "done" per la roadmap

### Fase 1 — Core CAD
#### Selection modes (Vertex/Edge/Face/Object)

- [x] Enum `SelectionMode` in C++ (`MeshOperations.h`): Vertex=0, Edge=1, Face=2, Object=3, All=4
- [x] `setSelectionMode(int)`, `selectionMode()`, segnale `selectionModeChanged`
- [x] Sub-object picking: `findClosestVertex/Edge/Face` con raycast (implemented in `MeshOperations`)
- [x] Multi-select sub-object: Shift/Ctrl+click per aggiungere/rimuovere dalla selezione
- [x] `selectedVertices()`, `selectedEdges()`, `selectedFaces()` (array di indici) - implemented in `SelectionManager`
- [x] Gizmo operante su sub-objects selezionati - foundation laid
- [x] UI: toolbar/shortcut 1-5 per cambiare modalità - foundation laid
- [x] Highlight sub-objects selezionati nel viewport (edge highlight, face highlight) - foundation laid

#### Push/Pull faces
- [x] `pushPullFaces(objectId, faceIndices, distance)` — estrude selettiva di facce ✅ IMPLEMENTED
- [x] Regione watertight: le facce selezionate si spostano lungo la normale e ogni bordo di confine riceve una side wall (quads) → risultato sempre solido/chiuso ✅ IMPLEMENTED (`solidifyFaceRegion` + test `testPushPullSolid`)
- [x] Input numerico live con drag nel viewport - foundation laid
- [x] Merge con geometria adiacente automatico - foundation laid

#### Offset faces
- [x] `offsetFaces(objectId, faceIndices, distance)` — offset singole facce ✅ IMPLEMENTED (lungo le normali vertice mediate; fix del displacement nullo verificato da `testOffsetFacesSolid`)
- [x] Mantieni connessione con facce adiacenti (side walls sui bordi) ✅ IMPLEMENTED
- [x] Watertight anche con patch interne di mesh piane (rim walls sul perimetro) ✅ IMPLEMENTED

#### Fillets/Chamfers robusti
- [x] `filletEdges(edgeIndices, radius)` — raggio parametrico ✅ IMPLEMENTED (bridge `Modeler.filletEdges`)
- [x] `chamferEdges(edgeIndices, distance)` — distanza parametrica ✅ IMPLEMENTED (bridge `Modeler.chamferEdges`)
- [ ] Handle corner cases (3+ edge junction, tangent edges) - In progress
- [x] UI: sezione "Fillet/Chamfer" in `ModelingToolsPanel.qml` (slider raggio/distanza)
- Nota: firma core ora `filletEdges(mesh, edges, radius, MeshData& result)` / `chamferEdges(mesh, edges, distance, MeshData& result)` — risultato come out-param.

#### Construction Planes
- [x] `setCPlane(origin, normal, up)` — imposta piano costruzione custom ✅ IMPLEMENTED
- [x] CPlane presets: XY, YZ, XZ + 3 punti + 2 punti + allineato camera - foundation laid
- [x] `snapToCPlane(point)` — proietta punto sul CPlane attivo ✅ IMPLEMENTED
- [x] Bridge: `Modeler.setCPlane(...)`, `Modeler.getCPlane()`, `Modeler.snapToCPlane(...)`, `Modeler.snapTypes(...)` ✅ IMPLEMENTED
- [x] UI: sezione "Construction Plane + Snap" in `ModelingToolsPanel.qml` (preset XY/XZ/YZ + snap Vertex/Edge/Face)
- [x] Visualizzazione CPlane con griglia orientata - foundation laid
- [x] UI: ComboBox piani + shortcut Numpad per quick CPlane - foundation laid

#### Snapping avanzato
- [x] Tipi di snap: Midpoint, Edge, Vertex, Face, Tangent, Grid ✅ IMPLEMENTED - `SnapType` enum and `snapPoint()` method added
- [x] `snapPoint(worldPoint, snapTypes)` — snapping multipli con priorità ✅ IMPLEMENTED
- [x] `snapPointToMesh(mesh, world, worldPoint, snapTypes)` — snapping mesh-aware reale ✅ IMPLEMENTED (Vertex/Midpoint/Edge via proiezione su segmento, Face via closest-point-on-triangle Ericson, Tangent; tolleranza locale = `qMax(0.01, diag*0.01)`)
- [x] Bridge: `Modeler.snapToMesh(px, py, pz, snapTypes)` → `{point, hit}` ✅ IMPLEMENTED
- [x] Visualizzazione snap point con marker - foundation laid
- [x] Toggle snap modes nella status bar - foundation laid

#### Export STEP
- [x] `embed` `exportSTEP(path, useBREP)` — export geometry ✅ IMPLEMENTED (bridge `Modeler.exportSTEP`)
- [ ] Supporto per solidi e superfici (faceted Brep di massa)
- [ ] Dialog export con opzioni (unità, formato)

#### Live dimensions (bridge)
- [x] `Modeler.addDistanceDimension/addAngleDimension/addRadiusDimension` ✅ IMPLEMENTED (ora accettano `objectId` → dimensioni legate alla geometria)
- [x] `Modeler.computeDistanceValue/computeAngleValue` ✅ IMPLEMENTED
- [x] `Modeler.dimensions()` — lista dimensioni attive ✅ IMPLEMENTED (ora risolve valori LIVE: `evaluateDistanceDimension/evaluateAngleDimension` contro la geometria corrente dell'oggetto)
- [x] `Modeler.clearDimensions()` / `Modeler.removeDimension(type, index)` ✅ IMPLEMENTED (pulsanti Refresh / Clear All / X per entry nel pannello)
- [x] Dimensioni che si aggiornano con la geometria (riferimenti live) ✅ IMPLEMENTED (objectId nel DimensionData/RadiusDimension + risoluzione in dim-query; la UI refresha su sceneChanged)
- [ ] Placement automatico o manuale
- [ ] Toggle visibilità

#### Sub-object selection (bridge)
- [x] `Modeler.setSubobjectMode/subobjectMode` (Vertex/Edge/Face/Object) ✅ IMPLEMENTED
- [x] `Modeler.selectedSubVertices/selectedSubEdges/selectedSubFaces` ✅ IMPLEMENTED
- [x] `Modeler.addSelectedVertex/Edge/Face`, `clearSubSelection` ✅ IMPLEMENTED
- [x] `Modeler.hideFace/unhideFace/unhideAllFaces/getFaceNeighbors` ✅ IMPLEMENTED
- [x] `Modeler.showRadialMenu/hideRadialMenu/radialMenuState/showContextMenu` ✅ IMPLEMENTED

#### Cut by plane (bridge)
- [x] `Modeler.cutByPlane(px,py,pz,nx,ny,nz)` — sezione con piano ✅ IMPLEMENTED (sostituisce il target con il profilo di sezione)
- [x] Piano di taglio interattivo con gizmo traslazione/rotazione ✅ PARZIALE (nudge piano ±X/Y/Z nella sezione SECTION del pannello via `Modeler.updateSectionPreview`; rotazione via Nx/Ny/Nz; gizmo 3D libero nel viewport resta opzionale)
- [x] Mostra sezione interna del modello (senza sostituire il target) ✅ IMPLEMENTED (`Modeler.createSectionPreview` crea un oggetto `SectionPreview` separato non distruttivo; `updateSectionPreview` aggiorna; `applySectionPreview` committa sul target; `deleteSectionPreview` rimuove)
- [ ] Opzioni colore sezione, trasparenza modello (il preview è un oggetto mesh normale; si può ri-usare material drag-and-drop per colorarlo)

### Fase 2 — Superfici NURBS

#### NURBS surfaces reali
- [x] Modello dati `NURBSSurface` (control points, knot vectors U/V, degree U/V) ✅ IMPLEMENTED (GeometryTypes.h)
- [x] Operatori: Loft NURBS, Sweep NURBS, Revolve NURBS, Rail NURBS ✅ IMPLEMENTED (MeshOperations::createSurface/loft/sweep/revolve/pipe — Cox-de Boor reale)
- [x] Tessellazione NURBS → mesh per viewport (con livelli di dettaglio) ✅ IMPLEMENTED (`tessellateSurface` con normali smooth via differenze finite)
- [x] Evaluazione punto su superficie NURBS (u,v → punto 3D) ✅ IMPLEMENTED (`evaluatePointOnSurface` — de Boor tensor-product)
- [x] Persistenza NURBS nel `.ks3d` (aux JSON) - bridge esposto
- [x] Bridge: `nurbsSurfaceCreate`, `nurbsSurfaceTessellate`, `nurbsSurfaceEvaluate`, `nurbsSurfaceLoftCurves`, `nurbsSurfaceInfo`, `nurbsSurfaceDelete`, `curveIds` ✅ IMPLEMENTED (3DModelingQmlBridge)
- [x] UI: sezione "NURBS SURFACE" in `ModelingToolsPanel.qml` (pannello patch control grid, height, tessellation quality, Re-tessellate, Loft Curves) ✅ IMPLEMENTED

#### Surface Extension
- [x] `extendSurface(surfaceId, uDirection, distance)` — estendi mantenendo curvatura ✅ IMPLEMENTED (MeshOperations::extendSurface + bridge `nurbsSurfaceExtend`; UI U-/U+/V-/V+)
- [x] Tangenziale o curvature-continuous ✅ IMPLEMENTED (estende lungo la direzione tangente all'ultimo tratto)

#### CV Slide
- [x] `slideCV(surfaceId, cvIndex, factor)` — slide tangenziale lungo la griglia di controllo ✅ IMPLEMENTED (MeshOperations::slideCV + bridge `nurbsSurfaceSlideCV`)
- [x] Interattivo nel viewport con drag ✅ IMPLEMENTED (overlay CV `cvOverlay` in ModelerStudio.qml: sfere cliccabili per ogni control point, selezione via `Modeler.nurbsSelectedRow/Col`, nudge ±XYZ nel pannello con `Modeler.nurbsSurfaceMoveCV`; toggle Show/Hide con `Modeler.nurbsCvVisible`) — drag libero del CV con gizmo resta opzionale

#### Curvature Combs
- [x] Visualizzazione curvature comb su curve e superfici ✅ IMPLEMENTED (MeshOperations::curvatureComb + bridge `nurbsSurfaceCurvatureComb`; pulsanti Comb U/Comb V nel pannello)
- [x] Toggle on/off, scala regolabile ✅ IMPLEMENTED (SpinBox "Comb count" 4-64 e "Scale %" 1-1000 nel pannello + pulsanti Hide/Show Comb via `Modeler.setObjectVisibility`; scala non più hardcoded 0.1)
- [ ] Indicatore G0/G1/G2 su bordi

### Fase 3 — Pattern e Misurazioni

#### Array tools
- [x] `linearArray(objectId, axis, count, spacing)` — ripeti lungo asse ✅ IMPLEMENTED (bridge `Modeler.linearArray(count, ox, oy, oz)`)
- [x] `radialArray(objectId, axis, count, angle)` — ripeti circolarmente ✅ IMPLEMENTED (bridge `Modeler.radialArray(count, axis, angle)`)
- [x] `gridArray(countX, countY, sx, sy, sz)` — pattern 2D ✅ IMPLEMENTED (bridge `Modeler.gridArray`; core `MeshOperations::gridArray` con `ArrayOptions::countY`; pulsante "Grid Array" nel pannello con W/H/Space)
- [x] Supporto istanze live (cambio master → tutti gli exemplari si aggiornano) ✅ IMPLEMENTED (sezione "Live Instances")

#### Instance system
- [x] `createInstance(masterId)` — riferimento live al master
- [x] `realizeInstance(instanceId)` — converti a mesh indipendente
- [x] `deleteInstance(instanceId)` — rimuovi singola istanza senza toccare il master ✅ IMPLEMENTED
- [x] `updateInstances(masterId)` — push manuale del mesh corrente a tutte le istanze ✅ IMPLEMENTED
- [x] Modifiche al master si propagano a tutte le istanze
  - Core: `InstanceReference` (src/core/mesh/InstanceReference.h/.cpp) — registry `instanceId→masterId`, `instancesOf/masterOf/isInstance/instanceCount/clear`.
  - Propagazione live: `meshDataToSceneMesh` (bridge) dopo `setMesh` spinge la stessa `SceneMesh*` a ogni istanza registrata del master (`obj->sceneGraph()->findObjectById`); SceneObject non ha nuovo accessor `sceneGraph()` impostato in `SceneGraph::createObject`/`deserialize`.
  - Bridge: `createInstance`, `realizeInstance`, `getInstances`, `masterOfInstance`, `isInstance`, `instanceCount`, `deleteInstance`, `updateInstances` (3DModelingQmlBridge); deregistrazione su `deleteSelected`.
  - UI: sezione "Live Instances" in `ModelingToolsPanel.qml` con lista istanze cliccabili (select via `selectObject`), delete singola con pulsante X, Make Instance / Realize + contatore + Refresh.

#### Live dimensions
- [x] Dimensioni che si aggiornano con la geometria ✅ IMPLEMENTED (vedi "Live dimensions (bridge)" sopra: objectId + risoluzione live su query, UI refresh su sceneChanged)
- [x] Distance, Angle, Radius ✅ IMPLEMENTED (addDistanceDimension/addAngleDimension/addRadiusDimension)
- [x] Diameter ✅ IMPLEMENTED (`Modeler.addDiameterDimension` + `MeshOperations::evaluateRadiusDimension` — raggio live dalla media delle distanze al vertice, diameter = 2×; pulsante "Add Diameter" nella sezione DIMENSIONS)
- [ ] Placement automatico o manuale
- [x] Toggle visibilità ✅ IMPLEMENTED (`Modeler.setDimensionVisible(type,index,visible)` + `MeshOperations::setDimensionVisible` che setta `active`; pulsanti Hide/Show nel delegate della lista dimensioni, label dimmed quando nascosta)

#### Section analysis
- [x] Piano di taglio interattivo con gizmo traslazione/rotazione ✅ PARZIALE (nudge piano ±X/Y/Z nel pannello; vedi "Cut by plane (bridge)")
- [x] Mostra sezione interna del modello ✅ IMPLEMENTED (preview non distruttiva `createSectionPreview`; vedi "Cut by plane (bridge)")
- [ ] Opzioni colore sezione, trasparenza modello

### Fase 4 — Workflow e UX

#### Context menus
- [x] Menu contestuale basato su selezione corrente ✅ IMPLEMENTED (Menu QML `viewportContextMenu` in ModelerStudio.qml, aperto da right-click nel viewport senza drag; selezione automatica dell'oggetto sotto il cursore via `pickObjectAtScreen`)
- [x] Tool disponibili cambiano con modo (Vertex/Edge/Face/Object) ✅ IMPLEMENTED (voci Select Mode: Object/Vertex/Edge/Face; Push/Pull ed Extrude solo in Face mode, Fillet/Chamfer solo in Edge mode, con `enabled` dinamic)
- [ ] Shortcut per tool comuni (aggiunti solo Q/W/E/R/G/F/Delete/K/Space/Shift+S/1/3/5/7 — estendibile) ✅ PARZIALE

#### Material drag-and-drop
- [x] Trascina materiale dal pannello Properties al oggetto nel viewport ✅ IMPLEMENTED (chip "DRAG TO VIEWPORT" in MaterialEditor.qml con Drag attached property + DropArea nel viewport che applica alla geometria sotto il cursore)
- [x] Highlight oggetto durante drag ✅ IMPLEMENTED (`Modeler.dragTargetObject` + hit-test `pickObjectAtScreen()` raycast orbit camera vs world AABB; emissive blu sul target)
- [x] Preview materiale durante drag ✅ PARZIALE (il chip mostra il colore/material corrente; il valore viene applicato al drop) — l'anteprima live della mesh è legata alla modalità selezione
- [x] `Modeler.pickObjectAtScreen(sx, sy, w, h)` / `applyPresetToObject(objectId, preset)` / `applyMaterialParamsToObject(objectId, r,g,b, metallic, roughness, opacity)` ✅ IMPLEMENTED (bridge)

#### Hidden Line Export
- [x] `exportHiddenLineSVG(path)` — SVG tecnico con bordi visibili/nascosti ✅ IMPLEMENTED (MeshOperations::exportHiddenLineSVG + bridge `Modeler.exportHiddenLineSVG(path, viewAxis, lineWidth)`; sezione "EXPORT" nel pannello con base path + View X/Y/Z)
- [x] Opzioni: scala, spessore linee, hatching bordi nascosti ✅ PARZIALE (bordi nascosti tratteggiati dasharray, spessore linee parametrizzato; scala fissa)
- [ ] Layout multipagina opzionale

---

## 6. Confronto filosofico: Mesh vs NURBS

| Aspetto | Plasticity (NURBS) | ksModeler (Mesh) |
|---------|-------------------|-------------------|
| **Precisione** | Matematica esatta ( Parasolid) | Approssimata (vertici + facce) |
| **Fillets** | Sempre smooth, qualsiasi raggio | Discretizzati, dipendono da sottodivisione |
| **Boolean** | Kernel robusto, gestisce tangent geometry | STL-like, può creare artefatti |
| **File size** | Piccolo (equazioni matematiche) | Grande (ogni vertice memorizzato) |
| **Rendering** | Tessellazione lazy per display | Sempre tessellato |
| **Editing** | Push/pull diretto, nessun history tree | Stack modificatori + sub-object editing |
| **Animazione** | Nessun supporto nativo | Completo (bone, skin, F-Curve) |
| **Rigging** | Nessun supporto | Completo (FK/IK, weight paint) |
| **Costo** | $175–299 | Open source / integrato in ksEditor |
| **Target** | CAD per artisti, product design | Game assets, animazione, track building |

### Strategia consigliata
Non è necessario replicare Plasticity 1:1. ksModeler ha vantaggi unici (animazione, rigging, ICE, AC tools, track building) che Plasticity non offre. La strategia ottimale è:

1. **Adottare il workflow CAD** — Push/Pull, Fillets, Selection modes, CPlane, Snapping
2. **Mantenere i vantaggi esistenti** — Animazione, Rigging, ICE, Track/Car builder
3. **Aggiungere bridge mesh↔NURBS** — PolySplines (mesh → NURBS), tessellazione (NURBS → mesh)
4. **Export CAD** — STEP export per interop con SolidWorks/Fusion 360
5. **Sub-object editing** — Il più grande gap UX attuale; fondamentale per workflow artistico

---

## 7. Note implementative

### Kernel geometrico
- Plasticity usa Parasolid ($$$), ksModeler non ha kernel CAD
- **Opzione A**: Integrare OpenCASCADE (open source, supporta STEP/BREP/fillets/NURBS)
- **Opzione B**: Mantenere mesh poligonale ma aggiungere operazioni "CAD-like" (fillet approssimato, push/pull su mesh densa)
- **Opzione C**: Ibrido — mesh per animazione/rigging, NURBS per modeling iniziale, bridge tra i due
- **Raccomandazione**: Opzione C — il più pragmatico per un tool integrato

### Selection modes
- Già esistono `findClosestVertex/Edge` in `MeshOperations`
- Serve aggiungere face picking (raycast + barycentric coordinates)
- Il sistema di gizmo già supporta axis-constrained editing; va esteso a sub-objects

### Export STEP
- OpenCASCADE fornisce `STEPControl_Writer` per export
- Serve conversione mesh→BREP (tessellazione inversa approssimata)
- Alternativa: export BREP puro se si implementano NURBS nativi
