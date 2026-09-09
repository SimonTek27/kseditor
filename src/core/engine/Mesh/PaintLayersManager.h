#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QVector2D>

struct PaintLayer {
    QString name;
    bool visible = true;
    float opacity = 1.0f;
    bool locked = false;
    float blendMode = 0.0f; // 0=Normal(alpha), 1=Multiply, 2=Screen, 3=Overlay, 4=SoftLight
    QVector<float> mask;    // Per-layer mask (vertex weights)
    
    PaintLayer() {}
    PaintLayer(const QString& n, bool vis = true, float opac = 1.0f, 
               bool lock = false, float blend = 0.0f)
        : name(n), visible(vis), opacity(opac), locked(lock), blendMode(blend) {}
};

class PaintLayersManager : public QObject
{
    Q_OBJECT
public:
    explicit PaintLayersManager(QObject* parent = nullptr);
    ~PaintLayersManager();
    
    // Layer management
    int addLayer(const QString& name);
    bool removeLayer(int index);
    bool renameLayer(int index, const QString& newName);
    void setLayerVisible(int index, bool visible);
    void setLayerLocked(int index, bool locked);
    void setLayerBlendMode(int index, float mode); // 0-4
    void setLayerOpacity(int index, float opacity);
    
    // Current layer
    int currentLayer() const { return m_currentLayer; }
    void setCurrentLayer(int index);
    
    // All layers
    const QVector<PaintLayer>& layers() const { return m_layers; }
    int layerCount() const { return m_layers.size(); }
    
    // Mask management per vertex
    void setVertexMask(int layerIndex, int vertexIndex, float weight);
    float vertexMask(int layerIndex, int vertexIndex) const;
    QVector<float> vertexMasks(int layerIndex) const;
    
    // Signals
signals:
    void layerAdded(int index);
    void layerRemoved(int index);
    void layerRenamed(int index, const QString& newName);
    void layerVisibilityChanged(int index, bool visible);
    void layerLockedChanged(int index, bool locked);
    void layerBlendModeChanged(int index, float mode);
    void layerOpacityChanged(int index, float opacity);
    void currentLayerChanged(int index);
    void layersChanged();
    
private:
    QVector<PaintLayer> m_layers;
    int m_currentLayer = -1;
};