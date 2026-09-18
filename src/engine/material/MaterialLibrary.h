#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>

/**
 * @brief Material Library System for Assetto Corsa
 *
 * Allows saving, loading, and sharing material presets.
 * Based on features from modded KsEditor (ascobash.wordpress.com/2015/07/22/kseditor/)
 *
 * Materials are saved with:
 * - Shader name and type
 * - All shader properties (float, vector, color)
 * - Texture references
 * - Material name for identification
 */
class MaterialLibrary {
public:
    struct MaterialPreset {
        QString name;
        QString shaderName;
        QString category; // e.g., "car", "track", "interior", "glass"
        QMap<QString, float> floatProperties;
        QMap<QString, QVector<float>> vectorProperties;
        QMap<QString, QVector<float>> colorProperties;
        QMap<QString, QString> texturePaths;
        QDateTime createdDate;
        QDateTime modifiedDate;
        QString author;
        QString description;
        bool isBuiltIn = false;
    };

    // Library management
    static bool saveLibrary(const QString& libraryPath = QString());
    static bool loadLibrary(const QString& libraryPath);
    static bool createDefaultLibrary();

    // Material operations
    static bool saveMaterial(const MaterialPreset& material);
    static bool saveMaterialAs(const MaterialPreset& material, const QString& name);
    static bool deleteMaterial(const QString& name);
    static bool renameMaterial(const QString& oldName, const QString& newName);

    // Material access
    static MaterialPreset getMaterial(const QString& name);
    static QVector<MaterialPreset> getAllMaterials();
    static QVector<MaterialPreset> getMaterialsByCategory(const QString& category);
    static QVector<MaterialPreset> searchMaterials(const QString& query);
    static bool hasMaterial(const QString& name);

    // Categories
    static QStringList getCategories();
    static void setCategories(const QStringList& categories);

    // Import/Export
    static bool importMaterial(const QString& filePath);
    static bool exportMaterial(const QString& name, const QString& filePath);
    static bool exportAllMaterials(const QString& directory);

    // Built-in materials
    static void loadBuiltInMaterials();
    static QVector<MaterialPreset> getBuiltInMaterials();

    // Utility
    static QString getLibraryPath();
    static QString getDefaultLibraryPath();
    static QString getLastError() { return m_lastError; }

private:
    static bool parseMaterialJson(const QJsonObject& json, MaterialPreset& material);
    static QJsonObject materialToJson(const MaterialPreset& material);

    static QMap<QString, MaterialPreset> m_materials;
    static QStringList m_categories;
    static QString m_libraryPath;
    static QString m_lastError;
};

/**
 * @brief Material Library Widget - UI for browsing and managing materials
 */
class MaterialLibraryWidget;
