# ksLiveryEditor vs Mudbox — Gap Analysis e Roadmap

Data: 2026-08-14
Scope: confronto tra il modulo Paint/Livery di ksEditor (`ksPaintEditor`) e Autodesk Mudbox (digital sculpting & 3D painting tool).

---

## 1. Stato attuale di ksPaintEditor

Funzionalità già implementate (verificate nel codebase):

### Painting Tools (22 tools)
- Move, Zoom, Pan
- Rectangle Select, Ellipse Select, Free Select, Fuzzy Select (magic wand)
- Color Picker
- Brush (soft/hard round, size/hardness/opacity/flow)
- Pencil (1px hard)
- Eraser (alpha reduction)
- Airbrush (flow-based falloff)
- Smudge (source-blend painting)
- Blur (box blur with strength)
- Sharpen (3x3 kernel)
- Dodge / Burn (tonal adjustment)
- Clone (Ctrl+click source, paint from offset)
- Healing (blend-based cloning)
- Bucket Fill (flood fill, tolerance, selection-aware)
- Gradient (linear/radial, primary/secondary colors)
- Text (dialog for text/font/color, renders to layer)
- Square Brush, Stamp

### Brush Parameters
- Size (1–200 px)
- Hardness (0–100%)
- Opacity (1–100%)
- Flow (1–100%)
- Strength (1–100%, filter brushes)
- Tolerance (0–100%, fuzzy select/bucket fill)

### Layer System
- Add layer (transparent ARGB32)
- Add layer from image (auto-scaled)
- Remove / Duplicate / Rename layer
- Move layer up/down
- Merge down (composites with blend mode + opacity)
- Per-layer opacity (0.0–1.0)
- Per-layer visibility toggle
- Per-layer blend mode
- Per-layer offset (x,y pixel offset)

### Blend Modes (12)
- Normal, Multiply, Screen, Overlay, Darken, Lighten
- Color Dodge, Color Burn, Hard Light, Soft Light, Difference, Exclusion

### Selection System
- Selection mask (ARGB32 QImage)
- Select All / Select None / Select Invert
- Marching ants animated boundary
- Selection-aware painting (all tools respect mask)
- Copy/Cut/Paste with selection masking

### Image Filters
- Invert, Grayscale/Desaturate, Sepia
- Brightness, Contrast
- Gaussian Blur, Sharpen

### Undo/Redo
- Snapshot-based (entire layer stack, 64 steps)
- Action-based (module-level, 50 steps)

### Skin / Livery Management
- Skin discovery (scan `<carPath>/skins/`)
- Create/Delete/Duplicate skin
- Skin INI parsing/writing (NAME, PRIORITY, BASE_COLOR, LICENSE_PLATE, etc.)
- Skin layers JSON (`skin_layers.json`) full serialization
- Load/save paint texture (`sides_1.png` / `paint.png`)

### Export / Import
- Export PNG (flattened composite)
- Export DDS (DXT5 block-compressed, custom compressor)
- Export skin as JSON
- Import skin from JSON
- Import decal (PNG/JPG/DDS/TGA/BMP/TIFF)

### Vector Design Tools
- Rectangle, Ellipse, Line, Polygon, Pen (Path)
- Select tool (click + box selection, 8-handle resize)
- Fill/Stroke colors per shape, opacity, stroke width
- Serialization to/from JSON
- Render to QImage at arbitrary resolution

### Color System
- Primary/Secondary foreground/background swatch
- Swap and reset
- Default palette (20 racing colors)
- Palette load/save from CSV

### 3D Preview / Viewport
- OBJ, KN5, GLTF/GLB model loading
- Live paint texture projection (vertex UV → vertex colors)
- Camera reset / focus
- Lit and Wireframe view modes

### License Plate Generation
- Country support, plate validation
- Auto-add as decal layer

### Template System
- 6 built-in templates (Racing Stripes, Carbon Edition, National Flag, Clean Canvas, Gradient Flow, Gulf-Inspired)
- Template creation with base texture + skin.ini

### QML Frontend
- PhotoGIMP-style layout
- 24 QML tools
- Layer panel with visibility, add/remove/reorder, blend mode, opacity
- Brush tool options bar
- Canvas zoom via mouse wheel
- Integration with TexturePainter QML bridge

---

## 2. Gap rispetto a Mudbox

### 2.1 3D Direct Painting  [PRIORITÀ 1]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Paint directly on 3D model surface | 3D paint: stroke appears on model in real-time, UV-mapped to 2D bitmap behind the scenes | 2D canvas painting only; 3D preview is read-only vertex color projection | **CRITICO** — manca il core workflow di Mudbox |
| Paint layers on 3D model | Layer-based paint with per-layer channel (diffuse, gloss, specular, reflection, bump) | Layer system esiste ma solo per 2D canvas; nessun paint su mesh 3D | **CRITICO** |
| UV View for 2D texture editing | Dedicated UV View showing 2D texture space + UV layout | Nessuna UV view dedicata | **GRANDE** |
| PTEX painting (UV-free) | Paint per-face without UVs, automatic per-face mapping | Nessuno | **GRANDE** — uv-free painting |
| Paint across multiple UV tiles | Automatic multi-tile management, per-tile resolution | Nessuno | **GRANDE** — UDIM-style workflow |
| Gigatexel Engine | Seamless handling of enormous texture datasets, automatic tile loading | Nessuno (texture limitato a singola immagine) | **GRANDE** — texture ad alta risoluzione |

### 2.2 Sculpting Tools  [PRIORITÀ 2]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Sculpt Tools Tray | Draw, Smooth, Grab, Pinch, Flatten, Wax, Scrape, Fill, Contrast, Bulge, Imprint, Reduce, Refine, Remesh | 9 sculpt brush in ksModeler (Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate); nessuno in PaintEditor | **GRANDE** — sculpt non integrato nel paint workflow |
| Sculpt Layers | Layer-based sculpting with strength, visibility, lock, merge, mirror, mask | Nessuno | **GRANDE** — non-destructive sculpt |
| Subdivision Levels | Multi-resolution: subdivide per 4×, switch between levels | Subdivision solo come modifier in ksModeler | **GRANDE** — mancano livelli di subdiv |
| Dynamic Tessellation | Increase/decrease mesh resolution on-the-fly during stroke | Nessuno | **GRANDE** — topologia dinamica |
| Mask / Freeze regions | Mask brush to protect areas from sculpt/paint | Selection mask 2D (non su mesh) | **MEDIO** — mask 3D mancante |
| Sculpt using stencils | Project image as sculpt displacement via stencil | Nessuno | **GRANDE** — sculpt con stencil |
| Sculpt using maps | Displacement/normal map-driven sculpting | Nessuno | **MEDIO** |
| Symmetry sculpting | Mirror sculpt across axis | Symmetry in ksModeler (X/Y/Z) | **PARZIALE** — esiste nel modeler |
| Transfer sculpt detail | Transfer detail between meshes | Nessuno | **MEDIO** |
| Curves-based sculpt | Sculpt along curves | Nessuno | **BASSO** |

### 2.3 Paint Channels & Materials  [PRIORITÀ 3]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Multi-channel paint | Diffuse, Gloss, Specular, Reflection, Bump, Displacement per layer | Solo color (ARGB32); PBR material in ksModeler separato | **GRANDE** — canali materiali mancanti |
| Paint material properties | Paint gloss/specular/bump directly on model | Materiali PBR separati (editor a pannelli) | **GRANDE** — paint-to-material workflow |
| Layer opacity per channel | Opacity/transparency per paint layer | Opacity per layer (2D canvas) | **PARZIALE** |
| Export paint layers | Export per-channel as separate images | Export PNG/DDS singolo | **GRANDE** — multi-channel export |

### 2.4 Retopology  [PRIORITÀ 4]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Retopologize mesh | Automatic retopology with curve-guided topology flow | Nessuno | **GRANDE** — retopology workflow |
| Guide curves for topology | User-placed curves control edge flow | Nessuno | **GRANDE** |
| Transfer sculpted detail | Project detail from high-poly to retopologized mesh | Nessuno | **GRANDE** — normal/displacement bake |
| Symmetry in retopology | Topological or axis-based symmetry | Nessuno | **MEDIO** |

### 2.5 UV Tools  [PRIORITÀ 5]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Create UVs from mesh | Auto-generate rough UVs | Unwrap LSCM in ksModeler (non nel paint editor) | **MEDIO** — UV tools nel modeler, non nel paint |
| UV View | 2D texture space + UV layout display | Nessuna UV view | **GRANDE** — manca visualizzazione 2D |
| Multi-tile UV (UDIM) | Automatic tile management | Nessuno | **GRANDE** |
| Paint across UV tiles | Seamless painting across tile boundaries | Nessuno | **GRANDE** |

### 2.6 Stencils & Projection  [PRIORITÀ 6]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Stencil tray | Built-in library of stencil images | Nessuno | **GRANDE** |
| Projection brush | Project stencil image onto 3D model as paint | Stamp tool (2D only) | **GRANDE** — 3D projection mancante |
| Stencil positioning | Move/rotate/scale stencil in 3D view with hotkeys | Nessuno | **GRANDE** |
| Custom stencils | Load any image as stencil | Nessuno | **MEDIO** |
| Stencil for sculpt | Use stencils to displace vertices | Nessuno | **GRANDE** |

### 2.7 Layers Avanzate  [PRIORITÀ 7]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Sculpt + Paint layers unified | Single Layers window with toggle Sculpt/Paint view | Layers solo per paint 2D; sculpt layers in ksModeler separato | **GRANDE** — unificazione mancante |
| Layer groups | Group sculpt/paint layers | Nessuno | **MEDIO** |
| Layer strength (sculpt) | Amplify/reduce/invert sculpt per layer | Opacity per layer (2D) | **PARZIALE** |
| Layer merge | Merge two or more layers | Merge down (2D canvas) | **PARZIALE** |
| Layer mask | Paint mask on layer to hide/show sculpt detail | Selection mask 2D | **MEDIO** |
| Layer lock | Lock layer to prevent edits | Nessuno | **MEDIO** |
| Layer mirror | Mirror sculpt layer on symmetrical model | Nessuno | **MEDIO** |
| Freeze mesh based on layer | Freeze sculpt layer for performance | Nessuno | **BASSO** |

### 2.8 Texture Baking  [PRIORITÀ 8]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Extract normal map | Bake sculpt detail to normal map | Nessuno | **GRANDE** — normal map bake |
| Extract displacement map | Bake sculpt detail to displacement map | Nessuno | **GRANDE** — displacement bake |
| Extract ambient occlusion | Bake AO from sculpt | Nessuno | **GRANDE** |
| Bake between meshes | Transfer texture from source to target mesh | Nessuno | **MEDIO** |
| Map resolution control | Per-channel resolution settings | Nessuno | **MEDIO** |

### 2.9 Import / Export  [PRIORITÀ 9]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| Import .mud files | Native Mudbox format | Nessuno | **BASSO** — formato proprietario |
| Import OBJ/FBX/STL | Standard 3D formats | OBJ/KN5/GLTF/GLB (solo preview) | **PARZIALE** — import per preview |
| Export OBJ with paint | Export painted mesh | Export solo texture | **MEDIO** |
| Export paint layers as images | Per-layer, per-channel export | Export PNG/DDS singolo | **GRANDE** — multi-layer export |
| Image format support | PSD, TIFF, PNG, EXR, TGA, BMP, JPEG | PNG, DDS | **MEDIO** — mancano PSD/TIFF/EXR |

### 2.10 UX e Workflow  [PRIORITÀ 10]

| Feature | Mudbox | ksPaintEditor | Gap |
|---------|--------|---------------|-----|
| 3D viewport painting | Paint while orbiting/zooming 3D model | Paint su 2D canvas, preview 3D separato | **CRITICO** — workflow completamente diverso |
| Quick-select trays | Number keys 1-9 for tool selection | Shortcut per tool (diversi) | **PARZIALE** |
| Properties window | Tool properties docked panel | Brush options bar | **PARZIALE** |
| Object List | Scene hierarchy with visibility/lock | Outliner nel modeler, non nel paint | **MEDIO** |
| Image Browser | Browse/manage texture images | Nessuno | **MEDIO** |
| Mudbox file format | Single .mud file with embedded layers | .ks3d + skin folders | **DIVERSO** — formato diverso |
| symmetry painting | Paint across symmetry axis | Nessuno | **MEDIO** |

---

## 3. Gap minori / placeholder noti

Rilevati durante l'analisi:

| Area | Problema |
|---|---|
| 3D paint stroke | Nessuna paint stroke interattiva su mesh 3D |
| UV view | Nessuna visualizzazione 2D della texture + UV layout |
| Multi-channel material paint | Paint solo colore; bump/specular/gloss mancanti |
| Stencil system | Nessuna libreria stencil o projection brush |
| Sculpt layers | Sculpt layer system mancante nel paint workflow |
| Retopology | Nessun tool di retopology |
| Texture baking | Nessuna bake normal/displacement/AO |
| PTEX | Nessun painting UV-free |
| UDIM | Nessun supporto multi-tile |
| Layer groups/mask/lock | Funzionalità layer avanzate mancanti |

---

## 4. Priorità consigliate

### Fase 1 — 3D Paint Core (gap critici)
1. **3D paint stroke** — paint interattiva direttamente sulla superficie mesh nel viewport 3D
2. **UV-mapped paint layer** — layer system che scrive su texture UV-mapped (non solo 2D canvas)
3. **UV View** — visualizzazione 2D della texture con layout UV
4. **Multi-channel paint** — paint su canali diffuse/specular/bump/displacement per layer

### Fase 2 — Sculpt Integration
5. **Sculpt layers** — layer system per sculpt con strength, visibility, merge
6. **Subdivision levels** — multi-resolution con switch tra livelli
7. **Dynamic tessellation** — increase/decrease risoluzione durante sculpt stroke
8. **Mask / Freeze** — mask brush per proteggere aree da sculpt/paint

### Fase 3 — Stencils & Projection
9. **Stencil tray** — libreria stencil con built-in images
10. **Projection brush** — project stencil image come paint su mesh 3D
11. **Stencil positioning** — move/rotate/scale stencil in 3D view
12. **Sculpt using stencils** — stencil come displacement map per sculpt

### Fase 4 — Retopology & Baking
13. **Retopologize** — auto-retopology con curve-guided flow
14. **Extract normal map** — bake sculpt detail → normal map
15. **Extract displacement map** — bake sculpt detail → displacement map
16. **Extract AO** — bake ambient occlusion da sculpt
17. **Transfer detail** — project detail tra meshes

### Fase 5 — Advanced Layers & Export
18. **Layer groups** — raggruppa paint/sculpt layers
19. **Layer mask** — mask per-paint layer
20. **Layer lock** — lock layer per prevenire edit
21. **Multi-layer export** — export per-layer, per-channel come immagini separate
22. **PSD/TIFF/EXR export** — formati aggiuntivi per pipeline professionali

---

## 5. Definizione di "done" per la roadmap

### Fase 1 — 3D Paint Core

#### 3D paint stroke
- [ ] Raycast su mesh nel viewport 3D → UV coordinate del punto colpito
- [ ] Paint stroke che scrive sulla texture UV-mapped in tempo reale
- [ ] Brush cursor visibile sulla superficie mesh (normale-aligned)
- [ ] Supporto per tutti i brush esistenti (Brush, Eraser, Clone, Smudge, Blur, etc.)
- [ ] Undo/redo delle paint strokes su mesh 3D

#### UV-mapped paint layer
- [ ] Modello dati `PaintLayer3D` con riferimento a mesh + UV set + texture bitmap
- [ ] Creazione layer con risoluzione configurabile (512–8192)
- [ ] Layer visibile/nascosto, opacity, blend mode su mesh 3D
- [ ] Persistenza paint layers nel `.ks3d` (aux JSON)

#### UV View
- [ ] Pannello UV View con visualizzazione 2D texture + UV wireframe overlay
- [ ] Paint interattiva anche in UV View (2D brush su texture)
- [ ] Sync bidirezionale: 3D paint → UV View si aggiorna, e viceversa
- [ ] Zoom/pan nella UV View

#### Multi-channel paint
- [ ] Enum `PaintChannel` (Diffuse, Specular, Gloss, Bump, Displacement, Opacity)
- [ ] Per-layer: channel type selezionabile alla creazione
- [ ] Material preview che combina tutti i canali paint
- [ ] Export per-canale come immagine separata

### Fase 2 — Sculpt Integration

#### Sculpt layers
- [ ] Modello dati `SculptLayer` con mesh delta (vertex offsets per subdivision level)
- [ ] Strength per layer (0–200%, invertibile negativo)
- [ ] Visibility, lock, merge, delete per sculpt layer
- [ ] UI: toggle Sculpt/Paint nella stessa Layers window

#### Subdivision levels
- [ ] Catmull-Clark subdivision multi-level con persistenza
- [ ] Switch tra livelli senza bake
- [ ] Sculpt su qualsiasi livello, propagazione automatica

#### Dynamic tessellation
- [ ] Tessellation adaptiva lungo lo sculpt stroke
- [ ] Falloff alignment con sculpt tool corrente
- [ ] Reduce tool per diminuire risoluzione

#### Mask / Freeze
- [ ] Mask brush per proteggere aree da sculpt/paint
- [ ] Mask visibile come overlay grayscale
- [ ] Freeze basato su sculpt layer per performance

### Fase 3 — Stencils & Projection

#### Stencil tray
- [ ] Libreria built-in di stencil images (skin, leather, fabric, rock, etc.)
- [ ] Caricamento custom stencil da file
- [ ] UI: tray con thumbnails + Off button

#### Projection brush
- [ ] Stencil projection su mesh 3D: position/rotate/scale stencil in viewport
- [ ] Paint che passa attraverso il stencil (color da immagine → mesh)
- [ ] Hotkeys: S+drag per scale, S+RMB per rotate, S+MMB per move

#### Sculpt using stencils
- [ ] Stencil come heightmap per displacement durante sculpt stroke
- [ ] Intensity configurabile

### Fase 4 — Retopology & Baking

#### Retopologize
- [ ] Auto-retopology da high-poly mesh
- [ ] Curve-guided topology flow (user-placed curves)
- [ ] Output: mesh con quad uniformi + UV transfer
- [ ] Opzione symmetry (axis-based o topology-based)

#### Texture baking
- [ ] Extract normal map (high-poly → low-poly via UV)
- [ ] Extract displacement map
- [ ] Extract ambient occlusion
- [ ] Resolution settings per bake
- [ ] Bake between different meshes

### Fase 5 — Advanced Layers & Export

#### Layer groups
- [ ] Raggruppa paint/sculpt layers in folder
- [ ] Toggle visibilità gruppo
- [ ] Opacity del gruppo

#### Layer mask
- [ ] Paint mask su singolo paint layer (grayscale)
- [ ] Mask editabile con brush
- [ ] Import/export mask come immagine

#### Layer lock
- [ ] Lock per layer singolo (impedisce paint/sculpt)
- [ ] Lock visuale nella UI

#### Multi-layer export
- [ ] Export per-layer come immagine separata
- [ ] Export per-channel (diffuse/specular/bump) come set di mappe
- [ ] Formati: PNG, TIFF, EXR, PSD

---

## 6. Confronto filosofico: 2D Paint Editor vs 3D Sculpt/Paint Tool

| Aspetto | Mudbox (3D Sculpt/Paint) | ksPaintEditor (2D Livery) |
|---------|--------------------------|---------------------------|
| **Paradigma** | Paint e sculpt direttamente sulla mesh 3D | Paint 2D su canvas, preview 3D read-only |
| **Target** | Character/creature artists, VFX, game studios | Livery/car painting per racing games |
| **Sculpt** | 15+ sculpt tools, layers, subdivision, tessellation | 9 brush in ksModeler (separato) |
| **Paint channels** | Diffuse, gloss, specular, bump, displacement | Solo colore ARGB32 |
| **UV workflow** | UV View, multi-tile, PTEX uv-free | UV tools in ksModeler (separato) |
| **Stencil** | Projection brush, stencil tray, sculpt stencils | Stamp 2D (base) |
| **Retopology** | Auto-retopology + curve-guided | Nessuno |
| **Baking** | Normal/displacement/AO bake | Nessuno |
| **File format** | .mud (embedded layers) | .ks3d + skin folders + PNG/DDS |
| **Layers** | Unified sculpt+paint layers, groups, mask, lock | Paint layers 2D (12 blend modes) |
| **Costo** | $10/mese o $100/anno | Open source / integrato in ksEditor |
| **Focus** | Sculpt + paint per character/environment | Livery painting per Assetto Corsa |

### Strategia consigliata
Mudbox eccelle in sculpt e 3D paint per character art; ksPaintEditor è un paint tool 2D per livery racing. La strategia ottimale è:

1. **Aggiungere 3D paint stroke** — il gap più critico; permette di paint direttamente sulla mesh
2. **Integrare sculpt layers nel paint workflow** — unificare sculpt e paint layers nella stessa UI
3. **Aggiungere UV View** — visualizzazione 2D essenziale per texture work
4. **Multi-channel paint** — paint su canali materiali (specular/bump/displacement)
5. **Stencil projection** — workflow Mudbox-style per detail painting
6. **Non replicare retopology/baking** — questi sono feature da DCC; ksEditor può fare a meno se il focus resta livery

---

## 7. Note implementative

### 3D paint stroke
- Il viewport 3D (`PaintViewport`) già carica mesh e mostra vertex colors da UV
- Serve aggiungere raycast interattivo: mouse → ray → triangle hit → barycentric → UV coordinate
- La texture bitmap va aggiornata in tempo reale al painting (dirty rect optimization)
- Brush cursor va proiettato sulla superficie mesh con normal alignment

### UV View
- QML: nuovo pannello con Image canvas + UV wireframe overlay da mesh data
- Paint interattiva in UV View: 2D brush su texture con sync a 3D viewport
- Il sistema UV già esiste in ksModeler (`MeshOperations::unwrapUVs`); va esposto nel paint editor

### Multi-channel paint
- Ogni `PaintLayer3D` ha un channel type; il material preview combina tutti i canali
- Bump/normal possono essere generati dal grayscale del paint layer
- Export per-canale: split del composite in N immagini

### Sculpt layers
- Il sculpt system in ksModeler già ha 9 brush reali (Draw/Smooth/Grab/Flatten/Crease/Inflate/Pinch/Smear/Negate); va esteso con layer system
- Mesh delta per layer: salva solo gli offset dei vertici modificati
- Strength come moltiplicatore sugli offset; invertibile (negativo)

### Stencil projection
- Il `PaintPainter` già ha `Stamp` tool; va esteso a 3D projection su mesh
- Stencil tray: QListWidget con thumbnails di immagini built-in + custom load
- Positioning: gizmo 2D overlay nel viewport 3D (translate/rotate/scale)

### Retopology
- OpenCASCADE o libigl forniscono retopology algorithms
- Curve-guided: servono curve 3D posizionate sulla mesh come guide per edge flow
- Transfer detail: projection + UV baking (raycast high→low poly)

### Texture baking
- Raycast high-poly → low-poly per normal/displacement/AO
- UV coordinates del low-poly determinano la posizione nel bake map
- Risoluzione configurabile per bake output
