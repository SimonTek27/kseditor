// ks_KsShaderMaterial.h
#pragma once

#include <QString>
#include <QColor>
#include <QMap>
#include <QPointF>
#include <QImage>
#include <vector>

namespace ks {

// Kunos shader types for Assetto Corsa
enum class KsShaderType {
    ksPerPixel,           // Standard shader
    ksPerPixelAT,         // With alpha test
    ksPerPixelNM,         // With normal map
    ksPerPixelMultiMap,   // Multi-texture blending
    ksMultilayer,         // Multi-layer (metallic paint)
    ksCarPaint,           // Car paint with flakes
    ksWindscreen,         // Windscreen/glass
    ksLight,              // Lights (headlights, brakes)
    ksTree,               // Trees (alpha test)
    ksSkidmark,           // Skid marks
    ksBrakeDisc,          // Brake disc glow
    ksRim,                // Wheel rim (metal)
    ksTyre,               // Tire rubber
    ksInterior,           // Cockpit interior
    ksCarbon,             // Carbon fiber material
    ksChrome,             // Chrome/mirror
    ksEmissive            // Self-illuminated
};

// Fresnel parameters (critical for AC visuals)
struct KsFresnelParams {
    float c = 0.2f;           // Fresnel coefficient
    float maxLevel = 1.0f;    // Maximum reflection at grazing angle
    float minLevel = 0.0f;    // Minimum reflection at normal incidence
    float exponent = 5.0f;    // Fresnel curve exponent
    bool enabled = true;
};

// Specular parameters
struct KsSpecularParams {
    float exponent = 50.0f;   // Shininess (higher = sharper)
    float intensity = 1.0f;   // Specular strength
    QColor color = Qt::white;  // Specular tint
    float fresnelFactor = 0.5f; // Fresnel influence on specular
};

// Multilayer paint (metallic/pearlescent)
struct KsMultilayerParams {
    QColor baseColor;          // Base coat color
    QColor flakeColor;         // Metallic flake color
    float flakeSize = 0.05f;   // Flake scale
    float flakeDensity = 0.8f; // Flake density
    float flakeStrength = 0.5f;// Flake intensity
    QColor clearCoat;          // Clear coat tint
    float clearCoatStrength = 1.0f;
    float noiseScale = 2.0f;   // Perlin noise for flake variation
};

// Complete KS material
struct KsMaterial {
    QString name;
    KsShaderType shaderType = KsShaderType::ksPerPixel;
    
    // Color properties
    QColor diffuseColor = Qt::white;
    QColor specularColor = Qt::white;
    QColor emissiveColor = Qt::black;
    
    // Weights
    float diffuseW = 1.0f;
    float specularW = 1.0f;
    float emissiveW = 1.0f;
    float alpha = 1.0f;
    
    // Alpha test (for trees, fences, grilles)
    float alphaTestRef = 0.5f;
    bool alphaTestEnabled = false;
    
    // Lighting
    float ambient = 0.5f;
    float diffuse = 1.0f;
    
    // KS-specific features
    KsFresnelParams fresnel;
    KsSpecularParams specular;
    KsMultilayerParams multilayer;
    
    // Texture slots (AC specific)
    QString txDiffuse;      // Albedo/Diffuse (DXT1)
    QString txNormal;       // Normal map - AC uses RG channels only! (DXT5)
    QString txSpecular;     // Specular RGB + Glossiness in Alpha (DXT5)
    QString txEmissive;     // Emissive map (DXT1)
    QString txDetail;       // Detail map for close-up (DXT1)
    QString txMask;         // Mask for multi-texture blending
    QString txAlpha;        // Separate alpha channel (DXT5)
    
    // Texture tiling
    float uvScale = 1.0f;
    float uvRotation = 0.0f;
    QPointF uvOffset = QPointF(0, 0);
    
    // Detail map tiling (usually larger scale)
    float detailUVScale = 4.0f;
    
    // Rendering
    bool doubleSided = false;
    bool castShadows = true;
    bool receiveShadows = true;
    int renderQueue = 2000;  // 2000=Geometry, 3000=Transparent
    
    // Custom properties (for extended shaders)
    QMap<QString, float> floatParams;
    QMap<QString, QColor> colorParams;
    
    // Serialization to/from KN5
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
};

// KS Material Library
class KsMaterialLibrary {
public:
    static KsMaterialLibrary& instance();
    
    // Load/Save
    bool loadFromKN5(const QString& path);
    bool saveToKN5(const QString& path);
    
    // Material management
    void addMaterial(const KsMaterial& material);
    void removeMaterial(const QString& name);
    KsMaterial* getMaterial(const QString& name);
    std::vector<KsMaterial> getAllMaterials() const;
    
    // AC standard materials presets
    static KsMaterial createCarPaintMaterial(const QColor& color, float metallic = 0.8f);
    static KsMaterial createGlassMaterial(float tint = 0.2f);
    static KsMaterial createTyreMaterial();
    static KsMaterial createCarbonMaterial();
    static KsMaterial createChromeMaterial();
    static KsMaterial createLightMaterial(const QColor& color);
    
private:
    std::vector<KsMaterial> m_materials;
    QMap<QString, int> m_nameToIndex;
};

// Material validator (checks for AC compatibility)
class KsMaterialValidator {
public:
    struct ValidationResult {
        bool isValid = true;
        std::vector<QString> errors;
        std::vector<QString> warnings;
    };
    
    ValidationResult validate(const KsMaterial& material);
    
    // Fix common issues
    void fixNormalMapFormat(QImage& normalMap);  // Convert to RG (AC format)
    void fixSpecularMapFormat(QImage& specularMap); // Ensure alpha has glossiness
    void ensurePowerOfTwoTextures(KsMaterial& material);
};

} // namespace ks