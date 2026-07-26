#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QFile>

/**
 * @brief KN5 File Decryptor for CSP-Protected Files
 *
 * Decrypts CSP-protected KN5 files used by Assetto Corsa Custom Shaders Patch.
 * Supports CSP encryption envelope __AC_SHADERS_PATCH_KN5ENC_v1__.
 *
 * Based on reverse engineering from:
 * - Kn5Decrypt (github.com/SeizureSaladd/Kn5Decrypt)
 */
class KN5Decrypt {
public:
    struct DecryptedKN5 {
        QByteArray kn5Data;
        QVector<QByteArray> textures;
        QVector<QByteArray> vertexMasks;
        QString manifest;
        bool fullRebuild = false;
        bool isValid = false;
    };

    // Main operations
    static DecryptedKN5 decrypt(const QString& kn5Path, QString* error = nullptr);
    static DecryptedKN5 decryptToFolder(const QString& kn5Path, const QString& outputDir, QString* error = nullptr);
    static bool unprotect(const QString& kn5Path, QString* error = nullptr);

    // Validation
    static bool isCSPProtected(const QString& kn5Path);
    static bool isKN5Unprotectable(const QString& kn5Path);

    // Utility
    static QString getLastError() { return m_lastError; }

private:
    static bool findEncryptedEnvelope(const QByteArray& data, int& offset, int& length);
    static bool parseEncryptedHeader(const QByteArray& headerData, int& encVersion, int& saltOffset);
    static QByteArray deriveKey(const QByteArray& salt, const QString& folderName);
    static bool decryptTextures(const QByteArray& encryptedTextures, QVector<QByteArray>& textures, const QByteArray& key);
    static bool decryptVertexMasks(const QByteArray& encryptedMasks, QVector<QByteArray>& masks, const QByteArray& key);
    static bool rebuildKN5(const QByteArray& originalBody, const QVector<QByteArray>& textures,
                          const QVector<QByteArray>& vertexMasks, QByteArray& result);

    static QString m_lastError;
};