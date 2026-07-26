#pragma once

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QVector>
#include <QFile>
#include <QDir>

/**
 * @brief Assetto Corsa Data (ACD) Archive Parser
 *
 * Parses and extracts data.acd files used by Assetto Corsa for car physics data.
 * ACD files are encrypted archives containing INI configuration files.
 *
 * Based on reverse engineering from community tools:
 * - AssettoTools (github.com/0danny/AssettoTools) - ACD extraction/encryption
 * - Kn5Decrypt (github.com/SeizureSaladd/Kn5Decrypt) - ACD decryption
 */
class ACDParser {
public:
    struct ACDFileEntry {
        QString fileName;
        quint32 offset;
        quint32 size;
        QByteArray data;
    };

    struct ACDArchive {
        QString filePath;
        QString folderName;
        QVector<ACDFileEntry> files;
        bool isEncrypted = false;
        bool isValid = false;
    };

    // Main operations
    static ACDArchive parse(const QString& acdPath, QString* error = nullptr);
    static bool extractAll(const QString& acdPath, const QString& outputDir, QString* error = nullptr);
    static bool createArchive(const QString& inputDir, const QString& acdPath, const QString& folderName, QString* error = nullptr);

    // Encryption/Decryption
    static QString createKey(const QString& folderName);
    static QByteArray decrypt(const QByteArray& data, const QString& key);
    static QByteArray encrypt(const QByteArray& data, const QString& key);

    // Validation
    static bool isValidACD(const QString& filePath);
    static bool isEncrypted(const QString& filePath);

    // Utility
    static QStringList getStandardFileNames();
    static QString getLastError() { return m_lastError; }

private:
    static bool parseHeader(QDataStream& stream, ACDArchive& archive);
    static bool parseFileEntries(QDataStream& stream, ACDArchive& archive);
    static bool extractFileData(QDataStream& stream, ACDArchive& archive);

    static QString m_lastError;
};

/**
 * @brief ACD Manager - High-level interface for ACD operations
 *
 * Provides convenient methods for managing ACD files in car directories.
 */
class ACDManager {
public:
    explicit ACDManager(const QString& carPath);

    // Detection
    bool hasACD() const;
    bool isACDEncrypted() const;
    QString getACDPath() const;

    // Extraction
    bool extractToFolder(const QString& outputDir = QString());
    bool extractToDefaultLocation();

    // Repacking
    bool repackFromFolder(const QString& inputDir = QString());
    bool repackFromDefaultLocation();

    // File access
    QByteArray getFileContent(const QString& fileName) const;
    QStringList getFileList() const;
    bool hasFile(const QString& fileName) const;

    // Status
    QString getStatus() const;
    QStringList getErrors() const { return m_errors; }

private:
    QString m_carPath;
    QString m_acdPath;
    ACDParser::ACDArchive m_archive;
    QStringList m_errors;
};
