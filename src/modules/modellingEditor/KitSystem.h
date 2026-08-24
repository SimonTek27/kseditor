#pragma once

#include <QString>
#include <QVector3D>
#include <QVector4D>
#include <QMap>
#include <QVector>

namespace ks {
namespace modelling {

/// Preset data for modeling operations
struct PresetData {
    QString name;
    QString category;
    QVector3D position;      // Default position offset
    QVector3D rotation;      // Default rotation offset (degrees)
    QVector3D scale;         // Default scale multiplier
    QVector4D color;         // UI color (r,g,b,a)
    QString material;        // Associated material name
    QString description;     // Human-readable description
    QMap<QString, QVariant> parameters;  // Custom parameters
    
    PresetData() : position(0,0,0), rotation(0,0,0), scale(1,1,1), color(1,1,1,1) {}
};

/// Kit system - collection of presets for modeling workflows
class KitSystem : public QObject {
    Q_OBJECT

public:
    explicit KitSystem(QObject* parent = nullptr);
    ~KitSystem();

    // Preset management
    void addPreset(const PresetData& preset);
    bool removePreset(const QString& name);
    PresetData getPreset(const QString& name) const;
    QVector<PresetData> getPresets(const QString& category = QString()) const;
    int presetCount() const;
    
    // Kit management (grouped presets)
    void addKit(const QString& kitName, const QVector<QString>& presetNames);
    QVector<QString> getKitPresets(const QString& kitName) const;
    QVector<QString> getAllKitNames() const;
    
    // Save/load
    bool saveToFile(const QString& path) const;
    bool loadFromFile(const QString& path);
    
    // Built-in presets
    void initializeBuiltInPresets();

signals:
    void presetAdded(const PresetData& preset);
    void presetRemoved(const QString& name);
    void kitAdded(const QString& kitName);
    void kitRemoved(const QString& kitName);
    void presetsChanged();

private:
    QVector<PresetData> m_presets;
    QMap<QString, QVector<QString>> m_kits;  // kitName -> list of preset names
    QString m_presetsPath;
};

} // namespace modelling
} // namespace ks