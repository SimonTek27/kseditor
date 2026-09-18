#include "RaceFlagAssetLoader.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>

namespace ks::engine::assets {

// ============================================================================
// Singleton
// ============================================================================

RaceFlagAssetLoader* RaceFlagAssetLoader::instance() {
    static RaceFlagAssetLoader inst;
    return &inst;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

RaceFlagAssetLoader::RaceFlagAssetLoader(QObject* parent)
    : QObject(parent) {
    initializeFlagNames();
}

RaceFlagAssetLoader::~RaceFlagAssetLoader() {
}

// ============================================================================
// Initialization
// ============================================================================

bool RaceFlagAssetLoader::loadFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "RaceFlagAssetLoader: Directory not found:" << dirPath;
        emit error(QString("Directory not found: %1").arg(dirPath));
        return false;
    }

    m_basePath = dirPath;
    int loadedCount = 0;

    for (auto it = m_filenames.constBegin(); it != m_filenames.constEnd(); ++it) {
        RaceFlagType type = it.key();
        QString filename = it.value();

        QString fullPath = dir.filePath(filename);
        if (QFile::exists(fullPath)) {
            QImage image(fullPath);
            if (!image.isNull()) {
                m_flags[type] = image;
                loadedCount++;
            } else {
                qWarning() << "RaceFlagAssetLoader: Failed to load image:" << fullPath;
            }
        } else {
            qDebug() << "RaceFlagAssetLoader: Flag not found:" << fullPath;
        }
    }

    m_loaded = (loadedCount > 0);
    if (m_loaded) {
        qDebug() << "RaceFlagAssetLoader: Loaded" << loadedCount << "flags from" << dirPath;
        emit loaded();
    } else {
        emit error(QString("No flags loaded from: %1").arg(dirPath));
    }

    return m_loaded;
}

bool RaceFlagAssetLoader::loadDefault() {
    // Try multiple search paths
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchPaths = {
        appDir + "/system/raceflags",
        appDir + "/../system/raceflags",
        appDir + "/../../bin/simulator/system/raceflags",
    };

    for (const QString& path : searchPaths) {
        if (QDir(path).exists()) {
            return loadFromDirectory(path);
        }
    }

    qWarning() << "RaceFlagAssetLoader: Could not find raceflags directory";
    emit error("Raceflags directory not found");
    return false;
}

// ============================================================================
// Asset access
// ============================================================================

bool RaceFlagAssetLoader::hasFlag(RaceFlagType type) const {
    return m_flags.contains(type);
}

QImage RaceFlagAssetLoader::getFlag(RaceFlagType type) const {
    return m_flags.value(type);
}

QByteArray RaceFlagAssetLoader::getFlagData(RaceFlagType type) const {
    QImage image = getFlag(type);
    if (image.isNull()) return QByteArray();

    QByteArray buffer;
    QBuffer buf(&buffer);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return buffer;
}

QString RaceFlagAssetLoader::getFlagFilename(RaceFlagType type) const {
    return m_filenames.value(type);
}

// ============================================================================
// Batch access
// ============================================================================

QMap<RaceFlagType, QImage> RaceFlagAssetLoader::getAllFlags() const {
    return m_flags;
}

QVector<RaceFlagType> RaceFlagAssetLoader::getAvailableFlags() const {
    return m_flags.keys().toVector();
}

// ============================================================================
// Private helpers
// ============================================================================

void RaceFlagAssetLoader::initializeFlagNames() {
    m_filenames[RaceFlagType::Green] = QStringLiteral("greenFlag.png");
    m_filenames[RaceFlagType::Yellow] = QStringLiteral("yellowFlag.png");
    m_filenames[RaceFlagType::Red] = QStringLiteral("redFlag.png");
    m_filenames[RaceFlagType::Blue] = QStringLiteral("blueFlag.png");
    m_filenames[RaceFlagType::White] = QStringLiteral("whiteFlag.png");
    m_filenames[RaceFlagType::Black] = QStringLiteral("blackFlag.png");
    m_filenames[RaceFlagType::Checked] = QStringLiteral("checkedFlag.png");
    m_filenames[RaceFlagType::SC] = QStringLiteral("sc.png");
    m_filenames[RaceFlagType::VSC] = QStringLiteral("vsc.png");
    m_filenames[RaceFlagType::Penalty] = QStringLiteral("penalty.png");
    m_filenames[RaceFlagType::WrongWay] = QStringLiteral("wrongway.png");
    m_filenames[RaceFlagType::Limit60] = QStringLiteral("60limit.png");
    m_filenames[RaceFlagType::Limit80] = QStringLiteral("80limit.png");
    m_filenames[RaceFlagType::TrafficGreen] = QStringLiteral("texture_trafficlight_green.png");
    m_filenames[RaceFlagType::TrafficYellow] = QStringLiteral("texture_trafficlight_yellow.png");
    m_filenames[RaceFlagType::TrafficRed] = QStringLiteral("texture_trafficlight_red.png");
    m_filenames[RaceFlagType::TrafficOff] = QStringLiteral("texture_trafficlight_off.png");
}

QString RaceFlagAssetLoader::findSystemDllDirectory() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchPaths = {
        appDir + "/system",
        appDir + "/../system",
        appDir + "/../../bin/simulator/system",
    };

    for (const QString& path : searchPaths) {
        if (QDir(path).exists()) {
            return path;
        }
    }
    return QString();
}

} // namespace ks::engine::assets
