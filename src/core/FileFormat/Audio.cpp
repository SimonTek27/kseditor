#include "core/FileFormat/Audio.h"
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QDataStream>
#include <cmath>

namespace ks {

static const QStringList s_supportedFormats = {"wav", "ogg", "flac"};

bool AudioFile::loadAudio(const QString& path, QByteArray& outData, AudioMetadata& metadata)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QString fmt;
    if (!detectFormat(data.left(16), fmt)) return false;

    if (fmt == "wav") {
        QBuffer buf(&data);
        buf.open(QIODevice::ReadOnly);
        return decodeWav(&buf, outData, metadata);
    }

    // For unsupported raw formats, just pass through
    outData = data;
    metadata.format = fmt;
    metadata.sampleRate = 44100;
    metadata.channels = 2;
    metadata.bitsPerSample = 16;
    metadata.durationMs = 0;
    return true;
}

bool AudioFile::saveAudio(const QString& path, const QByteArray& data, const AudioMetadata& metadata)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    bool ok = encodeWav(data, &file, metadata);
    file.close();
    return ok;
}

bool AudioFile::exportAudio(const QString& path, const QByteArray& data,
                            const AudioMetadata& metadata, const QString& format)
{
    Q_UNUSED(data);
    Q_UNUSED(metadata);
    Q_UNUSED(format);
    // For now, same as saveAudio (WAV output)
    return saveAudio(path, data, metadata);
}

bool AudioFile::importAudio(const QString& path, QByteArray& outData, AudioMetadata& metadata)
{
    return loadAudio(path, outData, metadata);
}

QVector<AudioChunk> AudioFile::splitIntoChunks(const QByteArray& data, int chunkDurationMs)
{
    QVector<AudioChunk> chunks;
    if (data.isEmpty()) return chunks;

    // Assume 44100 Hz, 16-bit stereo -> bytes per ms = 44100 * 2 * 2 / 1000 = 176.4
    int bytesPerSec = 44100 * 2 * 2; // 16-bit stereo
    int chunkSize = qMax(1, bytesPerSec * chunkDurationMs / 1000);

    int pos = 0;
    int startMs = 0;
    while (pos < data.size()) {
        int sz = qMin(chunkSize, data.size() - pos);
        AudioChunk chunk;
        chunk.data = data.mid(pos, sz);
        chunk.startMs = startMs;
        chunk.endMs = startMs + chunkDurationMs;
        chunks.append(chunk);
        pos += sz;
        startMs += chunkDurationMs;
    }
    return chunks;
}

QByteArray AudioFile::mergeChunks(const QVector<AudioChunk>& chunks)
{
    QByteArray merged;
    for (const auto& c : chunks)
        merged.append(c.data);
    return merged;
}

bool AudioFile::convertFormat(const QString& inputPath, const QString& outputPath,
                              const AudioMetadata& targetFormat)
{
    QByteArray data;
    AudioMetadata meta;
    if (!loadAudio(inputPath, data, meta)) return false;
    return saveAudio(outputPath, data, targetFormat);
}

QStringList AudioFile::supportedFormats()
{
    return s_supportedFormats;
}

bool AudioFile::isFormatSupported(const QString& extension)
{
    return s_supportedFormats.contains(extension.toLower());
}

bool AudioFile::detectFormat(const QByteArray& header, QString& format)
{
    if (header.size() < 4) return false;

    // RIFF/WAV
    if (header.startsWith("RIFF") && header.mid(8, 4) == "WAVE") {
        format = "wav";
        return true;
    }
    // Ogg
    if (header.startsWith("OggS")) {
        format = "ogg";
        return true;
    }
    // FLAC
    if (header.startsWith("fLaC")) {
        format = "flac";
        return true;
    }
    return false;
}

bool AudioFile::encodeWav(const QByteArray& data, QIODevice* output, const AudioMetadata& metadata)
{
    if (!output->isWritable()) return false;

    int bytesPerSample = metadata.bitsPerSample / 8;
    int blockAlign = metadata.channels * bytesPerSample;
    int byteRate = metadata.sampleRate * blockAlign;
    int dataSize = data.size();
    int fileSize = 36 + dataSize;

    QDataStream out(output);
    out.setByteOrder(QDataStream::LittleEndian);

    // RIFF header
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(fileSize);
    out.writeRawData("WAVE", 4);

    // fmt chunk
    out.writeRawData("fmt ", 4);
    out << static_cast<quint32>(16);                             // chunk size
    out << static_cast<quint16>(1);                              // PCM
    out << static_cast<quint16>(metadata.channels);
    out << static_cast<quint32>(metadata.sampleRate);
    out << static_cast<quint32>(byteRate);
    out << static_cast<quint16>(blockAlign);
    out << static_cast<quint16>(metadata.bitsPerSample);

    // data chunk
    out.writeRawData("data", 4);
    out << static_cast<quint32>(dataSize);
    out.writeRawData(data.constData(), dataSize);

    return true;
}

bool AudioFile::decodeWav(QIODevice* input, QByteArray& data, AudioMetadata& metadata)
{
    if (!input->isReadable()) return false;

    QDataStream in(input);
    in.setByteOrder(QDataStream::LittleEndian);

    char riff[4];
    in.readRawData(riff, 4);
    if (QByteArray(riff, 4) != "RIFF") return false;

    quint32 fileSize;
    in >> fileSize;
    Q_UNUSED(fileSize);

    char wave[4];
    in.readRawData(wave, 4);
    if (QByteArray(wave, 4) != "WAVE") return false;

    // Search for fmt and data chunks
    metadata.sampleRate = 44100;
    metadata.channels = 2;
    metadata.bitsPerSample = 16;

    while (true) {
        char chunkId[4];
        if (in.readRawData(chunkId, 4) != 4) break;
        quint32 chunkSize;
        in >> chunkSize;

        QByteArray id(chunkId, 4);
        if (id == "fmt ") {
            quint16 audioFmt, numChannels, bitsPerSample;
            quint32 sampleRate;
            in >> audioFmt >> numChannels >> sampleRate;
            metadata.channels = numChannels;
            metadata.sampleRate = sampleRate;

            in.skipRawData(6); // byteRate + blockAlign
            in >> bitsPerSample;
            metadata.bitsPerSample = bitsPerSample;

            // Skip remaining fmt chunk if any
            int remaining = chunkSize - 16;
            if (remaining > 0) in.skipRawData(remaining);
        } else if (id == "data") {
            data.resize(chunkSize);
            in.readRawData(data.data(), chunkSize);
            metadata.durationMs = static_cast<int>(
                1000.0 * chunkSize / (metadata.sampleRate * metadata.channels * (metadata.bitsPerSample / 8)));
            break;
        } else {
            in.skipRawData(chunkSize);
        }
    }

    metadata.format = "wav";
    return !data.isEmpty();
}

} // namespace ks
