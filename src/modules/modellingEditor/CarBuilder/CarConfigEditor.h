#ifndef CARCONFIGEDITOR_H
#define CARCONFIGEDITOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QTableWidget>
#include <QVector3D>

namespace ks {
namespace modeler {

// Car INI configuration structure (AC-specific)
struct CarConfigParameter {
    QString section;
    QString key;
    QString displayName;
    QVariant value;
    QString type; // "float", "int", "string", "color", "bool"
    QVariant minValue;
    QVariant maxValue;
    QString description;
};

// Car Config Editor (FreeCAD Spreadsheet-inspired)
class CarConfigEditor : public QObject {
    Q_OBJECT
    
public:
    explicit CarConfigEditor(QObject* parent = nullptr);
    ~CarConfigEditor();
    
    // Load/save INI files
    bool loadINI(const QString& filePath);
    bool saveINI(const QString& filePath);
    bool isLoaded() const { return m_loaded; }
    
    // Parameters management
    int parameterCount() const { return m_params.size(); }
    CarConfigParameter getParameter(int index) const;
    CarConfigParameter findParameter(const QString& section, const QString& key) const;
    
    // Set values
    void setParameterValue(int index, const QVariant& value);
    void setParameterValue(const QString& section, const QString& key, const QVariant& value);
    
    // Sections
    QStringList getSections() const;
    int getParametersInSection(const QString& section) const;
    
    // Populate QTableWidget (FreeCAD Spreadsheet style)
    void populateTable(QTableWidget* table);
    void updateFromTable(QTableWidget* table);
    
    // Car-specific presets
    void applyCarPreset(const QString& presetName);
    QStringList getAvailablePresets() const;
    
signals:
    void configLoaded(const QString& filePath);
    void configChanged();
    void parameterChanged(int index, const QVariant& value);
    void error(const QString& message);
    
private:
    QString m_filePath;
    QVector<CarConfigParameter> m_params;
    bool m_loaded;
    
    void parseINISection(const QString& section, const QMap<QString, QVariant>& values);
    void addParameter(const QString& section, const QString& key, const QVariant& value);
};


// KS Asset Browser (Blender Asset Browser-inspired)
class KsAssetBrowser : public QObject {
    Q_OBJECT

public:
    explicit KsAssetBrowser(QObject* parent = nullptr);
    ~KsAssetBrowser();

    void setContentPath(const QString& path);
    QString contentPath() const { return m_contentPath; }

    void refresh();

    struct AssetInfo {
        QString name;
        QString filePath;
        QString type; // "car", "track", "texture", "sound", "physics"
        QString previewPath;
        QMap<QString, QVariant> metadata;
    };

    QVector<AssetInfo> getAssets(const QString& type = QString()) const;
    AssetInfo findAsset(const QString& name) const;
    QVector<AssetInfo> search(const QString& query) const;

    void setTypeFilter(const QString& type);
    bool generatePreview(const QString& assetPath, const QString& outputPath);
    QString getPreviewPath(const QString& assetPath) const;

signals:
    void assetsRefreshed();
    void assetSelected(const QString& assetPath);
    void error(const QString& message);

private:
    QString m_contentPath;
    QVector<AssetInfo> m_assets;
    QString m_typeFilter;

    void scanCars();
    void scanTracks();
    void scanTextures();
    void scanSounds();
    void scanPhysics();
};



// Reference Image System (FreeCAD TechDraw-inspired)
class ReferenceImageSystem : public QObject {
    Q_OBJECT
    
public:
    explicit ReferenceImageSystem(QObject* parent = nullptr);
    ~ReferenceImageSystem();
    
    struct ReferenceImage {
        QString name;
        QString imagePath;
        QVector3D position;
        QVector3D rotation;
        QVector3D scale;
        float opacity = 1.0f;
        bool visible = true;
        int drawLayer = 0; // 0=background, 1=overlay
    };
    
    // Manage reference images
    int addImage(const QString& imagePath, const QString& name = QString());
    bool removeImage(int index);
    void clearImages();
    
    int imageCount() const { return m_images.size(); }
    ReferenceImage getImage(int index) const;
    
    // Transform
    void setImagePosition(int index, const QVector3D& pos);
    void setImageRotation(int index, const QVector3D& rot);
    void setImageScale(int index, const QVector3D& scale);
    void setImageOpacity(int index, float opacity);
    void setImageVisibility(int index, bool visible);
    
    // Drawing overlay (for modeling over references)
    void setDrawLayer(int index, int layer);
    
signals:
    void imageAdded(int index, const QString& name);
    void imageRemoved(int index);
    void imageChanged(int index);

private:
    QVector<ReferenceImage> m_images;
};

} // namespace modeler
} // namespace ks

#endif // CARCONFIGEDITOR_H
