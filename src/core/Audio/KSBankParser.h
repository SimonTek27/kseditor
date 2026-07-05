#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QList>

namespace ks { namespace audio {

struct BankEventInfo {
    QString  guid;
    QString  name;
    QString  path;
    quint32  flags       = 0;
    quint32  category    = 0;
    int      parameters  = 0;
    quint32  maxInstances = 0;
    quint32  length      = 0;
    QStringList parameterNames;
    QVector<float> parameterDefaults;
};

struct BankBusInfo {
    QString  name, path;
    float    volume = 1.0f;
    bool     muted  = false;
    bool     solo   = false;
    QStringList childBuses;
    QStringList linkedVCAs;
};

struct BankVCAInfo {
    QString name, path;
    float   volume = 1.0f;
};

struct BankSnapshotInfo {
    QString name, path;
};

struct BankSoundInfo {
    QString  name;
    quint32  sampleRate = 44100;
    quint32  channels   = 1;
    quint32  length     = 0;       // length in samples
    quint32  format     = 0;       // 0=PCM16, 6=Vorbis

    // Audio data extracted from FSB5
    QVector<float> samples;        // decoded PCM samples (interleaved)
    quint32  dataOffset = 0;       // offset in FSB5 data section
    quint32  dataSize   = 0;       // compressed size in bytes
    bool     hasAudioData = false;
};

struct ParsedBankData {
    bool     isValid     = false;
    bool     isEncrypted = false;
    bool     isLegacy    = false;
    quint32  version     = 0;
    quint32  size        = 0;
    QString  name;
    QString  filePath;
    QList<BankEventInfo>    events;
    QList<BankBusInfo>      buses;
    QList<BankVCAInfo>      vcas;
    QList<BankSnapshotInfo> snapshots;
    QList<BankSoundInfo>    sounds;
};

class KSBankParser : public QObject {
    Q_OBJECT
public:
    explicit KSBankParser(QObject* parent = nullptr);

    ParsedBankData parse(const QString& bankPath);
    ParsedBankData parseFromData(const QByteArray& data);

    bool       isValidBank(const QString& bankPath) const;
    bool       isEncrypted(const QString& bankPath) const;

    QStringList getEventPaths(const QString& bankPath) const;
    QStringList getEventNames(const QString& bankPath) const;
    QStringList getBusPaths(const QString& bankPath) const;
    QStringList getVCAPaths(const QString& bankPath) const;

    static QByteArray decryptData(const QByteArray& data, quint32 key);

    void clearCache() { m_cache.clear(); }

    // Extract decoded PCM audio data from bank after parsing
    bool extractAudioData(ParsedBankData& bankData) const;

    // FSB5 format constants
    enum FSB5Format : quint32 {
        FSB5_PCM8   = 0,
        FSB5_PCM16  = 1,
        FSB5_PCM24  = 2,
        FSB5_PCM32  = 3,
        FSB5_FLOAT  = 4,
        FSB5_ADPCM  = 5,
        FSB5_VORBIS = 6
    };

signals:
    void bankParsed(const QString& bankPath, const ParsedBankData& data);
    void parseError(const QString& bankPath, const QString& message);

private:
    QMap<QString, ParsedBankData> m_cache;
    QStringList m_stringTable;

    bool parseFEV2(const QByteArray& data, ParsedBankData& out);
    void readChunkStringTable(QDataStream& s, ParsedBankData& out, quint32 size);
    void readChunkEvents(QDataStream& s, ParsedBankData& out, quint32 size);
    void readChunkBuses(QDataStream& s, ParsedBankData& out, quint32 size);
    void readChunkVCAs(QDataStream& s, ParsedBankData& out, quint32 size);
    void readChunkSnapshots(QDataStream& s, ParsedBankData& out, quint32 size);
    void readChunkSounds(QDataStream& s, ParsedBankData& out, quint32 size);

    // FSB5 parsing helpers
    struct FSB5Header {
        quint32 magic;
        quint32 version;
        quint32 numSamples;
        quint32 sampleHeaderSize;
        quint32 totalDataSize;
        quint32 reserved;
    };

    struct FSB5SampleHeader {
        QString name;
        quint32 sampleRate;
        quint32 channels;
        quint32 format;
        quint32 length;        // in samples
        quint32 dataOffset;    // within sample data section
        quint32 dataSize;      // in bytes
        quint32 loopStart;
        quint32 loopEnd;
        quint32 mode;
    };

    bool parseFSB5Data(const QByteArray& bankData, ParsedBankData& out) const;
    bool readFSB5Headers(const QByteArray& fsbData, FSB5Header& hdr,
                         QVector<FSB5SampleHeader>& headers) const;
    bool decodeFSB5Samples(const QByteArray& fsbData, const FSB5Header& hdr,
                           const QVector<FSB5SampleHeader>& headers,
                           QVector<QVector<float>>& decodedSamples) const;
    bool decodePCM16(const QByteArray& src, QVector<float>& dst,
                     quint32 offset, quint32 size, quint32 channels) const;
    bool decodePCM8(const QByteArray& src, QVector<float>& dst,
                    quint32 offset, quint32 size, quint32 channels) const;

    QString stringAt(quint32 idx) const;
    static QString formatGUID(const quint8 bytes[16]);
    ParsedBankData parseFromData_legacy(const QByteArray& data);
};

}} // ks::audio
