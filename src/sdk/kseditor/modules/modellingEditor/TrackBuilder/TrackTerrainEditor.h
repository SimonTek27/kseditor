#ifndef TRACK_TRACKTERRAINEDITOR_H
#define TRACK_TRACKTERRAINEDITOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QImage>
#include <QVector3D>
#include <QVariantMap>
#include <QPixmap>

namespace ks {

enum class TerrainBrushType {
    Raise,
    Lower,
    Smooth,
    Flatten,
    Noise,
    Paint
};

enum class TerrainBrushShape {
    Circle,
    Square,
    Linear
};

struct TerrainBrushSettings {
    TerrainBrushType type = TerrainBrushType::Raise;
    TerrainBrushShape shape = TerrainBrushShape::Circle;
    float radius = 5.0f;
    float strength = 0.5f;
    float hardness = 0.5f;
    int resolution = 256;
    QString texturePath;
};

class TrackTerrainEditor : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTerrain READ currentTerrain NOTIFY currentTerrainChanged)
    Q_PROPERTY(int terrainWidth READ terrainWidth NOTIFY terrainSizeChanged)
    Q_PROPERTY(int terrainHeight READ terrainHeight NOTIFY terrainSizeChanged)
    Q_PROPERTY(float brushRadius READ brushRadius WRITE setBrushRadius NOTIFY brushSettingsChanged)
    Q_PROPERTY(float brushStrength READ brushStrength WRITE setBrushStrength NOTIFY brushSettingsChanged)
    Q_PROPERTY(QString brushType READ brushType WRITE setBrushType NOTIFY brushSettingsChanged)

public:
    static TrackTerrainEditor* instance();

    QString currentTerrain() const { return m_currentTerrain; }
    int terrainWidth() const { return m_width; }
    int terrainHeight() const { return m_height; }
    float brushRadius() const { return m_brush.radius; }
    float brushStrength() const { return m_brush.strength; }
    QString brushType() const;

    void setBrushRadius(float r) { m_brush.radius = r; emit brushSettingsChanged(); }
    void setBrushStrength(float s) { m_brush.strength = s; emit brushSettingsChanged(); }
    void setBrushType(const QString& type);

    bool loadTerrain(const QString& heightmapPath);
    bool saveTerrain(const QString& heightmapPath);
    bool createNewTerrain(int width, int height);

    void applyBrush(float x, float y, float delta);
    void applyBrushStroke(const QVector<QPointF>& points, float totalDelta);
    void smoothTerrain(float x, float y, float radius);
    void flattenTerrain(float x, float y, float radius, float targetHeight);
    void addNoise(float x, float y, float radius, float intensity);

    float getHeight(float x, float y) const;
    void setHeight(float x, float y, float height);
    QVector3D getNormal(float x, float y) const;

    QImage getHeightmapImage() const;
    QPixmap getBrushPreview() const;
    QVariantMap getTerrainStats() const;

    void undo();
    void redo();
    bool canUndo() const { return m_undoIndex > 0; }
    bool canRedo() const { return m_undoIndex < m_undoStack.size() - 1; }

    void exportToOBJ(const QString& path, float scale = 1.0f);
    void exportToPNG(const QString& path);
    void importFromPNG(const QString& path);

signals:
    void currentTerrainChanged();
    void terrainSizeChanged();
    void brushSettingsChanged();
    void terrainModified();
    void heightChanged(float x, float y, float newHeight);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

private:
    static TrackTerrainEditor* s_instance;

    TrackTerrainEditor(QObject* parent = nullptr);

    void saveUndoState();
    void pushUndoState(const QVector<float>& heights);
    int heightToIndex(float x, float y) const;
    void indexToHeight(int index, float& x, float& y) const;
    QPixmap generateBrushPreview() const;

    QString m_currentTerrain;
    int m_width = 256;
    int m_height = 256;
    QVector<float> m_heights;
    TerrainBrushSettings m_brush;

    QVector<QVector<float>> m_undoStack;
    int m_undoIndex = 0;
    static const int MAX_UNDO_STATES = 20;
};

} // namespace ks

#endif
