#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <cstdint>

namespace ks {

// KSPKG file table entry (0x100 bytes)
#pragma pack(push, 1)
struct ACEFileTableEntry {
    char     filePath[0xE0];      // +0x00  Null-terminated path
    int32_t  alignment;           // +0xE0
    uint16_t flags;               // +0xE4  (Directory=0x01, XorCipher=0x100)
    uint16_t pathLength;          // +0xE6
    uint64_t pathFnv1a;           // +0xE8  FNV1a-64 hash (0 = end of table)
    int64_t  fileSize;            // +0xF0
    int64_t  fileOffset;          // +0xF8
};
#pragma pack(pop)

static_assert(sizeof(ACEFileTableEntry) == 0x100, "FileTableEntry must be 256 bytes");

enum ACEFileFlags : uint16_t {
    ACEFlagDirectory = (1 << 0),   // 0x0001
    ACEFlagXorCipher = (1 << 8),   // 0x0100
};

struct ACEPackageEntry {
    QString name;
    QString path;
    int64_t offset = 0;
    int64_t size = 0;
    uint16_t flags = 0;
    bool isDirectory() const { return (flags & ACEFlagDirectory) != 0; }
    bool isXored() const { return (flags & ACEFlagXorCipher) != 0; }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["offset"] = static_cast<qint64>(offset);
        obj["size"] = static_cast<qint64>(size);
        obj["flags"] = static_cast<qint64>(flags);
        obj["isDirectory"] = isDirectory();
        obj["isXored"] = isXored();
        return obj;
    }
};

struct ACEPackageManifest {
    QString packagePath;
    quint64 fileSize = 0;
    quint64 xorKey = 0;
    quint32 entryCount = 0;
    QList<ACEPackageEntry> entries;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["packagePath"] = packagePath;
        obj["fileSize"] = static_cast<qint64>(fileSize);
        obj["xorKey"] = static_cast<qint64>(xorKey);
        obj["entryCount"] = static_cast<qint64>(entryCount);
        QJsonArray arr;
        for (const auto& entry : entries) {
            arr.append(entry.toJson());
        }
        obj["entries"] = arr;
        return obj;
    }
};

class ACEPackageParser {
public:
    ACEPackageParser() = default;

    bool open(const QString& filePath);
    void close();
    bool isOpen() const { return m_file.isOpen(); }

    bool readManifest();
    const ACEPackageManifest& manifest() const { return m_manifest; }

    QByteArray readFile(const QString& entryPath);
    QByteArray readFile(int index);
    QByteArray readFileRaw(const QString& entryPath);

    QStringList entryNames() const;
    QStringList fileNames() const;
    int entryCount() const { return m_manifest.entryCount; }
    bool hasEntry(const QString& path) const;
    int findEntry(const QString& path) const;

    static bool isKspkgFile(const QString& filePath);
    static quint64 extractXorKey(const QString& filePath);

private:
    QFile m_file;
    ACEPackageManifest m_manifest;

    bool readFileTable();
    void xorDecrypt(char* data, qint64 size, quint64 key, qint64 offset);
};

class ACEPackageExtractor {
public:
    static bool extractAll(const QString& packagePath, const QString& outputDir);
    static bool extractFile(const QString& packagePath, const QString& entryPath, const QString& outputPath);
    static QStringList listContents(const QString& packagePath);
};

} // namespace ks
