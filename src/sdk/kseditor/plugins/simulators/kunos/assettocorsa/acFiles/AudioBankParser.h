#pragma once

#include <QString>
#include <QVector>
#include <QMap>

/**
 * @brief Audio Bank Parser for Assetto Corsa
 *
 * Parses sound banks used by AC.
 * Based on:
 * - AC sound documentation
 * - Audio bank format specification
 *
 * Features:
 * - Bank parsing
 * - Sound sample extraction
 * - Audio format conversion
 * - Sound bank metadata
 */
class KSAudioBankParser {
public:
    struct AudioBank {
        QString name;
        QString path;
        quint32 version = 0;
        quint32 numSounds = 0;
        quint32 numEvents = 0;
        quint32 numGroups = 0;
        bool isEncrypted = false;
    };

    struct AudioSound {
        QString name;
        quint32 index = 0;
        quint32 length = 0;           // bytes
        float duration = 0.0f;        // seconds
        quint32 sampleRate = 44100;
        quint32 channels = 2;         // 1=mono, 2=stereo
        quint32 format = 0;           // 0=PCM16, 1=PCM24, 2=PCM32, 3=Compressed
        QByteArray data;
    };

    struct AudioEvent {
        QString name;
        quint32 index = 0;
        QVector<quint32> soundIndices;
        QMap<QString, float> parameters;
    };

    // Bank operations
    static AudioBank parseBank(const QString& bankPath);
    static bool extractSounds(const QString& bankPath, const QString& outputDir);
    static bool extractSound(const QString& bankPath, int soundIndex, const QString& outputPath);

    // Sound operations
    static QVector<AudioSound> getSounds(const QString& bankPath);
    static AudioSound getSound(const QString& bankPath, int index);
    static bool exportSound(const AudioSound& sound, const QString& outputPath);

    // Event operations
    static QVector<AudioEvent> getEvents(const QString& bankPath);

    // Format conversion
    static bool convertToWav(const QByteArray& pcmData, quint32 sampleRate, quint32 channels,
                              const QString& outputPath);
    static bool convertToOgg(const QByteArray& pcmData, quint32 sampleRate, quint32 channels,
                              const QString& outputPath, int quality = 6);

    // Validation
    static bool isValidBank(const QString& bankPath);
    static bool isEncrypted(const QString& bankPath);

    // Utility
    static QString getFormatName(quint32 format);
    static float calculateDuration(quint32 sampleCount, quint32 sampleRate);
    static quint32 calculateSampleCount(float duration, quint32 sampleRate);

private:
    static bool parseBankHeader(QDataStream& stream, AudioBank& bank);
    static bool parseSoundEntries(QDataStream& stream, QVector<AudioSound>& sounds);
    static bool parseEventEntries(QDataStream& stream, QVector<AudioEvent>& events);
};

/**
 * @brief Audio Bank Manager - High-level interface
 */
class KSAudioBankManager {
public:
    explicit KSAudioBankManager(const QString& carPath);

    // Operations
    bool loadBank(const QString& bankPath);
    bool extractAllSounds(const QString& outputDir);
    bool extractSound(int index, const QString& outputPath);

    // Access
    QVector<KSAudioBankParser::AudioSound> getSounds() const { return m_sounds; }
    QVector<KSAudioBankParser::AudioEvent> getEvents() const { return m_events; }

    // Analysis
    int getSoundCount() const { return m_sounds.size(); }
    float getTotalDuration() const;
    quint64 getTotalSize() const;

private:
    QString m_carPath;
    KSAudioBankParser::AudioBank m_bank;
    QVector<KSAudioBankParser::AudioSound> m_sounds;
    QVector<KSAudioBankParser::AudioEvent> m_events;
};
