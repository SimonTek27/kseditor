#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QImage>
#include <QSize>
#include <QRect>

namespace ks {

struct AtlasTexture {
    QString name;
    QImage image;
    QRect region;
};

class TextureAtlas : public QObject {
    Q_OBJECT

public:
    static TextureAtlas* instance();

    void addTexture(const QString& name, const QString& imagePath);
    void clearTextures();
    bool generateAtlas(int atlasSize = 2048, int padding = 4);
    QImage getAtlasImage() const { return m_atlasImage; }
    
    bool saveAtlas(const QString& outputPath);
    bool saveUVMap(const QString& outputPath);
    
    int textureCount() const { return m_textures.size(); }
    QList<AtlasTexture> getTextures() const { return m_textures; }

signals:
    void generationProgress(int current, int total);
    void generationComplete(bool success);

private:
    explicit TextureAtlas(QObject* parent = nullptr);
    static TextureAtlas* s_instance;
    
    QList<AtlasTexture> m_textures;
    QImage m_atlasImage;
    QSize m_atlasSize;
    
    bool packTextures(int atlasSize, int padding);
    QList<QRect> calculatePacking(int width, int height);
};

}