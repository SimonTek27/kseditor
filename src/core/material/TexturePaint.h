#pragma once

#include <QObject>
#include <QVector3D>
#include <QImage>
#include <QColor>
#include <QMap>
#include <QPainter>
#include <QVariant>

namespace ks {

class TexturePainter : public QObject {
    Q_OBJECT

public:
    static TexturePainter* instance();

    enum BrushType {
        BrushDraw,
        BrushSoft,
        BrushSmooth,
        BrushSmear,
        BrushClone,
        BrushBlur,
        BrushSharpen,
        BrushDodge,
        BrushBurn,
        BrushMask
    };
    Q_ENUM(BrushType)

    struct BrushSettings {
        BrushType type = BrushDraw;
        float size = 10.0f;
        float strength = 1.0f;
        float pressure = 1.0f;
        float spacing = 0.1f;
        float angle = 0.0f;
        bool usePressureSize = false;
        bool usePressureStrength = true;
        bool useAlpha = true;
        bool useColor = true;
        QColor color = Qt::white;
        int cloneSourceX = 0;
        int cloneSourceY = 0;
    };

    struct Stamp {
        QImage image;
        QVector3D position;
        float angle;
        float scale;
    };

    Q_INVOKABLE void setImage(const QImage& image);
    Q_INVOKABLE QImage getImage() const { return m_image; }
    Q_INVOKABLE void clearImage();

    Q_INVOKABLE void beginStroke(const QVector3D& pos);
    Q_INVOKABLE void addStamp(const QVector3D& pos);
    Q_INVOKABLE void endStroke();

    Q_INVOKABLE void setBrushType(BrushType type);
    Q_INVOKABLE int getBrushType() const { return m_brush.type; }

    Q_INVOKABLE void setBrushSize(float size);
    Q_INVOKABLE float getBrushSize() const { return m_brush.size; }

    Q_INVOKABLE void setBrushStrength(float strength);
    Q_INVOKABLE float getBrushStrength() const { return m_brush.strength; }

    Q_INVOKABLE void setBrushColor(const QColor& color);
    Q_INVOKABLE QColor getBrushColor() const { return m_brush.color; }

    Q_INVOKABLE void setBrushSpacing(float spacing);
    Q_INVOKABLE float getBrushSpacing() const { return m_brush.spacing; }

    Q_INVOKABLE void setBrushAngle(float angle);
    Q_INVOKABLE float getBrushAngle() const { return m_brush.angle; }

    Q_INVOKABLE void setCloneSource(int x, int y);
    Q_INVOKABLE QPoint getCloneSource() const;

    Q_INVOKABLE void setCloneImage(const QImage& image);

    Q_INVOKABLE QImage stampCircle(float size, const QColor& color);
    Q_INVOKABLE QImage stampSoft(float size, const QColor& color);
    Q_INVOKABLE QImage stampGaussian(float size, const QColor& color);

    Q_INVOKABLE QImage floodFill(int x, int y, int tolerance = 32);
    Q_INVOKABLE QImage colorize(int x, int y, const QColor& color, int tolerance);

    Q_INVOKABLE void resizeCanvas(int newWidth, int newHeight, int anchorX, int anchorY);
    Q_INVOKABLE void rotateCanvas(float angle);
    Q_INVOKABLE void flipCanvas(bool horizontal);

    Q_INVOKABLE void applyBlur(const QRect& region, float radius);
    Q_INVOKABLE void applySharpen(const QRect& region, float amount);
    Q_INVOKABLE void applyNoise(const QRect& region, float amount);

signals:
    void imageModified();
    void strokeEnded();
    void brushSettingsChanged();

private:
    TexturePainter(QObject* parent = nullptr);
    ~TexturePainter();
    Q_DISABLE_COPY(TexturePainter)

    static TexturePainter* s_instance;

    QImage m_image;
    QImage m_cloneImage;
    QImage m_strokeImage;
    QVector<Stamp> m_strokeStamps;
    BrushSettings m_brush;
    QPoint m_lastPos;
    QPoint m_cloneOffset;
    bool m_strokeActive;

    void applyBrushAt(const QPoint& pos);
    void drawStamp(const QImage& stamp, const QPoint& pos);
    float calculateAlpha(float distance, float maxDist);
    QColor mixColor(const QColor& a, const QColor& b, float t);
    void applyColorDodge(QRgb pixel, int delta);
    void applyColorBurn(QRgb pixel, int delta);
};

class TextureLayers : public QObject {
    Q_OBJECT

public:
    static TextureLayers* instance();

    struct Layer {
        QString id;
        QString name;
        QImage image;
        bool visible = true;
        bool locked = false;
        float opacity = 1.0f;
        int blendMode = 0;
        QPoint offset;
    };

    Q_INVOKABLE void addLayer(const QString& name);
    Q_INVOKABLE void removeLayer(const QString& id);
    Q_INVOKABLE QString duplicateLayer(const QString& id);

    Q_INVOKABLE Layer getLayer(const QString& id) const;
    Q_INVOKABLE QVector<Layer> getLayers() const;
    Q_INVOKABLE QStringList getLayerNames() const;

    Q_INVOKABLE void setLayerVisible(const QString& id, bool visible);
    Q_INVOKABLE void setLayerLocked(const QString& id, bool locked);
    Q_INVOKABLE void setLayerOpacity(const QString& id, float opacity);
    Q_INVOKABLE void setLayerBlendMode(const QString& id, int mode);

    Q_INVOKABLE void moveLayer(const QString& id, int newIndex);
    Q_INVOKABLE void mergeDown(const QString& id);
    Q_INVOKABLE QImage flatten() const;

    Q_INVOKABLE void clearLayer(const QString& id);
    Q_INVOKABLE void fillLayer(const QString& id, const QColor& color);

signals:
    void layerAdded(const QString& id);
    void layerRemoved(const QString& id);
    void layerModified(const QString& id);
    void layerOrderChanged();

private:
    TextureLayers(QObject* parent = nullptr);
    ~TextureLayers();
    Q_DISABLE_COPY(TextureLayers)

    static TextureLayers* s_instance;

    QVector<Layer> m_layers;
    int m_activeLayerIndex = 0;
};

class UVViewport : public QObject {
    Q_OBJECT

public:
    static UVViewport* instance();

    Q_INVOKABLE void setUVImage(const QImage& image);
    Q_INVOKABLE QImage getUVImage() const { return m_uvImage; }

    Q_INVOKABLE void setMeshUV(const QVariant& meshData);
    Q_INVOKABLE QVariant getMeshUV() const;

    Q_INVOKABLE void setVisibleArea(const QRect& rect);
    Q_INVOKABLE QRect getVisibleArea() const { return m_visibleArea; }

    Q_INVOKABLE void setGridVisible(bool visible);
    Q_INVOKABLE bool isGridVisible() const { return m_gridVisible; }

    Q_INVOKABLE void setGridSize(int size);
    Q_INVOKABLE int getGridSize() const { return m_gridSize; }

    Q_INVOKABLE void setAspectLocked(bool locked);
    Q_INVOKABLE bool isAspectLocked() const { return m_aspectLocked; }

    Q_INVOKABLE void unwrapAutomatic();
    Q_INVOKABLE void unwrapSmart(float angleThreshold = 45.0f);
    Q_INVOKABLE void packUVs(float margin = 0.01f);
    Q_INVOKABLE void stitchUVs(float threshold);

    Q_INVOKABLE float getUVScale() const { return m_uvScale; }
    Q_INVOKABLE void setUVScale(float scale);

    Q_INVOKABLE void setCursorPosition(int x, int y);
    Q_INVOKABLE QPoint getCursorPosition() const { return m_cursorPos; }

signals:
    void uvImageChanged();
    void meshUVChanged();
    void cursorMoved(const QPoint& pos);

private:
    UVViewport(QObject* parent = nullptr);
    ~UVViewport();
    Q_DISABLE_COPY(UVViewport)

    static UVViewport* s_instance;

    QImage m_uvImage;
    QVariant m_meshData;
    QRect m_visibleArea;
    QPoint m_cursorPos;
    bool m_gridVisible = true;
    int m_gridSize = 16;
    float m_uvScale = 1.0f;
    bool m_aspectLocked = true;
    float m_uvSeamAngle = 45.0f;
    float m_uvPackMargin = 0.01f;
    float m_uvStitchThreshold = 0.01f;
};

}