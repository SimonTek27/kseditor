#pragma once

#include <QObject>
#include <QImage>
#include <QRect>
#include <QVector>
#include "PaintTypes.h"

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

    // Selection
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

    Snapshot snapshot() const;
    void restore(const Snapshot& snap);
};

} // namespace paint
} // namespace ks