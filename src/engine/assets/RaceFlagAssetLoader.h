#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QImage>
#include <QByteArray>

namespace ks::engine::assets {

// ============================================================================
// RaceFlagType - Enumeration of race flag types
// ============================================================================

enum class RaceFlagType {
    Green = 0,
    Yellow,
    Red,
    Blue,
    White,
    Black,
    Checked,
    SC,             // Safety Car
    VSC,            // Virtual Safety Car
    Penalty,
    WrongWay,
    Limit60,        // 60 km/h limit
    Limit80,        // 80 km/h limit
    TrafficGreen,
    TrafficYellow,
    TrafficRed,
    TrafficOff,
    Count
};

// ============================================================================
// RaceFlagAssetLoader - Loads race flag textures from system/raceflags/
// ============================================================================

class RaceFlagAssetLoader : public QObject {
    Q_OBJECT

public:
    static RaceFlagAssetLoader* instance();

    explicit RaceFlagAssetLoader(QObject* parent = nullptr);
    ~RaceFlagAssetLoader();

    // --- Initialization ---
    bool loadFromDirectory(const QString& dirPath);
    bool loadDefault();

    // --- Asset access ---
    bool hasFlag(RaceFlagType type) const;
    QImage getFlag(RaceFlagType type) const;
    QByteArray getFlagData(RaceFlagType type) const;
    QString getFlagFilename(RaceFlagType type) const;

    // --- Batch access ---
    QMap<RaceFlagType, QImage> getAllFlags() const;
    QVector<RaceFlagType> getAvailableFlags() const;

    // --- Utility ---
    int getFlagCount() const { return m_flags.size(); }
    bool isLoaded() const { return m_loaded; }
    QString basePath() const { return m_basePath; }

signals:
    void loaded();
    void error(const QString& message);

private:
    void initializeFlagNames();
    QString findSystemDllDirectory() const;

    QMap<RaceFlagType, QImage> m_flags;
    QMap<RaceFlagType, QString> m_filenames;
    QString m_basePath;
    bool m_loaded = false;
};

} // namespace ks::engine::assets
