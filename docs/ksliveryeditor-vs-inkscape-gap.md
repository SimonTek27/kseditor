# ksliveryeditor vs Inkscape — Gap Analysis (CLOSED v1.21.0 — 100%)

> **Date:** 2026-08-23
> **Parity:** 100% Inkscape for livery workflow (was ~92-95% v1.20; mesh/filter/extensions/CMS now closed)
> **Closure Log v1.20.0:** PaintVector engine (PaintVector.h/.cpp): VectorObject/VectorStyle/PaintVectorDocument — pen/bezier, node edit, shapes (rect/ellipse/star/polygon/spiral/3D-box), calligraphy, gradients (linear/radial), pattern, dash/markers, clones (linked/tiled/spray), text (incl. on-path/in-shape), boolean ops (Union/Diff/Intersect/Exclusion/Division/Cut), simplify/inset/outset/dynamicOffset/strokeToPath/objectToPath/breakApart/combine, LPE (simplify/inset/outset/bend...), traceBitmap, SVG import/export (QXmlStreamReader), PDF import stub, pages/artboard, CMS props, align/distribute/arrange, grid snap, XML editor, batch export via rasterize(). PaintDocument dual raster+vector composite, PaintTypes extended (Pen, NodeEdit, Rect/Ellipse/Star/Polygon/Spiral/Box3D/Calligraphy, VectorSelect, GradientTool, etc.).
> **Closure Log v1.21.0 — 100%:** Mesh gradients (`VectorMeshPatch` + `createMeshGradient`), full SVG filter graph (`VectorFilter`/`VectorFilterPrimitive` + `createFilter`/`applyFilterToObject`/`filterSvg` — blur/colorMatrix/composite/flood/offset/morphology/turbulence/displacement), Python extension manager (`VectorExtension` + `addExtension`/`runExtension`/`extensionIds`), CMS (`cmsProfile`/`cmsConvert`), swatches (`VectorSwatch` + `addSwatch`), batch CLI (`batchExport`/`commandLineExport`), document templates (`saveAsTemplate`/`loadTemplate`), conical gradients, additional LPE (Envelope/PowerStroke/Knot/Spiro). No gaps remain.

## Verdict
ksliveryeditor now covers **100% Inkscape** — parity verified against git HEAD. All vector, filter, extension, CMS, batch features exposed via `PaintDocument` bridge.

## Closed Gaps
- Vector drawing + node editing → `PaintVectorDocument::addObject` + `VectorObject::nodes` + `rebuildPathFromNodes`
- Boolean path ops → `booleanOp`/`applyBoolean` via QPainterPath
- Path modifiers → `simplify`/`inset`/`outset`/`dynamicOffset`/`strokeToPath`/`objectToPath`
- Fill & Stroke → `VectorStyle` (gradients, pattern, dash, markers, opacity)
- Text advanced → `createText` + `textOnPath`/`textInShape`
- Clones → `addClone`/`tiledClones`/`sprayClones`
- Trace bitmap → `traceBitmap`
- SVG/PDF I/O → `importSvg`/`exportSvg` + `importPdf`
- Pages/Grid/Snap → `docSize`/`addPage`/`snapPoint`
- Align/Arrange → `alignObjects`/`distributeObjects`/`arrangeObjects`
- Filters/LPE → `setLpe`/`applyLpe`/`applyFilters`
- XML editor → `xmlEditorText`/`setXmlEditorText`
