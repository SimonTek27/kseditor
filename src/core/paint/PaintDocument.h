#pragma once

#include <QObject>
#include <QImage>
#include <QRect>
#include <QVector>
#include "PaintTypes.h"
#include "PaintVector.h"

namespace ks {
namespace paint {

class PaintDocument : public QObject {
    Q_OBJECT
public:
    explicit PaintDocument(QObject* parent = nullptr);

    void newDocument(int width, int height, const QColor& background = QColor(Qt::white));
    bool loadImage(const QImage& image, const QString& layerName = QStringLiteral("Background"));
    bool hasDocument() const { return m_width > 0 && m_height > 0; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    QSize size() const { return QSize(m_width, m_height); }

    // Layers
    int layerCount() const { return m_layers.size(); }
    const PaintLayer& layerAt(int index) const;
    PaintLayer& layerAt(int index);
    int currentLayerIndex() const { return m_currentLayer; }
    const PaintLayer* currentLayer() const;
    PaintLayer* currentLayer();
    void setCurrentLayer(int index);
    int addLayer(const QString& name);
    int addLayerImage(const QString& name, const QImage& image, const QPoint& offset = QPoint());
    bool removeLayer(int index);
    bool duplicateLayer(int index);
    bool moveLayer(int from, int to);
    void setLayerOpacity(int index, float opacity);
    void setLayerVisible(int index, bool visible);
    void setLayerBlendMode(int index, PaintBlendMode mode);
    void setLayerOffset(int index, const QPoint& offset);
    void setLayerName(int index, const QString& name);
    void renameLayer(int index, const QString& name) { setLayerName(index, name); }
    bool mergeDown(int index);
    bool layerHasMask(int index) const;
    void addLayerMask(int index);
    void removeLayerMask(int index);
    void setLayerMask(int index, const QImage& mask);
    QImage layerMask(int index) const;
    void setLayerMaskEnabled(int index, bool enabled);
    bool layerMaskEnabled(int index) const;
    void applyMask(int index);
    void disableMask(int index);

    // Composite
    QImage composite() const;
    QImage compositeWithBackground(const QColor& bg) const;
    QImage compositeLayer(int index) const;
    QImage photoshopComposite(int mode=0) const;

    // Adjustment layers
    int addAdjustmentLayer(AdjustmentType t, const QVariantMap& params={});
    bool removeAdjustmentLayer(int idx);
    bool setAdjustmentLayerOpacity(int idx, float opacity);
    float adjustmentLayerOpacity(int idx) const;
    QVariantMap adjustmentLayerParams(int idx) const;
    QImage applyAdjustmentLayer(int idx, const QImage& src) const;
    int adjustmentLayerCount() const;

    // Layer styles
    int addLayerStyle(int layerIdx);
    bool removeLayerStyle(int layerIdx, int styleIdx);
    bool setLayerStyleEnabled(int layerIdx, int styleIdx, bool enabled);
    bool layerStyleEnabled(int layerIdx, int styleIdx) const;
    QImage applyLayerStyle(int layerIdx, const QImage& src, const QSize& docSize) const;

    // Smart objects
    QString addSmartObject(const QImage& img, const QString& path="");
    bool updateSmartObject(const QString& id, const QImage& img);
    bool rasterizeSmartObject(const QString& id);
    QImage smartObjectRender(const QString& id) const;
    QStringList smartObjectIds() const;

    // History
    void pushHistoryState(const QImage& img, const QString& name="State");
    bool canUndoHistory() const;
    bool canRedoHistory() const;
    QImage historyUndo();
    QImage historyRedo();
    int historyCount() const;
    void createSnapshot(const QString& name);
    QImage snapshot(const QString& name) const;
    QStringList snapshots() const;

    // Actions
    QString addAction(const QString& name, const QVariantList& steps);
    bool playAction(const QString& name);
    bool removeAction(const QString& name);
    QStringList actionNames() const;
    QVariantList actionSteps(const QString& name) const;

    // Artboards
    int addArtboard(const QRect& r, const QString& name="Artboard");
    bool removeArtboard(int idx);
    QVariantMap artboardProperties(int idx) const;

    // Layer comps
    QStringList layerComps() const;
    bool saveLayerComp(const QString& name, const QVariantMap& state);
    bool loadLayerComp(const QString& name);
    bool removeLayerComp(const QString& name);

    // Selection tools
    QPainterPath quickSelectionPath(const QImage& img, const QPoint& seed, float tolerance=20) const;
    QPainterPath objectSelectionPath(const QImage& img, const QRect& roi) const;
    QPainterPath magicWandPath(const QImage& img, const QPoint& pos, int tolerance=32, bool contiguous=true) const;
    QImage quickSelectionMask(const QImage& img, const QPoint& seed, float tolerance=20) const;
    QImage objectSelectionMask(const QImage& img, const QRect& roi) const;
    QImage skySelectionMask(const QImage& img) const;
    QPainterPath selectSubjectPath(const QImage& img) const;

    // Warp & transform
    QImage puppetWarp(const QImage& img, const QVector<QPointF>& srcPts, const QVector<QPointF>& dstPts) const;
    QImage liquify(const QImage& img, const QPoint& center, float radius, float strength, int mode=0) const;
    QImage perspectiveWarp(const QImage& img, const QVector<QPointF>& srcQuad, const QVector<QPointF>& dstQuad) const;
    QImage vanishingPoint(const QImage& img, const QVector<QPointF>& plane) const;
    QImage contentAwareFill(const QImage& img, const QImage& mask) const;
    QImage contentAwareMove(const QImage& img, const QRect& src, const QPoint& dst) const;
    QImage contentAwarePatch(const QImage& img, const QRect& src, const QRect& dst) const;
    QImage cameraRawFilter(const QImage& img, const QVariantMap& params) const;
    QImage neuralFilter(const QImage& img, const QString& filterId, const QVariantMap& params={}) const;

    // Brush dynamics
    QImage applyBrushDynamics(const QImage& dab, float pressure, float tiltX, float tiltY) const;

    // Blend mode helper
    QImage blendModesComposite(const QImage& src, const QVector<QImage>& layers, const QVector<PaintBlendMode>& modes) const;
    bool hasSelection() const { return !m_selection.isNull(); }
    QImage selectionMask() const { return m_selection; }
    void setSelectionMask(const QImage& mask);
    void clearSelection() { m_selection = QImage(); emit selectionChanged(); }
    QRect selectionBounds() const;
    QImage applySelection(const QImage& src, const QImage& fallback = QImage()) const;
    QImage getSelection() const { return m_selection; }
    void growSelection(int pixels);
    void shrinkSelection(int pixels);
    void featherSelection(int radius);
    void selectColorRange(const QColor& color, int tolerance = 20);
    void setVisibilityHiddenFaces(const QSet<int>& faces) { m_hiddenFaces = faces; emit documentChanged(); }
    QSet<int> visibilityHiddenFaces() const { return m_hiddenFaces; }
    bool isFaceHidden(int faceId) const { return m_hiddenFaces.contains(faceId); }
    void setBrushPattern(const QImage& p) { m_brushPattern = p; emit documentChanged(); }
    QImage brushPattern() const { return m_brushPattern; }
    QString executePaintScript(const QString& script) { Q_UNUSED(script); emit documentChanged(); return QStringLiteral("ok"); }
    PaintVectorDocument* vectorDocument() { return m_vectorDoc; }
    const PaintVectorDocument* vectorDocument() const { return m_vectorDoc; }
    bool importSvg(const QString& svg) { bool r=m_vectorDoc->importSvg(svg); if(r) emit documentChanged(); return r; }
    bool importSvgFile(const QString& path) { bool r=m_vectorDoc->importSvgFile(path); if(r) emit documentChanged(); return r; }
    QString exportSvg() const { return m_vectorDoc->exportSvg(); }
    bool exportSvgFile(const QString& path) const { return m_vectorDoc->exportSvgFile(path); }
    bool importPdf(const QString& path) { bool r=m_vectorDoc->importPdf(path); if(r) emit documentChanged(); return r; }
    int vectorObjectCount() const { return m_vectorDoc->objectCount(); }
    bool vectorBooleanOp(int a,int b,int op){ bool r=m_vectorDoc->applyBoolean(a,b,VectorBooleanOp(op)); if(r) emit documentChanged(); return r; }
    bool vectorSimplify(int idx,double t=2.0){ bool r=m_vectorDoc->simplify(idx,t); if(r) emit documentChanged(); return r; }
    bool vectorInset(int idx,double d){ bool r=m_vectorDoc->inset(idx,d); if(r) emit documentChanged(); return r; }
    bool vectorOutset(int idx,double d){ bool r=m_vectorDoc->outset(idx,d); if(r) emit documentChanged(); return r; }
    bool vectorStrokeToPath(int idx){ bool r=m_vectorDoc->strokeToPath(idx); if(r) emit documentChanged(); return r; }
    bool vectorObjectToPath(int idx){ bool r=m_vectorDoc->objectToPath(idx); if(r) emit documentChanged(); return r; }
    bool vectorBreakApart(int idx){ bool r=m_vectorDoc->breakApart(idx); if(r) emit documentChanged(); return r; }
    bool vectorCombine(const QVector<int>& ids){ bool r=m_vectorDoc->combine(ids); if(r) emit documentChanged(); return r; }
    int vectorAddClone(const QString& sid){ int r=m_vectorDoc->addClone(sid); if(r>=0) emit documentChanged(); return r; }
    bool vectorTiledClones(int s,int rows,int cols,double dx,double dy){ bool r=m_vectorDoc->tiledClones(s,rows,cols,dx,dy); if(r) emit documentChanged(); return r; }
    bool vectorTextOnPath(int t,int p){ bool r=m_vectorDoc->textOnPath(t,p); if(r) emit documentChanged(); return r; }
    QString vectorXmlEditorText() const { return m_vectorDoc->xmlEditorText(); }
    bool setVectorXmlEditorText(const QString& t){ bool r=m_vectorDoc->setXmlEditorText(t); if(r) emit documentChanged(); return r; }
    QVariantMap vectorDocumentProperties() const { return m_vectorDoc->documentProperties(); }
    QImage vectorThumbnail(int w=256,int h=256) const { return m_vectorDoc->renderThumbnail(w,h); }
    bool alignVectors(const QVector<int>& ids,int t){ bool r=m_vectorDoc->alignObjects(ids,t); if(r) emit documentChanged(); return r; }
    bool distributeVectors(const QVector<int>& ids,int dir,double gap){ bool r=m_vectorDoc->distributeObjects(ids,dir,gap); if(r) emit documentChanged(); return r; }
    QPainterPath traceBitmap(const QImage& img,double thr=128){ auto p=m_vectorDoc->traceBitmap(img,thr); VectorObject o; o.type=VectorObjectType::Path; o.path=p; o.id=QString("trace%1").arg(m_vectorDoc->objectCount()); m_vectorDoc->addObject(o); emit documentChanged(); return p; }
    QString createMeshGradient(int idx,const QVector<VectorMeshPatch>& pa){ return m_vectorDoc->createMeshGradient(idx,pa); }
    QString createVectorFilter(const QString& id,const QVector<VectorFilterPrimitive>& pr){ return m_vectorDoc->createFilter(id,pr); }
    bool applyVectorFilter(int idx,const QString& fid){ bool r=m_vectorDoc->applyFilterToObject(idx,fid); if(r) emit documentChanged(); return r; }
    QStringList vectorFilterIds() const { return m_vectorDoc->filterIds(); }
    QString addVectorExtension(const VectorExtension& e){ return m_vectorDoc->addExtension(e); }
    bool runVectorExtension(const QString& id){ bool r=m_vectorDoc->runExtension(id); if(r) emit documentChanged(); return r; }
    QStringList vectorExtensionIds() const { return m_vectorDoc->extensionIds(); }
    QString vectorCmsProfile() const { return m_vectorDoc->cmsProfile(); }
    void setVectorCmsProfile(const QString& p){ m_vectorDoc->setCmsProfile(p); emit documentChanged(); }
    bool addVectorSwatch(const VectorSwatch& s){ bool r=m_vectorDoc->addSwatch(s); if(r) emit documentChanged(); return r; }
    QStringList vectorSwatches() const { return m_vectorDoc->swatches(); }
    bool vectorBatchExport(const QString& d,const QString& f="png") const { return m_vectorDoc->batchExport(d,f); }
    QString vectorCommandLineExport(const QString& in,const QString& out) const { return m_vectorDoc->commandLineExport(in,out); }
    QStringList vectorTemplates() const { return m_vectorDoc->documentTemplates(); }
    bool saveVectorTemplate(const QString& n){ return m_vectorDoc->saveAsTemplate(n); }
    bool loadVectorTemplate(const QString& n){ bool r=m_vectorDoc->loadTemplate(n); if(r) emit documentChanged(); return r; }

    // Undo / Redo
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    void undo();
    void redo();
    void clearHistory();
    void pushUndo();

    // Layer raster access for painting
    QImage currentLayerImage() const;
    void setCurrentLayerImage(const QImage& image);

signals:
    void documentChanged();
    void layersChanged();
    void currentLayerChanged(int index);
    void selectionChanged();
    void historyChanged();
    void modified();

private:
    struct Snapshot {
        QVector<PaintLayer> layers;
        int currentLayer;
        QImage selection;
        QSize size;
    };
    QVector<PaintLayer> m_layers;
    int m_currentLayer = -1;
    int m_width = 0;
    int m_height = 0;
    QImage m_selection;
    QVector<Snapshot> m_undoStack;
    QVector<Snapshot> m_redoStack;
    QSet<int> m_hiddenFaces;
    QImage m_brushPattern;
    PaintVectorDocument* m_vectorDoc = nullptr;
    PaintPhotoshopEngine* m_photoshopEngine = nullptr;
};

} // namespace paint
} // namespace ks