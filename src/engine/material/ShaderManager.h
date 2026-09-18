#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>

/**
 * @brief Shader Manager for Assetto Corsa
 *
 * Manages shaders used by AC for rendering.
 * Based on:
 * - KN5 file format documentation (Hagn's Site)
 * - ac-custom-shaders-patch/sdk-shaders
 * - AC shader system documentation
 *
 * AC Shaders:
 * - ksBasic - Basic shader
 * - ksPerfCarPaint - Car paint with performance
 * - ksTyres - Tire shader
 * - ksBrakeDisc - Brake disc shader
 * - ksGlass - Glass shader
 * - ksLeather - Leather shader
 * - ksCarbon - Carbon fiber shader
 * - ksTrees - Tree shader
 * - ksTerrain - Terrain shader
 * - ksWall - Wall shader
 */
class ShaderManager {
public:
    struct ShaderProperty {
        QString name;
        QString type;           // "float", "vec2", "vec3", "vec4", "texture"
        float defaultValue = 0.0f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        QString description;
    };

    struct ShaderInfo {
        QString name;
        QString category;       // "car", "track", "effect"
        QString description;
        QVector<ShaderProperty> properties;
        bool isBuiltIn = true;
        QString vertexShader;
        QString fragmentShader;
    };

    struct ShaderPreset {
        QString name;
        QString shaderName;
        QMap<QString, float> floatProperties;
        QMap<QString, QVector<float>> vectorProperties;
        QMap<QString, QString> textureProperties;
    };

    // Shader registry
    static void initializeShaders();
    static QVector<ShaderInfo> getAllShaders();
    static QVector<ShaderInfo> getShadersByCategory(const QString& category);
    static ShaderInfo getShader(const QString& name);
    static bool hasShader(const QString& name);

    // Property access
    static QVector<ShaderProperty> getProperties(const QString& shaderName);
    static ShaderProperty getProperty(const QString& shaderName, const QString& propName);

    // Presets
    static QVector<ShaderPreset> getPresets(const QString& shaderName);
    static ShaderPreset getPreset(const QString& shaderName, const QString& presetName);
    static bool savePreset(const ShaderPreset& preset);

    // Shader validation
    static bool validateShaderConfig(const QString& shaderName,
                                      const QMap<QString, float>& properties,
                                      QString* error = nullptr);

    // Built-in shaders
    static ShaderInfo getBasicShader();
    static ShaderInfo getCarPaintShader();
    static ShaderInfo getTyresShader();
    static ShaderInfo getBrakeDiscShader();
    static ShaderInfo getGlassShader();
    static ShaderInfo getLeatherShader();
    static ShaderInfo getCarbonShader();
    static ShaderInfo getTreesShader();
    static ShaderInfo getTerrainShader();
    static ShaderInfo getWallShader();

    // Utility
    static QStringList getShaderCategories();
    static QString getShaderTypeName(const QString& type);

private:
    static QMap<QString, ShaderInfo> s_shaders;
    static bool s_initialized;
};

/**
 * @brief Shader Editor Widget - UI for editing shader properties
 */
class ShaderEditorWidget;
