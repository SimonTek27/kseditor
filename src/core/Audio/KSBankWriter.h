#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>

#include "KSFSPROImporter.h"

namespace ks { namespace audio {

class KSAudioProject;
struct SoundBank;

// ============================================================================
// KSBankWriter — writes FMOD Studio 1.08.x compatible .bank files (FEV2)
//
// Output: binary .bank files that Assetto Corsa can load.
// Also generates GUIDs.txt for AC event GUID resolution.
// ============================================================================

class KSBankWriter : public QObject {
    Q_OBJECT
public:
    explicit KSBankWriter(QObject* parent = nullptr);

    // Write a .bank file from a KSAudioProject
    bool writeProjectBanks(const KSAudioProject& project, const QString& outputDir,
                           const QString& assetsDir = QString());

    // Write a single .bank file from a SoundBank + audio asset directory
    bool writeBank(const SoundBank& bank,
                   const QString& assetsDir,
                   const QString& outputPath);

    // Write GUIDs.txt for the project events
    bool writeGUIDsFile(const KSAudioProject& project, const QString& outputPath);

    // Convert .ksaudio to .bank in one step
    bool convertKSAudioToBank(const QString& ksaudioPath,
                              const QString& assetsDir,
                              const QString& bankOutputPath);

    QString lastError() const { return m_lastError; }

signals:
    void writeStarted(const QString& bankName);
    void writeProgress(int percent);
    void writeCompleted(const QString& bankPath);
    void writeFailed(const QString& error);

private:
    QString m_lastError;

    // FEV2 chunk writers
    struct FEV2Header {
        quint32 magic = 0x46455632;    // "FEV2"
        quint32 version = 0x00010800;  // FMOD 1.08.x
        quint32 flags = 0;
    };

    struct EventEntry {
        QByteArray guid;           // 16 bytes
        quint32 nameIdx, pathIdx;  // string table indices
        quint32 flags = 0;
        quint32 category = 0;
        quint32 maxInstances = 0;
        quint32 length = 0;
        QVector<quint32> paramNameIdxs;
        QVector<float> paramDefaults;
    };

    struct SoundEntry {
        quint32 nameIdx;
        quint32 sampleRate = 44100;
        quint32 channels = 2;
        quint32 length = 0;     // audio data size in bytes
        quint32 format = 0;     // 0=PCM
    };

    // Build a string table from all project strings
    // Returns: map<string → index>
    QMap<QString, quint32> buildStringTable(const FSPROProject& proj,
                                            QByteArray& strTableData);

    // Write FEV2 header + all chunks
    void writeFEV2Header(QDataStream& s, const FEV2Header& hdr);
    void writeSTRTChunk(QDataStream& s, const QByteArray& strTable);
    void writeEVTSChunk(QDataStream& s, const QVector<EventEntry>& events);
    void writeBUSSEntry(QDataStream& s, const FSPROBus& bus,
                         const QMap<QString, quint32>& strMap);
    void writeVCASChunk(QDataStream& s, const QVector<FSPROVCA>& vcas,
                         const QMap<QString, quint32>& strMap);
    void writeSNDSChunk(QDataStream& s, const QVector<SoundEntry>& sounds);

    // Generate a deterministic GUID for an event name
    QByteArray makeGUID(const QString& name);

    // Read audio file data for embedding
    bool readAudioData(const QString& filePath, QByteArray& outData,
                       quint32& sampleRate, quint32& channels);
};

}} // namespace ks::audio
