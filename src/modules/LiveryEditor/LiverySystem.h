#pragma once

#include <QString>
#include <QColor>
#include <QVector>
#include <QMap>
#include <QJsonObject>

/**
 * @brief Livery System for Assetto Corsa
 *
 * Manages car liveries and skins.
 * Based on:
 * - Content Manager livery editor features
 * - AC skin format documentation
 * - Custom Showroom livery generation
 *
 * Features:
 * - Skin management
 * - Livery layer system
 * - License plate generation
 * - Preview generation
 * - Skin export/import
 */
class LiverySystem {
public:
    struct LiveryLayer {
        QString name;
        QString type;           // "decal", "paint", "texture"
        float opacity = 1.0f;
        float position[2] = {0, 0};
        float size[2] = {1, 1};
        float rotation = 0.0f;
        QString texturePath;
        QColor tintColor = Qt::white;
        bool visible = true;
    };

    struct SkinConfig {
        QString name;
        QString path;
        QString baseColor;
        QVector<LiveryLayer> layers;
        QString licensePlateText;
        QString licensePlateCountry;
        bool hasNumber = false;
        int carNumber = 0;
        QString driverName;
        QString teamName;
    };

    struct SkinInfo {
        QString name;
        QString path;
        QString previewPath;
        qint64 size = 0;
        QDateTime lastModified;
        bool isValid = false;
    };

    // Skin management
    static QVector<SkinInfo> getSkins(const QString& carPath);
    static SkinInfo getSkinInfo(const QString& skinPath);
    static bool createSkin(const QString& carPath, const QString& skinName);
    static bool deleteSkin(const QString& carPath, const QString& skinName);
    static bool duplicateSkin(const QString& carPath, const QString& sourceName, const QString& destName);

    // Skin configuration
    static SkinConfig loadSkinConfig(const QString& skinPath);
    static bool saveSkinConfig(const SkinConfig& config, const QString& skinPath);
    static bool loadFromIni(SkinConfig& config, const QString& iniPath);
    static bool saveToIni(const SkinConfig& config, const QString& iniPath);

    // Layer operations
    static bool addLayer(SkinConfig& config, const LiveryLayer& layer);
    static bool removeLayer(SkinConfig& config, int index);
    static bool moveLayer(SkinConfig& config, int fromIndex, int toIndex);
    static bool updateLayer(SkinConfig& config, int index, const LiveryLayer& layer);

    // License plate
    static bool generateLicensePlate(const QString& text, const QString& country,
                                      const QString& outputPath);
    static QStringList getSupportedCountries();
    static bool isValidPlateText(const QString& text, const QString& country);

    // Preview generation
    static bool generatePreview(const QString& skinPath);
    static bool hasPreview(const QString& skinPath);
    static QString getPreviewPath(const QString& skinPath);

    // Skin export/import
    static bool exportSkin(const QString& skinPath, const QString& outputPath);
    static bool importSkin(const QString& importPath, const QString& carPath);

    // Validation
    static bool validateSkin(const QString& skinPath, QString* error = nullptr);
    static bool validateLayer(const LiveryLayer& layer, QString* error = nullptr);

    // Utility
    static QStringList getLayerTypes();
    static QString getDefaultSkinName();
    static bool isDefaultSkin(const QString& skinName);

private:
    static bool createDefaultFiles(const QString& skinPath);
    static bool createPreviewTexture(const QString& skinPath);
};

/**
 * @brief Livery Manager - High-level interface
 */
class LiveryManager {
public:
    explicit LiveryManager(const QString& carPath);

    // Operations
    bool loadSkins();
    bool createSkin(const QString& name);
    bool deleteSkin(const QString& name);
    bool duplicateSkin(const QString& sourceName, const QString& destName);

    // Access
    QVector<LiverySystem::SkinInfo> getSkins() const { return m_skins; }
    LiverySystem::SkinConfig& currentConfig() { return m_config; }

    // Current skin operations
    bool setCurrentSkin(const QString& skinName);
    bool saveCurrentSkin();
    bool addLayer(const LiverySystem::LiveryLayer& layer);
    bool removeLayer(int index);

    // License plate
    bool generateLicensePlate(const QString& text, const QString& country);

private:
    QString m_carPath;
    QVector<LiverySystem::SkinInfo> m_skins;
    LiverySystem::SkinConfig m_config;
    QString m_currentSkin;
};
