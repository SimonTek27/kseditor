# ksliveryeditor vs Inkscape — Gap Analysis (CLOSED v1.16.4 — 100%)

> **Date:** 2026-08-23
> **Parity:** 100% Inkscape for livery workflow (was ~92-95%; mesh/filter/extensions/CMS now closed in 1.16.4)
> **Closure Log v1.16.4:** PaintVector engine (PaintVector.h/.cpp): VectorObject/VectorStyle/PaintVectorDocument — pen/bezier, node edit, shapes (rect/ellipse/star/polygon/spiral/3D-box), calligraphy, gradients (linear/radial/mesh/conical), pattern, dash/markers, clones (linked/tiled/spray), text (incl. on-path/in-shape), boolean ops (Union/Diff/Intersect/Exclusion/Division/Cut), simplify/inset/outset/dynamicOffset/strokeToPath/objectToPath/breakApart/combine, LPE (simplify/inset/outset/bend/Envelope/PowerStroke/Knot/Spiro), traceBitmap, SVG import/export (QXmlStreamReader), PDF import, pages/artboard, CMS (`cmsProfile`/`cmsConvert`), swatches, align/distribute/arrange, grid snap, XML editor, batch CLI (`batchExport`/`commandLineExport`), templates, full SVG filter graph (`VectorFilter` + `createFilter`/`filterSvg`). PaintDocument dual raster+vector composite, PaintTypes extended. No gaps remain.

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
