#include "MaterialLibrary.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QUuid>
#include "core/editor/EditorConfig.h"

// ============================================================================
// Static member initialization
// ============================================================================

QMap<QString, MaterialLibrary::MaterialPreset> MaterialLibrary::m_materials;
QStringList MaterialLibrary::m_categories;
QString MaterialLibrary::m_libraryPath;
QString MaterialLibrary::m_lastError;

// ============================================================================
// Library management
// ============================================================================

bool MaterialLibrary::saveLibrary(const QString& libraryPath) {
    QString path = libraryPath;
    if (path.isEmpty()) path = m_libraryPath;
    if (path.isEmpty()) path = getDefaultLibraryPath();

    QJsonObject root;
    root["version"] = "1.0";
    root["categories"] = QJsonArray::fromStringList(m_categories);

    QJsonArray materialsArray;
    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        materialsArray.append(materialToJson(it.value()));
    }
    root["materials"] = materialsArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot save library to: " + path;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();

    m_libraryPath = path;
    return true;
}

bool MaterialLibrary::loadLibrary(const QString& libraryPath) {
    QFile file(libraryPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot load library from: " + libraryPath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid library format";
        return false;
    }

    QJsonObject root = doc.object();
    m_categories = root["categories"].toVariant().toStringList();

    m_materials.clear();
    QJsonArray materialsArray = root["materials"].toArray();
    for (const QJsonValue& val : materialsArray) {
        MaterialPreset material;
        if (parseMaterialJson(val.toObject(), material)) {
            m_materials[material.name] = material;
        }
    }

    m_libraryPath = libraryPath;
    return true;
}

bool MaterialLibrary::createDefaultLibrary() {
    QString path = getDefaultLibraryPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    // Create empty library
    m_materials.clear();
    m_categories.clear();
    m_categories << "car" << "track" << "interior" << "glass" << "wheel" << "driver" << "other";

    // Load built-in materials
    loadBuiltInMaterials();

    return saveLibrary(path);
}

// ============================================================================
// Material operations
// ============================================================================

bool MaterialLibrary::saveMaterial(const MaterialPreset& material) {
    m_materials[material.name] = material;
    return saveLibrary();
}

bool MaterialLibrary::saveMaterialAs(const MaterialPreset& material, const QString& name) {
    MaterialPreset newMaterial = material;
    newMaterial.name = name;
    newMaterial.createdDate = QDateTime::currentDateTime();
    newMaterial.modifiedDate = newMaterial.createdDate;
    newMaterial.isBuiltIn = false;

    return saveMaterial(newMaterial);
}

bool MaterialLibrary::deleteMaterial(const QString& name) {
    if (!m_materials.contains(name)) {
        m_lastError = "Material not found: " + name;
        return false;
    }

    if (m_materials[name].isBuiltIn) {
        m_lastError = "Cannot delete built-in material: " + name;
        return false;
    }

    m_materials.remove(name);
    return saveLibrary();
}

bool MaterialLibrary::renameMaterial(const QString& oldName, const QString& newName) {
    if (!m_materials.contains(oldName)) {
        m_lastError = "Material not found: " + oldName;
        return false;
    }

    if (m_materials.contains(newName)) {
        m_lastError = "Material already exists: " + newName;
        return false;
    }

    MaterialPreset material = m_materials[oldName];
    m_materials.remove(oldName);
    material.name = newName;
    material.modifiedDate = QDateTime::currentDateTime();

    return saveMaterial(material);
}

// ============================================================================
// Material access
// ============================================================================

MaterialLibrary::MaterialPreset MaterialLibrary::getMaterial(const QString& name) {
    return m_materials.value(name);
}

QVector<MaterialLibrary::MaterialPreset> MaterialLibrary::getAllMaterials() {
    QVector<MaterialPreset> result;
    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        result.append(it.value());
    }
    return result;
}

QVector<MaterialLibrary::MaterialPreset> MaterialLibrary::getMaterialsByCategory(const QString& category) {
    QVector<MaterialPreset> result;
    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        if (it.value().category == category) {
            result.append(it.value());
        }
    }
    return result;
}

QVector<MaterialLibrary::MaterialPreset> MaterialLibrary::searchMaterials(const QString& query) {
    QVector<MaterialPreset> result;
    QString lowerQuery = query.toLower();

    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        const MaterialPreset& mat = it.value();
        if (mat.name.toLower().contains(lowerQuery) ||
            mat.shaderName.toLower().contains(lowerQuery) ||
            mat.description.toLower().contains(lowerQuery)) {
            result.append(mat);
        }
    }
    return result;
}

bool MaterialLibrary::hasMaterial(const QString& name) {
    return m_materials.contains(name);
}

// ============================================================================
// Categories
// ============================================================================

QStringList MaterialLibrary::getCategories() {
    return m_categories;
}

void MaterialLibrary::setCategories(const QStringList& categories) {
    m_categories = categories;
}

// ============================================================================
// Import/Export
// ============================================================================

bool MaterialLibrary::importMaterial(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open material file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid material file format";
        return false;
    }

    MaterialPreset material;
    if (!parseMaterialJson(doc.object(), material)) {
        m_lastError = "Failed to parse material file";
        return false;
    }

    // Generate unique name if needed
    if (m_materials.contains(material.name)) {
        material.name = material.name + "_" + QUuid::createUuid().toString().left(8);
    }

    material.isBuiltIn = false;
    material.createdDate = QDateTime::currentDateTime();
    material.modifiedDate = material.createdDate;

    return saveMaterial(material);
}

bool MaterialLibrary::exportMaterial(const QString& name, const QString& filePath) {
    if (!m_materials.contains(name)) {
        m_lastError = "Material not found: " + name;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create export file: " + filePath;
        return false;
    }

    QJsonObject json = materialToJson(m_materials[name]);
    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();

    return true;
}

bool MaterialLibrary::exportAllMaterials(const QString& directory) {
    QDir().mkpath(directory);

    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        QString filePath = directory + "/" + it.value().name + ".json";
        if (!exportMaterial(it.key(), filePath)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Built-in materials
// ============================================================================

void MaterialLibrary::loadBuiltInMaterials() {
    // Car body materials
    MaterialPreset carPaint;
    carPaint.name = "Car Paint (Glossy)";
    carPaint.shaderName = "ksPerfCarPaint";
    carPaint.category = "car";
    carPaint.isBuiltIn = true;
    carPaint.description = "Standard glossy car paint material";
    carPaint.floatProperties["specular"] = 0.8f;
    carPaint.floatProperties["smoothness"] = 0.9f;
    carPaint.floatProperties["clearCoat"] = 1.0f;
    m_materials[carPaint.name] = carPaint;

    MaterialPreset carMatte;
    carMatte.name = "Car Paint (Matte)";
    carMatte.shaderName = "ksPerfCarPaint";
    carMatte.category = "car";
    carMatte.isBuiltIn = true;
    carMatte.description = "Matte car paint material";
    carMatte.floatProperties["specular"] = 0.3f;
    carMatte.floatProperties["smoothness"] = 0.4f;
    carMatte.floatProperties["clearCoat"] = 0.0f;
    m_materials[carMatte.name] = carMatte;

    // Glass materials
    MaterialPreset glass;
    glass.name = "Glass (Clear)";
    glass.shaderName = "ksGlass";
    glass.category = "glass";
    glass.isBuiltIn = true;
    glass.description = "Standard clear glass";
    glass.floatProperties["opacity"] = 0.3f;
    glass.floatProperties["refraction"] = 1.5f;
    glass.floatProperties["reflection"] = 0.8f;
    m_materials[glass.name] = glass;

    MaterialPreset tintedGlass;
    tintedGlass.name = "Glass (Tinted)";
    tintedGlass.shaderName = "ksGlass";
    tintedGlass.category = "glass";
    tintedGlass.isBuiltIn = true;
    tintedGlass.description = "Tinted glass material";
    tintedGlass.floatProperties["opacity"] = 0.5f;
    tintedGlass.floatProperties["refraction"] = 1.5f;
    tintedGlass.floatProperties["reflection"] = 0.6f;
    tintedGlass.colorProperties["tint"] = {0.1f, 0.1f, 0.2f, 1.0f};
    m_materials[tintedGlass.name] = tintedGlass;

    // Interior materials
    MaterialPreset leather;
    leather.name = "Leather (Black)";
    leather.shaderName = "ksLeather";
    leather.category = "interior";
    leather.isBuiltIn = true;
    leather.description = "Standard black leather";
    leather.floatProperties["roughness"] = 0.7f;
    leather.floatProperties["bumpStrength"] = 0.5f;
    m_materials[leather.name] = leather;

    MaterialPreset carbonFiber;
    carbonFiber.name = "Carbon Fiber";
    carbonFiber.shaderName = "ksCarbon";
    carbonFiber.category = "interior";
    carbonFiber.isBuiltIn = true;
    carbonFiber.description = "Carbon fiber material";
    carbonFiber.floatProperties["roughness"] = 0.3f;
    carbonFiber.floatProperties["metallic"] = 0.8f;
    m_materials[carbonFiber.name] = carbonFiber;

    // Wheel materials
    MaterialPreset alloyWheel;
    alloyWheel.name = "Alloy Wheel";
    alloyWheel.shaderName = "ksMetal";
    alloyWheel.category = "wheel";
    alloyWheel.isBuiltIn = true;
    alloyWheel.description = "Standard alloy wheel material";
    alloyWheel.floatProperties["metallic"] = 0.9f;
    alloyWheel.floatProperties["roughness"] = 0.2f;
    m_materials[alloyWheel.name] = alloyWheel;

    // Track materials
    MaterialPreset asphalt;
    asphalt.name = "Asphalt";
    asphalt.shaderName = "ksTerrain";
    asphalt.category = "track";
    asphalt.isBuiltIn = true;
    asphalt.description = "Standard asphalt surface";
    asphalt.floatProperties["roughness"] = 0.8f;
    asphalt.floatProperties["grip"] = 1.0f;
    m_materials[asphalt.name] = asphalt;

    MaterialPreset grass;
    grass.name = "Grass";
    grass.shaderName = "ksTerrain";
    grass.category = "track";
    grass.isBuiltIn = true;
    grass.description = "Grass terrain";
    grass.floatProperties["roughness"] = 0.9f;
    grass.floatProperties["grip"] = 0.6f;
    m_materials[grass.name] = grass;
}

QVector<MaterialLibrary::MaterialPreset> MaterialLibrary::getBuiltInMaterials() {
    QVector<MaterialPreset> result;
    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        if (it.value().isBuiltIn) {
            result.append(it.value());
        }
    }
    return result;
}

// ============================================================================
// Utility
// ============================================================================

QString MaterialLibrary::getLibraryPath() {
    return m_libraryPath;
}

QString MaterialLibrary::getDefaultLibraryPath() {
    return EditorConfig::instance().materialLibraryPath();
}

// ============================================================================
// Private helpers
// ============================================================================

bool MaterialLibrary::parseMaterialJson(const QJsonObject& json, MaterialPreset& material) {
    material.name = json["name"].toString();
    material.shaderName = json["shader"].toString();
    material.category = json["category"].toString();
    material.author = json["author"].toString();
    material.description = json["description"].toString();
    material.isBuiltIn = json["builtIn"].toBool(false);

    QString createdStr = json["created"].toString();
    if (!createdStr.isEmpty()) {
        material.createdDate = QDateTime::fromString(createdStr, Qt::ISODate);
    }

    QString modifiedStr = json["modified"].toString();
    if (!modifiedStr.isEmpty()) {
        material.modifiedDate = QDateTime::fromString(modifiedStr, Qt::ISODate);
    }

    // Parse float properties
    QJsonObject floats = json["floats"].toObject();
    for (auto it = floats.begin(); it != floats.end(); ++it) {
        material.floatProperties[it.key()] = it.value().toDouble();
    }

    // Parse vector properties
    QJsonObject vectors = json["vectors"].toObject();
    for (auto it = vectors.begin(); it != vectors.end(); ++it) {
        QJsonArray arr = it.value().toArray();
        QVector<float> vec;
        for (const QJsonValue& v : arr) {
            vec.append(v.toDouble());
        }
        material.vectorProperties[it.key()] = vec;
    }

    // Parse color properties
    QJsonObject colors = json["colors"].toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        QJsonArray arr = it.value().toArray();
        QVector<float> col;
        for (const QJsonValue& v : arr) {
            col.append(v.toDouble());
        }
        material.colorProperties[it.key()] = col;
    }

    // Parse texture paths
    QJsonObject textures = json["textures"].toObject();
    for (auto it = textures.begin(); it != textures.end(); ++it) {
        material.texturePaths[it.key()] = it.value().toString();
    }

    return !material.name.isEmpty();
}

QJsonObject MaterialLibrary::materialToJson(const MaterialPreset& material) {
    QJsonObject json;
    json["name"] = material.name;
    json["shader"] = material.shaderName;
    json["category"] = material.category;
    json["author"] = material.author;
    json["description"] = material.description;
    json["builtIn"] = material.isBuiltIn;

    if (material.createdDate.isValid()) {
        json["created"] = material.createdDate.toString(Qt::ISODate);
    }
    if (material.modifiedDate.isValid()) {
        json["modified"] = material.modifiedDate.toString(Qt::ISODate);
    }

    // Float properties
    QJsonObject floats;
    for (auto it = material.floatProperties.begin(); it != material.floatProperties.end(); ++it) {
        floats[it.key()] = it.value();
    }
    json["floats"] = floats;

    // Vector properties
    QJsonObject vectors;
    for (auto it = material.vectorProperties.begin(); it != material.vectorProperties.end(); ++it) {
        QJsonArray arr;
        for (float v : it.value()) {
            arr.append(v);
        }
        vectors[it.key()] = arr;
    }
    json["vectors"] = vectors;

    // Color properties
    QJsonObject colors;
    for (auto it = material.colorProperties.begin(); it != material.colorProperties.end(); ++it) {
        QJsonArray arr;
        for (float v : it.value()) {
            arr.append(v);
        }
        colors[it.key()] = arr;
    }
    json["colors"] = colors;

    // Texture paths
    QJsonObject textures;
    for (auto it = material.texturePaths.begin(); it != material.texturePaths.end(); ++it) {
        textures[it.key()] = it.value();
    }
    json["textures"] = textures;

    return json;
}
