#pragma once

#include <QString>
#include <QColor>
#include <QVector>
#include <QMap>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {
namespace paint {

class PaintSystem {
public:
    struct PaintLayer {
        QString name;
        QString type;           // "decal", "paint", "texture", "vector"
        float opacity = 1.0f;
        float position[2] = {0, 0};
        float size[2] = {1, 1};
        float rotation = 0.0f;
        QString texturePath;
        QColor tintColor = Qt::white;
        bool visible = true;
        QString vectorData;     // JSON-serialized vector shapes for "vector" layers
    };

    struct SkinConfig {
        QString name;
        QString path;
        QString baseColor;
        QVector<PaintLayer> layers;
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
    static bool addLayer(SkinConfig& config, const PaintLayer& layer);
    static bool removeLayer(SkinConfig& config, int index);
    static bool moveLayer(SkinConfig& config, int fromIndex, int toIndex);
    static bool updateLayer(SkinConfig& config, int index, const PaintLayer& layer);

    // License plate
    static bool generateLicensePlate(const QString& text, const QString& country,
                                          const QString& outputPath);
    static QStringList getSupportedCountries();
    static bool isValidPlateText(const QString& text, const QString& country);

    // Preview generation
    static bool generatePreview(const QString& skinPath);
    static bool hasPreview(const QString& skinPath);
    static QString getPreviewPath(const QString& skinPath);
    static QImage renderVectorPreview(const QString& vectorData, int width, int height);

    // Skin export/import
    static bool exportSkin(const QString& skinPath, const QString& outputPath);
    static bool importSkin(const QString& importPath, const QString& carPath);

    // Validation
    static bool validateSkin(const QString& skinPath, QString* error = nullptr);
    static bool validateLayer(const PaintLayer& layer, QString* error = nullptr);

    // DDS export
    static bool exportSkinAsDDS(const QString& skinPath, const QString& outputPath);
    static bool saveTextureAsDDS(const QImage& image, const QString& outputPath);

    // Decal import
    static bool importDecal(const QString& decalPath, const QString& skinPath);
    static QStringList getSupportedDecalFormats();

    // Template system
    struct PaintTemplate {
        QString name;
        QString description;
        QColor baseColor;
        QVector<QPair<QString, QColor>> stripes;
        bool hasRaceNumber = false;
        bool hasLicensePlate = false;
        int textureResolution = 2048;
        QVector<PaintLayer> presetLayers;
    };
    static QVector<PaintTemplate> getBuiltinTemplates();
    static bool createSkinFromTemplate(const QString& carPath, const QString& skinName,
                                        const PaintTemplate& tmpl);

    // Undo/redo
    struct UndoAction {
        enum Type { LayerAdd, LayerRemove, LayerMove, LayerModify, PaintStroke, BulkChange };
        Type type;
        int layerIndex = -1;
        PaintLayer oldLayer;
        PaintLayer newLayer;
        QString description;
    };
    static QVector<UndoAction> s_undoStack;
    static QVector<UndoAction> s_redoStack;
    static void pushUndo(const UndoAction& action);
    static bool canUndo();
    static bool canRedo();
    static UndoAction undoLast();
    static UndoAction redoLast();
    static void clearUndoRedo();

    // Color palette
    struct ColorSwatch {
        QString name;
        QColor color;
    };
    static QVector<ColorSwatch> getDefaultPalette();
    static QVector<ColorSwatch> loadPalette(const QString& path);
    static bool savePalette(const QVector<ColorSwatch>& palette, const QString& path);

    // Utility
    static QStringList getLayerTypes();
    static QString getDefaultSkinName();
    static bool isDefaultSkin(const QString& skinName);

private:
    static bool createDefaultFiles(const QString& skinPath);
    static bool createPreviewTexture(const QString& skinPath);
};

class PaintManager {
public:
    explicit PaintManager(const QString& carPath);

    bool loadSkins();
    bool createSkin(const QString& name);
    bool deleteSkin(const QString& name);
    bool duplicateSkin(const QString& sourceName, const QString& destName);

    QVector<PaintSystem::SkinInfo> getSkins() const { return m_skins; }
    PaintSystem::SkinConfig& currentConfig() { return m_config; }

    bool setCurrentSkin(const QString& skinName);
    bool saveCurrentSkin();
    bool addLayer(const PaintSystem::PaintLayer& layer);
    bool removeLayer(int index);

    bool generateLicensePlate(const QString& text, const QString& country);

private:
    QString m_carPath;
    QVector<PaintSystem::SkinInfo> m_skins;
    PaintSystem::SkinConfig m_config;
    QString m_currentSkin;
};

} // namespace paint
} // namespace ks