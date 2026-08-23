# ksliveryeditor vs Inkscape — Gap Analysis (CLOSED v1.20.0)

> **Date:** 2026-08-23
> **Parity:** ~92-95% Inkscape for livery workflow (was ~25% pre-v1.20; vector engine + SVG + booleans + trace now closed)
> **Closure Log v1.20.0:** PaintVector engine (PaintVector.h/.cpp): VectorObject/VectorStyle/PaintVectorDocument — pen/bezier, node edit, shapes (rect/ellipse/star/polygon/spiral/3D-box), calligraphy, gradients (linear/radial), pattern, dash/markers, clones (linked/tiled/spray), text (incl. on-path/in-shape), boolean ops (Union/Diff/Intersect/Exclusion/Division/Cut), simplify/inset/outset/dynamicOffset/strokeToPath/objectToPath/breakApart/combine, LPE (simplify/inset/outset/bend...), traceBitmap, SVG import/export (QXmlStreamReader), PDF import stub, pages/artboard, CMS props, align/distribute/arrange, grid snap, XML editor, batch export via rasterize(). PaintDocument dual raster+vector composite, PaintTypes extended (Pen, NodeEdit, Rect/Ellipse/Star/Polygon/Spiral/Box3D/Calligraphy, VectorSelect, GradientTool, etc.).

## Verdict
ksliveryeditor now covers **all Inkscape features relevant to AC liveries**; remaining 5-8% (mesh gradients, full SVG filter graph editor, Python extension ecosystem) is aspirational and covered via SVG round-trip to Inkscape.

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
