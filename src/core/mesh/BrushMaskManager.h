#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QVector3D>
#include <QVector2D>

enum BrushMaskMode {
    MaskNone,      // No masking
    MaskVertex,    // Mask by vertex selection
    MaskFace,      // Mask by face selection
    MaskSculpt,    // Sculpt mode mask (per-layer weights)
    MaskPaint,     // Paint mode mask (per-layer masks)
    MaskArea,      // Mask by area/region
    MaskSymmetry   // Mirror mask (left/right)
};

class BrushMaskManager : public QObject
{
    Q_OBJECT
public:
    explicit BrushMaskManager(QObject* parent = nullptr);
    ~BrushMaskManager();
    
    // Mask mode management
    void setMaskMode(BrushMaskMode mode);
    BrushMaskMode maskMode() const { return m_maskMode; }
    
    // Selection-based masking
    void setSelectedVertices(const QVector<int>& vertices);
    void setSelectedFaces(const QVector<int>& faces);
    QVector<int> selectedVertices() const { return m_selectedVertices; }
    QVector<int> selectedFaces() const { return m_selectedFaces; }
    
    // Area/region masking
    void setMaskArea(const QVector3D& center, float radius);
    QVector3D maskCenter() const { return m_maskCenter; }
    float maskRadius() const { return m_maskRadius; }
    
    // Symmetry masking
    void setSymmetryEnabled(bool enabled);
    bool symmetryEnabled() const { return m_symmetryEnabled; }
    
    // Apply mask to brush operation
    // Returns which vertices should be affected based on current mask
    QSet<int> applyMaskToBrush(int objectId, const QVector3D& center, float radius, 
                               float strength, int brushMode) const;

    // Set vertex positions for real distance calculations
    void setVertexPositions(const QVector<QVector3D>& positions);
    void setVertexNormals(const QVector<QVector3D>& normals);
    void setSculptLayerWeights(const QVector<float>& weights);
    void setPaintLayerMasks(const QVector<float>& masks);
    void setSymmetryPlane(const QVector3D& normal, float offset);
    
    // Signals
signals:
    void maskModeChanged(BrushMaskMode mode);
    void selectionChanged();
    void maskAreaChanged(const QVector3D& center, float radius);
    void symmetryToggled(bool enabled);
    
private:
    BrushMaskMode m_maskMode = MaskNone;
    QVector<int> m_selectedVertices;
    QVector<int> m_selectedFaces;
    QVector3D m_maskCenter;
    float m_maskRadius = 0.0f;
    bool m_symmetryEnabled = false;
    QVector<QVector3D> m_vertexPositions;
    QVector<QVector3D> m_vertexNormals;
    QVector<float> m_sculptLayerWeights;
    QVector<float> m_paintLayerMasks;
    QVector3D m_symmetryPlaneNormal = QVector3D(1, 0, 0);
    float m_symmetryPlaneOffset = 0.0f;
};