#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>
#include <QTextStream>
#include <QFile>
#include <QProcess>

namespace ks {

class ACEContentQMLBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAceInstalled READ isAceInstalled NOTIFY aceStatusChanged)
    Q_PROPERTY(QString aceRoot READ aceRoot NOTIFY aceStatusChanged)
    Q_PROPERTY(QStringList cars READ cars NOTIFY contentChanged)
    Q_PROPERTY(QStringList tracks READ tracks NOTIFY contentChanged)
    Q_PROPERTY(QStringList mods READ mods NOTIFY contentChanged)

public:
    explicit ACEContentQMLBridge(QObject* parent = nullptr) : QObject(parent) {
        detectInstallation();
    }

    static ACEContentQMLBridge* instance() {
        static ACEContentQMLBridge* s = new ACEContentQMLBridge();
        return s;
    }

    bool isAceInstalled() const { return !m_aceRoot.isEmpty(); }
    QString aceRoot() const { return m_aceRoot; }
    QStringList cars() const { return listContentDir(m_aceRoot + "/content/cars"); }
    QStringList tracks() const { return listContentDir(m_aceRoot + "/content/tracks"); }
    QStringList mods() const { return listMods(); }

    Q_INVOKABLE bool detectInstallation() {
        QString found = findBestInstallation();
        if (!found.isEmpty() && found != m_aceRoot) {
            m_aceRoot = found;
            emit aceStatusChanged();
            emit contentChanged();
            return true;
        }
        return false;
    }

    Q_INVOKABLE void setCustomPath(const QString& path) {
        if (QDir(path).exists()) {
            m_aceRoot = path;
            emit aceStatusChanged();
            emit contentChanged();
        }
    }

    Q_INVOKABLE QVariantList listCarsDetailed() {
        return listContentDirDetailed(m_aceRoot + "/content/cars", "car");
    }

    Q_INVOKABLE QVariantList listTracksDetailed() {
        return listContentDirDetailed(m_aceRoot + "/content/tracks", "track");
    }

    Q_INVOKABLE QVariantList listModsDetailed() {
        QVariantList result;
        QString modsDir = savedGamesPath() + "/mods";
        QDir dir(modsDir);
        if (!dir.exists()) return result;
        for (const QFileInfo& fi : dir.entryInfoList(QStringList() << "*.kspkg", QDir::Files)) {
            QVariantMap m;
            m["name"] = fi.fileName();
            m["path"] = fi.absoluteFilePath();
            m["type"] = "mod";
            m["size"] = fi.size();
            result.append(m);
        }
        return result;
    }

    Q_INVOKABLE QVariantMap getCarInfo(const QString& carName) {
        QVariantMap info;
        QString carPath = m_aceRoot + "/content/cars/" + carName;
        if (!QDir(carPath).exists()) return info;
        info["name"] = carName;
        info["path"] = carPath;
        info["exists"] = true;

        QDir skinsDir(carPath + "/skins");
        if (skinsDir.exists()) {
            info["skins"] = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            info["skinCount"] = info["skins"].toStringList().size();
        } else {
            info["skins"] = QStringList();
            info["skinCount"] = 0;
        }

        qint64 size = 0;
        QDirIterator it(carPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); size += it.fileInfo().size(); }
        info["totalSize"] = size;

        return info;
    }

    Q_INVOKABLE QVariantList getSkins(const QString& carName) {
        QVariantList result;
        QString skinsPath = m_aceRoot + "/content/cars/" + carName + "/skins";
        QDir skinsDir(skinsPath);
        if (!skinsDir.exists()) return result;
        for (const QString& s : skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QVariantMap m;
            m["name"] = s;
            m["path"] = skinsDir.absoluteFilePath(s);
            m["preview"] = QFile::exists(skinsDir.absoluteFilePath(s) + "/preview.png")
                ? skinsDir.absoluteFilePath(s) + "/preview.png" : "";
            result.append(m);
        }
        return result;
    }

    Q_INVOKABLE QVariantMap getModInfo(const QString& modFileName) {
        QVariantMap info;
        QString modsDir = savedGamesPath() + "/mods";
        QString modPath = modsDir + "/" + modFileName;
        QFileInfo fi(modPath);
        if (!fi.exists()) return info;
        info["name"] = modFileName;
        info["path"] = modPath;
        info["size"] = fi.size();
        info["lastModified"] = fi.lastModified().toString(Qt::ISODate);
        return info;
    }

    Q_INVOKABLE QStringList searchContent(const QString& query) {
        QStringList results;
        QString lower = query.toLower();
        for (const QString& dir : QStringList() << m_aceRoot + "/content/cars" << m_aceRoot + "/content/tracks") {
            QDir d(dir);
            if (!d.exists()) continue;
            for (const QString& entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (entry.toLower().contains(lower))
                    results.append(entry);
            }
        }
        return results;
    }

    Q_INVOKABLE bool launchGame(const QString& track = QString(), const QString& car = QString()) {
        QString exePath = m_aceRoot + "/AssettoCorsaEVO.exe";
        if (!QFileInfo::exists(exePath)) return false;
        QStringList args;
        if (!track.isEmpty()) args << "-track" << track;
        if (!car.isEmpty()) args << "-car" << car;
        return QProcess::startDetached(exePath, args, m_aceRoot);
    }

signals:
    void aceStatusChanged();
    void contentChanged();

private:
    QString m_aceRoot;

    static QString savedGamesPath() {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ACE-Modder";
    }

    static QStringList findAllInstallations() {
        QStringList installations;
        QSettings steamSettings("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
        QString steamPath = steamSettings.value("SteamPath").toString();
        if (!steamPath.isEmpty()) {
            QString libraryFile = steamPath + "/steamapps/libraryfolders.vdf";
            if (QFile::exists(libraryFile)) {
                QFile file(libraryFile);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (line.contains("\"path\"")) {
                            QStringList parts = line.split("\"");
                            if (parts.size() >= 4) {
                                QString libPath = parts[3].replace("\\\\", "/");
                                QString acePath = libPath + "/steamapps/common/Assetto Corsa EVO";
                                if (isValidInstallation(acePath) && !installations.contains(acePath))
                                    installations.append(acePath);
                            }
                        }
                    }
                }
            }
        }
        QStringList fallbacks = {
            "C:/Program Files (x86)/Steam/steamapps/common/Assetto Corsa EVO",
            "D:/Steam/steamapps/common/Assetto Corsa EVO",
            "E:/Steam/steamapps/common/Assetto Corsa EVO",
            "F:/SteamLibrary/steamapps/common/Assetto Corsa EVO",
        };
        for (const QString& p : fallbacks) {
            if (isValidInstallation(p) && !installations.contains(p))
                installations.append(p);
        }
        return installations;
    }

    static bool isValidInstallation(const QString& path) {
        if (path.isEmpty()) return false;
        QDir dir(path);
        if (!dir.exists()) return false;
        if (QFile::exists(path + "/AssettoCorsaEVO.exe")) return true;
        if (QFile::exists(path + "/content.kspkg")) return true;
        QDir contentDir(path + "/content");
        return contentDir.exists() && (contentDir.exists("cars") || contentDir.exists("tracks"));
    }

    static QString findBestInstallation() {
        QStringList installs = findAllInstallations();
        return installs.isEmpty() ? QString() : installs.first();
    }

    static QStringList listContentDir(const QString& dirPath) {
        QDir dir(dirPath);
        if (!dir.exists()) return QStringList();
        return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    static QVariantList listContentDirDetailed(const QString& dirPath, const QString& type) {
        QVariantList result;
        QDir dir(dirPath);
        if (!dir.exists()) return result;
        for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QVariantMap m;
            m["name"] = fi.fileName();
            m["path"] = fi.absoluteFilePath();
            m["type"] = type;
            m["preview"] = QFile::exists(fi.absoluteFilePath() + "/preview.png")
                ? fi.absoluteFilePath() + "/preview.png" : "";
            result.append(m);
        }
        return result;
    }

    static QStringList listMods() {
        QString modsDir = savedGamesPath() + "/mods";
        QDir dir(modsDir);
        if (!dir.exists()) return QStringList();
        return dir.entryList(QStringList() << "*.kspkg", QDir::Files);
    }
};


class ACEProtobufQmlBridge : public QObject {
    Q_OBJECT

public:
    explicit ACEProtobufQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QVariantMap decode(const QByteArray& data) {
        QVariantMap result;
        if (data.isEmpty()) return result;
        int pos = 0;
        decodeMessage(data, pos, data.size(), result);
        return result;
    }

    Q_INVOKABLE QVariantMap decodeHex(const QString& hexStr) {
        QByteArray data = QByteArray::fromHex(hexStr.toLatin1());
        return decode(data);
    }

    Q_INVOKABLE QString extractString(const QByteArray& data, int offset, int length) {
        if (offset + length > data.size()) return {};
        return QString::fromUtf8(data.mid(offset, length));
    }

    Q_INVOKABLE QStringList knownFiles() {
        return {
            "CarData", "CarBehaviour", "CarSetup", "TyresData",
            "PhysicsSnapshot", "AudioData", "Scene", "TrackData",
            "Weather", "Renderer", "GraphicsConfig", "SessionData"
        };
    }

    Q_INVOKABLE QVariantMap knownFieldNames() {
        QVariantMap m;
        m["tyre_mapping"] = QStringList{
            "WIDTH", "RADIUS", "RATE", "DAMP", "DY0", "DX0",
            "DY1", "DX1", "DY2", "DX2", "CROSS_HEIGHT", "CROSS_WIDTH",
            "TREAD_H", "TREAD_V", "TREAD_SIZE", "COMBINED_SLOPE",
            "COMBINED_CURVE", "SIDE_SLIP", "LONG_SLIP", "MUE"
        };
        return m;
    }

private:
    static int decodeVarint(const QByteArray& data, int& pos, int maxPos) {
        int result = 0;
        int shift = 0;
        while (pos < maxPos) {
            quint8 byte = static_cast<quint8>(data[pos]);
            result |= (byte & 0x7F) << shift;
            pos++;
            if (!(byte & 0x80)) break;
            shift += 7;
        }
        return result;
    }

    static void decodeMessage(const QByteArray& data, int& pos, int maxPos, QVariantMap& out) {
        while (pos < maxPos) {
            int startPos = pos;
            int tag = decodeVarint(data, pos, maxPos);
            if (tag == 0 && pos == startPos) break;
            int wireType = tag & 0x07;
            int fieldNum = tag >> 3;
            QString key = QString("field_%1").arg(fieldNum);

            if (wireType == 0) {
                out[key] = static_cast<qint64>(decodeVarint(data, pos, maxPos));
            } else if (wireType == 5) {
                if (pos + 4 > maxPos) break;
                quint32 v;
                memcpy(&v, data.constData() + pos, 4);
                out[key] = v;
                pos += 4;
            } else if (wireType == 1) {
                if (pos + 8 > maxPos) break;
                quint64 v;
                memcpy(&v, data.constData() + pos, 8);
                out[key] = v;
                pos += 8;
            } else if (wireType == 2) {
                int len = decodeVarint(data, pos, maxPos);
                if (pos + len > maxPos) break;
                QByteArray chunk = data.mid(pos, len);
                bool printable = true;
                int printCount = 0;
                for (int i = 0; i < qMin(len, 64); i++) {
                    char c = chunk[i];
                    if (c >= 0x20 && c < 0x7F) printCount++;
                    else if (c != '\n' && c != '\r' && c != '\t') { printable = false; break; }
                }
                if (printable && printCount > len / 2) {
                    out[key] = QString::fromUtf8(chunk);
                } else {
                    QVariantMap nested;
                    int nestedPos = 0;
                    decodeMessage(chunk, nestedPos, len, nested);
                    if (!nested.isEmpty()) {
                        out[key] = nested;
                    } else {
                        out[key] = chunk.toHex();
                    }
                }
                pos += len;
            } else {
                break;
            }
        }
    }
};

} // namespace ks
