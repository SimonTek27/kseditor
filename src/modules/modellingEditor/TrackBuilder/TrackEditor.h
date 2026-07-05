#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QImage>
#include <QVector3D>

namespace ks {

class TrackTerrain {
public:
    int width = 512;
    int height = 512;
    float worldWidth = 1000.0f;
    float worldHeight = 1000.0f;
    QVector<float> heightmap;
    QVector<QVector3D> normals;
    
    TrackTerrain() { reset(); }
    
    void reset() {
        heightmap.resize(width * height, 0.0f);
        normals.resize(width * height, QVector3D(0, 1, 0));
    }
    
    float getHeight(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return 0.0f;
        return heightmap[y * width + x];
    }
    
    float getHeightWorld(float wx, float wz) const {
        float nx = (wx / worldWidth + 0.5f) * width;
        float nz = (wz / worldHeight + 0.5f) * height;
        int x = static_cast<int>(nx);
        int y = static_cast<int>(nz);
        return getHeight(x, y);
    }
    
    void setHeight(int x, int y, float h) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        heightmap[y * width + x] = h;
    }
    
    float sampleBilinear(float wx, float wz) const {
        float nx = (wx / worldWidth + 0.5f) * (width - 1);
        float nz = (wz / worldHeight + 0.5f) * (height - 1);
        
        int x0 = static_cast<int>(nx);
        int z0 = static_cast<int>(nz);
        int x1 = x0 + 1;
        int z1 = z0 + 1;
        
        if (x1 >= width) x1 = width - 1;
        if (z1 >= height) z1 = height - 1;
        
        float fx = nx - x0;
        float fz = nz - z0;
        
        float h00 = getHeight(x0, z0);
        float h10 = getHeight(x1, z0);
        float h01 = getHeight(x0, z1);
        float h11 = getHeight(x1, z1);
        
        float h0 = h00 * (1 - fx) + h10 * fx;
        float h1 = h01 * (1 - fx) + h11 * fx;
        
        return h0 * (1 - fz) + h1 * fz;
    }
};

class TrackEditor : public QObject {
    Q_OBJECT
public:
    static TrackEditor* instance();
    
    void newTrack(const QString& name, int width, int height);
    bool loadTrack(const QString& path);
    bool saveTrack(const QString& path);
    void exportToImage(const QString& path);
    
    TrackTerrain* terrain() { return &m_terrain; }
    
    void raiseTerrain(float wx, float wz, float radius, float amount);
    void lowerTerrain(float wx, float wz, float radius, float amount);
    void smoothTerrain(float wx, float wz, float radius, float strength);
    void flattenTerrain(float wx, float wz, float radius, float targetHeight);
    void noiseTerrain(float wx, float wz, float radius, float scale, float amplitude);
    
    void setBrushSize(float size) { m_brushSize = size; }
    float brushSize() const { return m_brushSize; }
    void setBrushStrength(float s) { m_brushStrength = s; }
    float brushStrength() const { return m_brushStrength; }
    
    void recalculateNormals();
    QVector3D calculateNormal(int x, int y) const;
    
    QVector<float> getHeightmap() const { return m_terrain.heightmap; }
    QVector<QVector3D> getNormals() const { return m_terrain.normals; }
    
    void addSpline(const QString& name, const QVector<QVector3D>& points);
    QVector<QVector3D> getSplinePoints(const QString& name) const;
    QStringList getSplines() const { return m_splines.keys(); }
    
    void addMeshDecoration(const QString& name, const QVector3D& position, const QString& meshId);
    
signals:
    void terrainModified();
    void trackLoaded(const QString& path);
    void trackSaved(const QString& path);
    void trackNameChanged(const QString& name);

private:
    explicit TrackEditor(QObject* parent = nullptr);
    static TrackEditor* s_instance;
    
    QString m_trackName;
    TrackTerrain m_terrain;
    float m_brushSize = 10.0f;
    float m_brushStrength = 1.0f;
    
    QMap<QString, QVector<QVector3D>> m_splines;
    QMap<QString, QPair<QVector3D, QString>> m_decorations;
    
    void applyBrush(float wx, float wz, std::function<float(float, float, float)> func);
    float getFalloff(float dist, float radius);
};

class TrackTerrainTool : public QObject {
    Q_OBJECT
public:
    enum Tool { Raise, Lower, Smooth, Flatten, Noise, Pick, Ramp };

    explicit TrackTerrainTool(QObject* parent = nullptr);

    void setTool(Tool t) { m_currentTool = t; emit toolChanged(t); }
    Tool currentTool() const { return m_currentTool; }
    
    void setBrushRadius(float r) { m_brushRadius = r; }
    float brushRadius() const { return m_brushRadius; }
    
    void setBrushStrength(float s) { m_brushStrength = s; }
    float brushStrength() const { return m_brushStrength; }
    
    void applyAt(const QVector3D& worldPos);
    
    float getHeightAt(const QVector3D& pos) const;
    QVector3D getNormalAt(const QVector3D& pos) const;
    
    void beginStroke();
    void endStroke();

signals:
    void toolChanged(Tool tool);
    
private:
    Tool m_currentTool = Raise;
    float m_brushRadius = 10.0f;
    float m_brushStrength = 1.0f;
    QVector3D m_lastPos;
    bool m_strokeActive = false;
    
    TrackEditor* m_trackEditor = nullptr;
};

} // namespace ks