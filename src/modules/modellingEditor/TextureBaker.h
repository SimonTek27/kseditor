#pragma once

#include <QString>
#include <QMap>
#include <QColor>
#include <QImage>

namespace ks { namespace geometry { class Mesh3D; } }

namespace ks {
namespace io {

// CPU texture baker: rasterizes mesh properties (normals, AO, base color, PBR
// channels) into an image in UV space. Output maps can be saved to disk via
// addBakeTarget + bake, or fetched in memory with getBakedTexture.
class TextureBaker
{
public:
    TextureBaker() = default;
    ~TextureBaker() = default;

    enum BakeType { Diffuse, Normal, Roughness, Metallic, AO, Height, Emission };
    enum PackChannel { RoughnessCh = 1 << 0, MetallicCh = 1 << 1, AOCh = 1 << 2, HeightCh = 1 << 3, EmissionCh = 1 << 4 };

    void setSourceMesh(geometry::Mesh3D* mesh) { m_source = mesh; }
    void setTargetResolution(int width, int height) { m_width = width; m_height = height; }
    void setBaseColor(const QColor& color) { m_baseColor = color; }

    void addBakeTarget(BakeType type, const QString& outputPath);
    static QString textureTypeName(BakeType type);

    void bake(BakeType type);
    QImage getBakedTexture(BakeType type) const { return m_bakedTextures.value(type); }

    QImage packRgba(int packChannels) const;

private:
    geometry::Mesh3D* m_source = nullptr;
    int m_width = 2048;
    int m_height = 2048;
    QColor m_baseColor = Qt::white;
    QMap<BakeType, QString> m_targets;
    QMap<BakeType, QImage> m_bakedTextures;
};

} // namespace io
} // namespace ks
