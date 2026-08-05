#include "ACEPackageParser.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDataStream>

namespace ks {

// ─── KSPKG format constants ─────────────────────────────────────────────────

static constexpr qint64 FILE_TABLE_SIZE = 0x2000000;     // 32 MB
static constexpr int    ENTRY_SIZE      = 0x100;          // 256 bytes
static constexpr int    MAX_ENTRIES     = 0x20000;        // 131,072
static constexpr int    PATH_FIELD_SIZE = 0xE0;           // 224 bytes

// Default XOR key for known builds (can be overridden by extraction)
static constexpr quint64 DEFAULT_XOR_KEY = 0xDC533268824E4400ULL;

// ─── ACEPackageParser ────────────────────────────────────────────────────────

bool ACEPackageParser::open(const QString& filePath) {
    if (m_file.isOpen()) close();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ACEPackageParser] Cannot open:" << filePath << m_file.errorString();
        return false;
    }

    m_manifest.packagePath = filePath;
    m_manifest.fileSize = m_file.size();
    return true;
}

void ACEPackageParser::close() {
    m_file.close();
    m_manifest = ACEPackageManifest();
}

bool ACEPackageParser::readManifest() {
    if (!m_file.isOpen()) return false;

    m_manifest.xorKey = extractXorKey(m_manifest.packagePath);
    if (m_manifest.xorKey == 0) {
        qWarning() << "[ACEPackageParser] Could not extract XOR key, using default";
        m_manifest.xorKey = DEFAULT_XOR_KEY;
    }

    return readFileTable();
}

bool ACEPackageParser::readFileTable() {
    // File table is at the end of the file
    qint64 tableStart = m_manifest.fileSize - FILE_TABLE_SIZE;
    if (tableStart < 0) {
        qWarning() << "[ACEPackageParser] File too small for file table:" << m_manifest.fileSize;
        return false;
    }

    if (!m_file.seek(tableStart)) {
        qWarning() << "[ACEPackageParser] Cannot seek to file table at" << tableStart;
        return false;
    }

    QByteArray tableData = m_file.read(FILE_TABLE_SIZE);
    if (tableData.size() != FILE_TABLE_SIZE) {
        qWarning() << "[ACEPackageParser] Short read on file table:" << tableData.size();
        return false;
    }

    // XOR-decrypt the file table
    xorDecrypt(tableData.data(), tableData.size(), m_manifest.xorKey, 0);

    // Parse entries
    m_manifest.entries.clear();
    const char* ptr = tableData.constData();
    const char* end = tableData.constData() + tableData.size();

    for (int i = 0; i < MAX_ENTRIES && ptr + ENTRY_SIZE <= end; ++i) {
        const auto* entry = reinterpret_cast<const ACEFileTableEntry*>(ptr);

        // End of table marker: FNV1a hash == 0
        if (entry->pathFnv1a == 0) {
            break;
        }

        ACEPackageEntry pkgEntry;
        pkgEntry.path = QString::fromLatin1(entry->filePath, qMin(entry->pathLength, static_cast<quint16>(PATH_FIELD_SIZE)));
        pkgEntry.name = pkgEntry.path;
        pkgEntry.offset = entry->fileOffset;
        pkgEntry.size = entry->fileSize;
        pkgEntry.flags = entry->flags;

        m_manifest.entries.append(pkgEntry);
        ptr += ENTRY_SIZE;
    }

    m_manifest.entryCount = m_manifest.entries.size();
    qDebug() << "[ACEPackageParser] Read" << m_manifest.entryCount << "entries,"
             << "XOR key:" << Qt::hex << m_manifest.xorKey;
    return true;
}

QByteArray ACEPackageParser::readFile(const QString& entryPath) {
    int idx = findEntry(entryPath);
    if (idx < 0) return QByteArray();
    return readFile(idx);
}

QByteArray ACEPackageParser::readFile(int index) {
    if (index < 0 || index >= m_manifest.entries.size()) return QByteArray();

    const ACEPackageEntry& entry = m_manifest.entries[index];
    if (entry.isDirectory()) return QByteArray();

    if (!m_file.seek(entry.offset)) {
        qWarning() << "[ACEPackageParser] Cannot seek to offset" << entry.offset;
        return QByteArray();
    }

    QByteArray data = m_file.read(entry.size);
    if (data.size() != entry.size) {
        qWarning() << "[ACEPackageParser] Short read:" << data.size() << "expected" << entry.size;
        return QByteArray();
    }

    // XOR-decrypt file data if flagged
    if (entry.isXored()) {
        xorDecrypt(data.data(), data.size(), m_manifest.xorKey, entry.offset);
    }

    return data;
}

QByteArray ACEPackageParser::readFileRaw(const QString& entryPath) {
    int idx = findEntry(entryPath);
    if (idx < 0) return QByteArray();

    const ACEPackageEntry& entry = m_manifest.entries[idx];
    if (!m_file.seek(entry.offset)) return QByteArray();
    return m_file.read(entry.size);
}

QStringList ACEPackageParser::entryNames() const {
    QStringList names;
    names.reserve(m_manifest.entries.size());
    for (const auto& entry : m_manifest.entries) {
        names.append(entry.path);
    }
    return names;
}

QStringList ACEPackageParser::fileNames() const {
    QStringList names;
    names.reserve(m_manifest.entries.size());
    for (const auto& entry : m_manifest.entries) {
        if (!entry.isDirectory()) {
            names.append(entry.path);
        }
    }
    return names;
}

bool ACEPackageParser::hasEntry(const QString& path) const {
    return findEntry(path) >= 0;
}

int ACEPackageParser::findEntry(const QString& path) const {
    for (int i = 0; i < m_manifest.entries.size(); ++i) {
        if (m_manifest.entries[i].path == path) {
            return i;
        }
    }
    return -1;
}

bool ACEPackageParser::isKspkgFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    if (file.size() < FILE_TABLE_SIZE + 16) return false;

    // Check if file table exists by trying to extract XOR key
    quint64 key = extractXorKey(filePath);
    return key != 0;
}

quint64 ACEPackageParser::extractXorKey(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return 0;

    qint64 fileSize = file.size();
    if (fileSize < FILE_TABLE_SIZE + 8) return 0;

    // The XOR key can be extracted from the last 8 bytes of the file table
    // because the table is 0-padded: KEY ^ 0 = KEY
    qint64 lastEntryOffset = fileSize - 8;
    if (!file.seek(lastEntryOffset)) return 0;

    QByteArray lastBytes = file.read(8);
    if (lastBytes.size() != 8) return 0;

    quint64 key = *reinterpret_cast<const quint64*>(lastBytes.constData());

    // Validate: key should not be all zeros or all ones
    if (key == 0 || key == 0xFFFFFFFFFFFFFFFFULL) return 0;

    return key;
}

void ACEPackageParser::xorDecrypt(char* data, qint64 size, quint64 key, qint64 offset) {
    if (key == 0) return;

    // XOR with 8-byte repeating key, starting from the given offset
    quint64 keyBytes[2];
    memcpy(keyBytes, &key, 8);

    char* keyPtr = reinterpret_cast<char*>(keyBytes);
    for (qint64 i = 0; i < size; ++i) {
        qint64 keyIdx = (offset + i) % 8;
        data[i] ^= keyPtr[keyIdx];
    }
}

// ─── ACEPackageExtractor ────────────────────────────────────────────────────

bool ACEPackageExtractor::extractAll(const QString& packagePath, const QString& outputDir) {
    ACEPackageParser parser;
    if (!parser.open(packagePath)) return false;
    if (!parser.readManifest()) return false;

    QDir outDir(outputDir);
    if (!outDir.exists()) outDir.mkpath(".");

    for (const auto& entry : parser.manifest().entries) {
        if (entry.isDirectory()) continue;

        QString outPath = outDir.filePath(entry.path);
        QDir entryDir = QFileInfo(outPath).absoluteDir();
        if (!entryDir.exists()) entryDir.mkpath(".");

        QByteArray data = parser.readFile(entry.path);
        if (data.isEmpty()) {
            qWarning() << "[ACEPackageExtractor] Failed to read:" << entry.path;
            continue;
        }

        QFile outFile(outPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            qWarning() << "[ACEPackageExtractor] Cannot write:" << outPath;
            continue;
        }
        outFile.write(data);
        outFile.close();
    }

    return true;
}

bool ACEPackageExtractor::extractFile(const QString& packagePath, const QString& entryPath,
                                       const QString& outputPath) {
    ACEPackageParser parser;
    if (!parser.open(packagePath)) return false;
    if (!parser.readManifest()) return false;

    QByteArray data = parser.readFile(entryPath);
    if (data.isEmpty()) return false;

    QFileInfo outInfo(outputPath);
    QDir outDir = outInfo.absoluteDir();
    if (!outDir.exists()) outDir.mkpath(".");

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) return false;
    outFile.write(data);
    outFile.close();
    return true;
}

QStringList ACEPackageExtractor::listContents(const QString& packagePath) {
    ACEPackageParser parser;
    if (!parser.open(packagePath)) return QStringList();
    if (!parser.readManifest()) return QStringList();
    return parser.entryNames();
}

} // namespace ks
