#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QIODevice>

namespace ks {

struct AudioMetadata {
    int sampleRate = 44100;
    int channels = 2;
    int bitsPerSample = 16;
    int durationMs = 0;
    QString format;
    QString codec;
};

struct AudioChunk {
    QByteArray data;
    int startMs = 0;
    int endMs = 0;
};

class AudioFile {
public:
    static bool loadAudio(const QString& path, QByteArray& outData, AudioMetadata& metadata);
    static bool saveAudio(const QString& path, const QByteArray& data, const AudioMetadata& metadata);

    static bool exportAudio(const QString& path, const QByteArray& data, const AudioMetadata& metadata, const QString& format);
    static bool importAudio(const QString& path, QByteArray& outData, AudioMetadata& metadata);

    static QVector<AudioChunk> splitIntoChunks(const QByteArray& data, int chunkDurationMs);
    static QByteArray mergeChunks(const QVector<AudioChunk>& chunks);

    static bool convertFormat(const QString& inputPath, const QString& outputPath, const AudioMetadata& targetFormat);

    static QStringList supportedFormats();
    static bool isFormatSupported(const QString& extension);

private:
    static bool detectFormat(const QByteArray& header, QString& format);
    static bool encodeWav(const QByteArray& data, QIODevice* output, const AudioMetadata& metadata);
    static bool decodeWav(QIODevice* input, QByteArray& data, AudioMetadata& metadata);
};

} // namespace ks