// tools_ACTextureTools.h
#pragma once

#include <QImage>
#include <QString>
#include <QPointF>
#include <QMap>
#include "ShaderMaterial.h"
#include <vector>

namespace ks {

// AC texture compression formats
enum class KsTextureFormat {
    DXT1,           // Diffuse, Emissive (no alpha)
    DXT5,           // Normal maps, Specular (with alpha)
    DXT5_RG,        // AC normal map format (RG only, B calculated)
    BC7,            // High quality compression
    Uncompressed
};

// Normal map conversion for AC (RG channels only)
struct KsNormalMapSettings {
    bool convertToRG = true;      // AC uses only RG, B is calculated
    bool swapGreen = false;       // Some tools export Y-flipped
    bool invertRed = false;       // Invert X direction
    bool invertGreen = true;      // AC requires Y inverted (OpenGL vs DirectX)
    float strength = 1.0f;        // Normal intensity
    bool generateMipmaps = true;
};

// Specular/Glossiness packing
struct KsSpecularSettings {
    enum SpecularSource {
        SpecularFromColor,
        SpecularFromAlpha,
        SpecularFromLuminance
    };
    
    SpecularSource specularSource = SpecularFromColor;
    float defaultSpecular = 0.5f;
    float defaultGlossiness = 0.5f;
    bool packGlossinessInAlpha = true;  // AC stores glossiness in alpha channel
};

// Car livery texture generation
struct CarLiverySettings {
    QColor baseColor = Qt::red;
    QColor stripeColor = Qt::white;
    int stripeCount = 2;
    float stripeWidth = 0.1f;
    QString logoPath;           // Sponsor logo overlay
    QPointF logoPosition;
    float logoScale = 0.2f;
    QString numberPath;         // Race number texture
    QString numberText;         // Or generate from text
    bool generateWireframe = false;
    int wireframeColor = 0x000000;
    int textureResolution = 2048;
};

class KsTextureTools {
public:
    // Normal map conversion for AC
    static QImage convertNormalMapForAC(const QImage& source, const KsNormalMapSettings& settings);
    static QImage generateNormalMapFromHeightmap(const QImage& heightmap, float strength = 1.0f);
    
    // Specular map packing
    static QImage packSpecularMap(const QImage& specularRGB, const QImage& glossiness);
    static QImage extractGlossinessFromSpecular(const QImage& specularMap);
    static QImage generateSpecularFromMaterial(float specularValue, float glossiness, int size = 64);
    
    // DDS compression for AC
    static QImage compressToDXT(const QImage& source, KsTextureFormat format, bool generateMipmaps = true);
    static QByteArray lastCompressedData();
    static bool saveAsDDS(const QImage& image, const QString& path, KsTextureFormat format);
    
    // Livery texture generation
    static QImage generateLiveryTexture(const CarLiverySettings& settings);
    static QImage addDecalToLivery(const QImage& livery, const QImage& decal, 
                                    const QPointF& uvPosition, float scale);
    
    // Texture optimization for AC
    static QImage optimizeForAC(const QImage& source, KsShaderType shaderType);
    static void validateTexturePaths(const KsMaterial& material, std::vector<QString>& missingTextures);
    
    // Texture channel operations (AC specific)
    static QImage swapChannelsForAC(const QImage& image, int rSrc, int gSrc, int bSrc, int aSrc);
    static QImage extractRGChannels(const QImage& image);  // For normal maps
    static QImage combineRGBA(const QImage& r, const QImage& g, const QImage& b, const QImage& a);
    
    // Mipmap generation with AC-specific filtering
    static std::vector<QImage> generateACMipmaps(const QImage& source, 
                                                   KsShaderType shaderType,
                                                   int maxLevels = 0);
    
    // Texture preview in AC environment
    static QImage previewTextureOnModel(const QImage& texture, const QImage& modelUVLayout);
};

// AC Texture Atlas for liveries
class KsLiveryAtlas {
public:
    struct LiveryPart {
        QString name;           // "body", "hood", "roof", "spoiler"
        QRect uvRect;           // UV coordinates in atlas
        QImage texture;         // Individual texture
    };
    
    bool addPart(const QString& name, const QImage& texture, const QRect& uvRect);
    bool removePart(const QString& name);
    QImage buildAtlas(int resolution = 4096);
    QImage extractPart(const QString& name);
    
    // Paint across UV seams
    void paintSeamless(const QPointF& uvStart, const QPointF& uvEnd, const QColor& color, float radius);
    
private:
    std::vector<LiveryPart> m_parts;
    QMap<QString, int> m_partIndex;
};

} // namespace ks