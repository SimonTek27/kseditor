#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QPointF>

namespace ks {

class TerrainEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTerrain READ currentTerrain NOTIFY currentTerrainChanged)
    Q_PROPERTY(int terrainWidth READ terrainWidth NOTIFY terrainSizeChanged)
    Q_PROPERTY(int terrainHeight READ terrainHeight NOTIFY terrainSizeChanged)
    Q_PROPERTY(float brushRadius READ brushRadius WRITE setBrushRadius NOTIFY brushSettingsChanged)
    Q_PROPERTY(float brushStrength READ brushStrength WRITE setBrushStrength NOTIFY brushSettingsChanged)
    Q_PROPERTY(QString brushType READ brushType WRITE setBrushType NOTIFY brushSettingsChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoAvailableChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY redoAvailableChanged)

public:
    static TerrainEditorQmlBridge* instance();

    QString currentTerrain() const;
    int terrainWidth() const;
    int terrainHeight() const;
    float brushRadius() const;
    float brushStrength() const;
    QString brushType() const;
    bool canUndo() const;
    bool canRedo() const;

    void setBrushRadius(float r);
    void setBrushStrength(float s);
    void setBrushType(const QString& type);

    Q_INVOKABLE bool loadTerrain(const QString& heightmapPath);
    Q_INVOKABLE bool saveTerrain(const QString& heightmapPath);
    Q_INVOKABLE bool createNewTerrain(int width, int height);
    Q_INVOKABLE void applyBrush(float x, float y, float delta);
    Q_INVOKABLE void applyBrushStroke(const QVariantList& points, float totalDelta);
    Q_INVOKABLE void smoothTerrain(float x, float y, float radius);
    Q_INVOKABLE void flattenTerrain(float x, float y, float radius, float targetHeight);
    Q_INVOKABLE void addNoise(float x, float y, float radius, float intensity);
    Q_INVOKABLE float getHeight(float x, float y) const;
    Q_INVOKABLE void setHeight(float x, float y, float height);
    Q_INVOKABLE QVariantMap getTerrainStats() const;
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void exportToOBJ(const QString& path, float scale = 1.0f);
    Q_INVOKABLE void exportToPNG(const QString& path);
    Q_INVOKABLE void importFromPNG(const QString& path);
    Q_INVOKABLE QVariantList getHeightmapData() const;

signals:
    void currentTerrainChanged();
    void terrainSizeChanged();
    void brushSettingsChanged();
    void terrainModified();
    void undoAvailableChanged(bool available);
    void redoAvailableChanged(bool available);

private:
    static TerrainEditorQmlBridge* s_instance;
    TerrainEditorQmlBridge(QObject* parent = nullptr);
};

} // namespace ks
