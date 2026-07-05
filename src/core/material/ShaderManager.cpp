#include "ShaderManager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// ============================================================================
// Static member initialization
// ============================================================================

QMap<QString, ShaderManager::ShaderInfo> ShaderManager::s_shaders;
bool ShaderManager::s_initialized = false;

// ============================================================================
// Shader registry
// ============================================================================

void ShaderManager::initializeShaders() {
    if (s_initialized) return;

    s_shaders["ksBasic"] = getBasicShader();
    s_shaders["ksPerfCarPaint"] = getCarPaintShader();
    s_shaders["ksTyres"] = getTyresShader();
    s_shaders["ksBrakeDisc"] = getBrakeDiscShader();
    s_shaders["ksGlass"] = getGlassShader();
    s_shaders["ksLeather"] = getLeatherShader();
    s_shaders["ksCarbon"] = getCarbonShader();
    s_shaders["ksTrees"] = getTreesShader();
    s_shaders["ksTerrain"] = getTerrainShader();
    s_shaders["ksWall"] = getWallShader();

    s_initialized = true;
}

QVector<ShaderManager::ShaderInfo> ShaderManager::getAllShaders() {
    initializeShaders();
    QVector<ShaderInfo> shaders;
    for (auto it = s_shaders.begin(); it != s_shaders.end(); ++it) {
        shaders.append(it.value());
    }
    return shaders;
}

QVector<ShaderManager::ShaderInfo> ShaderManager::getShadersByCategory(const QString& category) {
    initializeShaders();
    QVector<ShaderInfo> shaders;
    for (auto it = s_shaders.begin(); it != s_shaders.end(); ++it) {
        if (it.value().category == category) {
            shaders.append(it.value());
        }
    }
    return shaders;
}

ShaderManager::ShaderInfo ShaderManager::getShader(const QString& name) {
    initializeShaders();
    return s_shaders.value(name);
}

bool ShaderManager::hasShader(const QString& name) {
    initializeShaders();
    return s_shaders.contains(name);
}

// ============================================================================
// Property access
// ============================================================================

QVector<ShaderManager::ShaderProperty> ShaderManager::getProperties(const QString& shaderName) {
    return getShader(shaderName).properties;
}

ShaderManager::ShaderProperty ShaderManager::getProperty(const QString& shaderName, const QString& propName) {
    QVector<ShaderProperty> props = getProperties(shaderName);
    for (const ShaderProperty& prop : props) {
        if (prop.name == propName) {
            return prop;
        }
    }
    return ShaderProperty();
}

// ============================================================================
// Presets
// ============================================================================

QVector<ShaderManager::ShaderPreset> ShaderManager::getPresets(const QString& shaderName) {
    QVector<ShaderPreset> presets;

    if (shaderName == "ksPerfCarPaint") {
        ShaderPreset glossy;
        glossy.name = "Glossy";
        glossy.shaderName = shaderName;
        glossy.floatProperties["specular"] = 0.8f;
        glossy.floatProperties["smoothness"] = 0.9f;
        glossy.floatProperties["clearCoat"] = 1.0f;
        presets.append(glossy);

        ShaderPreset matte;
        matte.name = "Matte";
        matte.shaderName = shaderName;
        matte.floatProperties["specular"] = 0.3f;
        matte.floatProperties["smoothness"] = 0.4f;
        matte.floatProperties["clearCoat"] = 0.0f;
        presets.append(matte);

        ShaderPreset metallic;
        metallic.name = "Metallic";
        metallic.shaderName = shaderName;
        metallic.floatProperties["specular"] = 0.9f;
        metallic.floatProperties["smoothness"] = 0.8f;
        metallic.floatProperties["metallic"] = 0.9f;
        presets.append(metallic);
    } else if (shaderName == "ksGlass") {
        ShaderPreset clear;
        clear.name = "Clear";
        clear.shaderName = shaderName;
        clear.floatProperties["opacity"] = 0.3f;
        clear.floatProperties["refraction"] = 1.5f;
        presets.append(clear);

        ShaderPreset tinted;
        tinted.name = "Tinted";
        tinted.shaderName = shaderName;
        tinted.floatProperties["opacity"] = 0.5f;
        tinted.vectorProperties["tint"] = {0.1f, 0.1f, 0.2f, 1.0f};
        presets.append(tinted);
    } else if (shaderName == "ksTyres") {
        ShaderPreset street;
        street.name = "Street";
        street.shaderName = shaderName;
        street.floatProperties["slip"] = 0.5f;
        street.floatProperties["wear"] = 0.0f;
        presets.append(street);

        ShaderPreset worn;
        worn.name = "Worn";
        worn.shaderName = shaderName;
        worn.floatProperties["slip"] = 0.8f;
        worn.floatProperties["wear"] = 0.5f;
        presets.append(worn);
    }

    return presets;
}

ShaderManager::ShaderPreset ShaderManager::getPreset(const QString& shaderName, const QString& presetName) {
    QVector<ShaderPreset> presets = getPresets(shaderName);
    for (const ShaderPreset& preset : presets) {
        if (preset.name == presetName) {
            return preset;
        }
    }
    return ShaderPreset();
}

bool ShaderManager::savePreset(const ShaderPreset& preset) {
    QString presetsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/shader_presets";
    QDir().mkpath(presetsDir);

    QString filePath = presetsDir + "/" + preset.name + ".json";
    QJsonObject obj;
    obj["name"] = preset.name;
    obj["shaderName"] = preset.shaderName;

    QJsonObject floatProps;
    for (auto it = preset.floatProperties.begin(); it != preset.floatProperties.end(); ++it) {
        floatProps[it.key()] = it.value();
    }
    obj["floatProperties"] = floatProps;

    QJsonObject vecProps;
    for (auto it = preset.vectorProperties.begin(); it != preset.vectorProperties.end(); ++it) {
        QJsonArray arr;
        for (float v : it.value()) arr.append(v);
        vecProps[it.key()] = arr;
    }
    obj["vectorProperties"] = vecProps;

    QJsonObject texProps;
    for (auto it = preset.textureProperties.begin(); it != preset.textureProperties.end(); ++it) {
        texProps[it.key()] = it.value();
    }
    obj["textureProperties"] = texProps;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(obj).toJson());
    file.close();
    return true;
}

// ============================================================================
// Shader validation
// ============================================================================

bool ShaderManager::validateShaderConfig(const QString& shaderName,
                                          const QMap<QString, float>& properties,
                                          QString* error) {
    if (!hasShader(shaderName)) {
        if (error) *error = "Unknown shader: " + shaderName;
        return false;
    }

    QVector<ShaderProperty> validProps = getProperties(shaderName);

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        bool found = false;
        for (const ShaderProperty& prop : validProps) {
            if (prop.name == it.key()) {
                if (it.value() < prop.minValue || it.value() > prop.maxValue) {
                    if (error) *error = "Property " + it.key() + " out of range";
                    return false;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            if (error) *error = "Unknown property: " + it.key();
            return false;
        }
    }

    return true;
}

// ============================================================================
// Built-in shaders
// ============================================================================

ShaderManager::ShaderInfo ShaderManager::getBasicShader() {
    ShaderInfo info;
    info.name = "ksBasic";
    info.category = "car";
    info.description = "Basic shader for simple materials";
    info.isBuiltIn = true;
    return info;
}

ShaderManager::ShaderInfo ShaderManager::getCarPaintShader() {
    ShaderInfo info;
    info.name = "ksPerfCarPaint";
    info.category = "car";
    info.description = "Performance car paint with clear coat";
    info.isBuiltIn = true;

    info.properties.append({"specular", "float", 0.8f, 0.0f, 1.0f, "Specular intensity"});
    info.properties.append({"smoothness", "float", 0.9f, 0.0f, 1.0f, "Surface smoothness"});
    info.properties.append({"clearCoat", "float", 1.0f, 0.0f, 1.0f, "Clear coat layer"});
    info.properties.append({"metallic", "float", 0.0f, 0.0f, 1.0f, "Metallic appearance"});
    info.properties.append({"baseColor", "vec3", 1.0f, 0.0f, 1.0f, "Base color tint"});
    info.properties.append({"decalMap", "texture", 0, 0, 0, "Decal texture"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getTyresShader() {
    ShaderInfo info;
    info.name = "ksTyres";
    info.category = "car";
    info.description = "Tire shader with slip and wear";
    info.isBuiltIn = true;

    info.properties.append({"slip", "float", 0.5f, 0.0f, 1.0f, "Tire slip amount"});
    info.properties.append({"wear", "float", 0.0f, 0.0f, 1.0f, "Tire wear level"});
    info.properties.append({"dirtyLevel", "float", 0.0f, 0.0f, 1.0f, "Dirt level"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});
    info.properties.append({"txGlow", "texture", 0, 0, 0, "Glow map"});
    info.properties.append({"txDirty", "texture", 0, 0, 0, "Dirt overlay"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getBrakeDiscShader() {
    ShaderInfo info;
    info.name = "ksBrakeDisc";
    info.category = "car";
    info.description = "Brake disc shader with temperature glow";
    info.isBuiltIn = true;

    info.properties.append({"glow", "float", 0.0f, 0.0f, 1.0f, "Brake glow intensity"});
    info.properties.append({"temperature", "float", 0.0f, 0.0f, 1.0f, "Temperature level"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txGlow", "texture", 0, 0, 0, "Glow texture"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getGlassShader() {
    ShaderInfo info;
    info.name = "ksGlass";
    info.category = "car";
    info.description = "Glass shader with refraction and reflection";
    info.isBuiltIn = true;

    info.properties.append({"opacity", "float", 0.3f, 0.0f, 1.0f, "Glass opacity"});
    info.properties.append({"refraction", "float", 1.5f, 1.0f, 2.0f, "Refraction index"});
    info.properties.append({"reflection", "float", 0.8f, 0.0f, 1.0f, "Reflection intensity"});
    info.properties.append({"tint", "vec4", 0.0f, 0.0f, 1.0f, "Glass tint color"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getLeatherShader() {
    ShaderInfo info;
    info.name = "ksLeather";
    info.category = "car";
    info.description = "Leather material shader";
    info.isBuiltIn = true;

    info.properties.append({"roughness", "float", 0.7f, 0.0f, 1.0f, "Surface roughness"});
    info.properties.append({"bumpStrength", "float", 0.5f, 0.0f, 1.0f, "Bump map strength"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getCarbonShader() {
    ShaderInfo info;
    info.name = "ksCarbon";
    info.category = "car";
    info.description = "Carbon fiber material shader";
    info.isBuiltIn = true;

    info.properties.append({"roughness", "float", 0.3f, 0.0f, 1.0f, "Surface roughness"});
    info.properties.append({"metallic", "float", 0.8f, 0.0f, 1.0f, "Metallic appearance"});
    info.properties.append({"weaveScale", "float", 1.0f, 0.1f, 10.0f, "Weave pattern scale"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getTreesShader() {
    ShaderInfo info;
    info.name = "ksTrees";
    info.category = "track";
    info.description = "Tree and vegetation shader";
    info.isBuiltIn = true;

    info.properties.append({"windStrength", "float", 0.5f, 0.0f, 1.0f, "Wind animation strength"});
    info.properties.append({"leafScale", "float", 1.0f, 0.1f, 2.0f, "Leaf size scale"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getTerrainShader() {
    ShaderInfo info;
    info.name = "ksTerrain";
    info.category = "track";
    info.description = "Terrain surface shader";
    info.isBuiltIn = true;

    info.properties.append({"roughness", "float", 0.8f, 0.0f, 1.0f, "Surface roughness"});
    info.properties.append({"grip", "float", 1.0f, 0.0f, 2.0f, "Grip level"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});
    info.properties.append({"txDetail", "texture", 0, 0, 0, "Detail texture"});

    return info;
}

ShaderManager::ShaderInfo ShaderManager::getWallShader() {
    ShaderInfo info;
    info.name = "ksWall";
    info.category = "track";
    info.description = "Wall and barrier shader";
    info.isBuiltIn = true;

    info.properties.append({"roughness", "float", 0.7f, 0.0f, 1.0f, "Surface roughness"});
    info.properties.append({"damage", "float", 0.0f, 0.0f, 1.0f, "Damage level"});
    info.properties.append({"txDiffuse", "texture", 0, 0, 0, "Diffuse texture"});
    info.properties.append({"txNormal", "texture", 0, 0, 0, "Normal map"});

    return info;
}

// ============================================================================
// Utility
// ============================================================================

QStringList ShaderManager::getShaderCategories() {
    return QStringList() << "car" << "track" << "effect";
}

QString ShaderManager::getShaderTypeName(const QString& type) {
    if (type == "float") return "Float";
    if (type == "vec2") return "Vector 2";
    if (type == "vec3") return "Vector 3";
    if (type == "vec4") return "Vector 4";
    if (type == "texture") return "Texture";
    return type;
}
