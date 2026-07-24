#include "BankParser.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>
#include <QtEndian>
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

// FMOD Studio .bank file format constants (FMOD 1.08.x)
// A .bank file starts with a FEV2 header block.
// The raw sample data blocks within the bank use FSB5 sub-headers.
static constexpr quint32 FMOD_BANK_MAGIC_FEV2 = 0x46455632; // "FEV2"
static constexpr quint32 FMOD_BANK_MAGIC_RIFF = 0x52494646; // "RIFF" (legacy FEV)
static constexpr quint32 FMOD_FSB5_MAGIC      = 0x46534235; // "FSB5"

// XOR decryption key for FMOD banks (common key used by many AC banks)
static constexpr quint32 FMOD_DECRYPT_KEY = 0xAE12B3F4;

namespace ks {
namespace audio {

KSBankParser::KSBankParser(QObject* parent)
    : QObject(parent) {}

ParsedBankData KSBankParser::parse(const QString& bankPath) {
    if (m_cache.contains(bankPath))
        return m_cache[bankPath];

    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit parseError(bankPath, "Cannot open file: " + bankPath);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    ParsedBankData result = parseFromData(data);
    result.filePath = bankPath;
    result.name     = QFileInfo(bankPath).completeBaseName();
    if (result.isValid) {
        m_cache[bankPath] = result;
        emit bankParsed(bankPath, result);
    }
    return result;
}

ParsedBankData KSBankParser::parseFromData(const QByteArray& rawData) {
    ParsedBankData result;
    result.size = rawData.size();

    if (rawData.size() < 8) {
        emit parseError("", "Data too small for bank header");
        return result;
    }

    // Peek magic (little-endian)
    quint32 magic = 0;
    memcpy(&magic, rawData.constData(), 4);

    if (magic == FMOD_BANK_MAGIC_FEV2) {
        // Check encryption flag before parsing
        if (rawData.size() >= 12) {
            quint32 flags = 0;
            memcpy(&flags, rawData.constData() + 8, 4);
            result.isEncrypted = (flags & 0x1) != 0;
        }

        QByteArray workData = rawData;
        if (result.isEncrypted) {
            workData = decryptData(rawData, FMOD_DECRYPT_KEY);
        }

        result.isValid = parseFEV2(workData, result);

        // Extract audio data after metadata parsing
        if (result.isValid) {
            // FSB5 data may be embedded after the FEV2 metadata
            // or may need decryption first
            QByteArray audioData = result.isEncrypted ? workData : rawData;
            parseFSB5Data(audioData, result);
        }
    } else if (magic == FMOD_BANK_MAGIC_RIFF) {
        // Legacy FEV — treat as valid but not fully parsed
        result.isValid   = true;
        result.isLegacy  = true;
        result.version   = 0;
        qWarning() << "KSBankParser: Legacy RIFF/FEV bank — limited parsing";
    } else {
        emit parseError("", QString("Unknown bank magic 0x%1 — not a FMOD bank")
                        .arg(magic, 8, 16, QChar('0')));
    }
    return result;
}

// ============================================================================
// FEV2 parser — FMOD Studio 1.08 format
// ============================================================================
bool KSBankParser::parseFEV2(const QByteArray& data, ParsedBankData& out) {
    QDataStream s(data);
    s.setByteOrder(QDataStream::LittleEndian);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // Header
    quint32 magic;    s >> magic;   // FEV2
    quint32 version;  s >> version; // e.g. 0x00010800 for 1.08.x
    out.version = version;

    quint32 flags; s >> flags;
    out.isEncrypted = (flags & 0x1) != 0;

    // FMOD 1.08 bank has a variable-length chunk-based layout.
    // We read chunk headers until EOF.
    while (!s.atEnd()) {
        if (s.device()->pos() + 8 > data.size()) break;

        quint32 chunkTag;   s >> chunkTag;
        quint32 chunkSize;  s >> chunkSize;

        qint64 chunkStart = s.device()->pos();
        qint64 chunkEnd   = chunkStart + chunkSize;
        if (chunkEnd > data.size()) break;

        switch (chunkTag) {
        case 0x53545254: // "STRT" — string table
            readChunkStringTable(s, out, chunkSize);
            break;
        case 0x45565453: // "EVTS" — event list
            readChunkEvents(s, out, chunkSize);
            break;
        case 0x42555353: // "BUSS" — buses
            readChunkBuses(s, out, chunkSize);
            break;
        case 0x56434153: // "VCAS" — VCAs
            readChunkVCAs(s, out, chunkSize);
            break;
        case 0x534E4150: // "SNAP" — snapshots
            readChunkSnapshots(s, out, chunkSize);
            break;
        case 0x534E4453: // "SNDS" — sound assets
            readChunkSounds(s, out, chunkSize);
            break;
        default:
            break; // unknown chunk — skip below
        }

        // Always seek to end of chunk
        s.device()->seek(chunkEnd);
    }

    return true;
}

void KSBankParser::readChunkStringTable(QDataStream& s, ParsedBankData& out, quint32 size) {
    quint32 count; s >> count;
    m_stringTable.clear();
    m_stringTable.reserve(count);
    for (quint32 i = 0; i < count; ++i) {
        quint32 len; s >> len;
        QByteArray bytes(len, '\0');
        s.readRawData(bytes.data(), len);
        m_stringTable.append(QString::fromUtf8(bytes));
    }
}

void KSBankParser::readChunkEvents(QDataStream& s, ParsedBankData& out, quint32 /*size*/) {
    quint32 count; s >> count;
    for (quint32 i = 0; i < count; ++i) {
        BankEventInfo ev;

        // GUID (16 bytes)
        quint8 guidBytes[16]; s.readRawData(reinterpret_cast<char*>(guidBytes), 16);
        ev.guid = formatGUID(guidBytes);

        quint32 nameIdx, pathIdx;
        s >> nameIdx >> pathIdx;
        ev.name = stringAt(nameIdx);
        ev.path = stringAt(pathIdx);
        if (!ev.path.startsWith("event:/"))
            ev.path = "event:/" + ev.path;

        s >> ev.flags;
        s >> ev.category;
        s >> ev.maxInstances;
        s >> ev.length;

        quint32 paramCount; s >> paramCount;
        ev.parameters = (int)paramCount;
        for (quint32 p = 0; p < paramCount; ++p) {
            quint32 pNameIdx; s >> pNameIdx;
            float pDefault;   s >> pDefault;
            ev.parameterNames.append(stringAt(pNameIdx));
            ev.parameterDefaults.append(pDefault);
        }
        out.events.append(ev);
    }
}

void KSBankParser::readChunkBuses(QDataStream& s, ParsedBankData& out, quint32 /*size*/) {
    quint32 count; s >> count;
    for (quint32 i = 0; i < count; ++i) {
        BankBusInfo bus;
        quint32 nameIdx, pathIdx; s >> nameIdx >> pathIdx;
        bus.name = stringAt(nameIdx);
        bus.path = stringAt(pathIdx);
        if (!bus.path.startsWith("bus:/"))
            bus.path = "bus:/" + bus.path;
        s >> bus.volume;
        quint8 flags; s >> flags;
        bus.muted = (flags & 0x01) != 0;
        bus.solo  = (flags & 0x02) != 0;
        quint32 childCount; s >> childCount;
        for (quint32 c = 0; c < childCount; ++c) {
            quint32 idx; s >> idx;
            bus.childBuses.append(stringAt(idx));
        }
        quint32 vcaCount; s >> vcaCount;
        for (quint32 v = 0; v < vcaCount; ++v) {
            quint32 idx; s >> idx;
            bus.linkedVCAs.append(stringAt(idx));
        }
        out.buses.append(bus);
    }
}

void KSBankParser::readChunkVCAs(QDataStream& s, ParsedBankData& out, quint32 /*size*/) {
    quint32 count; s >> count;
    for (quint32 i = 0; i < count; ++i) {
        BankVCAInfo vca;
        quint32 nameIdx, pathIdx; s >> nameIdx >> pathIdx;
        vca.name = stringAt(nameIdx);
        vca.path = stringAt(pathIdx);
        if (!vca.path.startsWith("vca:/"))
            vca.path = "vca:/" + vca.path;
        s >> vca.volume;
        out.vcas.append(vca);
    }
}

void KSBankParser::readChunkSnapshots(QDataStream& s, ParsedBankData& out, quint32 /*size*/) {
    quint32 count; s >> count;
    for (quint32 i = 0; i < count; ++i) {
        BankSnapshotInfo snap;
        quint32 nameIdx, pathIdx; s >> nameIdx >> pathIdx;
        snap.name = stringAt(nameIdx);
        snap.path = stringAt(pathIdx);
        if (!snap.path.startsWith("snapshot:/"))
            snap.path = "snapshot:/" + snap.path;
        out.snapshots.append(snap);
    }
}

void KSBankParser::readChunkSounds(QDataStream& s, ParsedBankData& out, quint32 /*size*/) {
    quint32 count; s >> count;
    for (quint32 i = 0; i < count; ++i) {
        BankSoundInfo snd;
        quint32 nameIdx; s >> nameIdx;
        snd.name = stringAt(nameIdx);
        s >> snd.sampleRate >> snd.channels >> snd.length >> snd.format;
        out.sounds.append(snd);
    }
}

// ============================================================================
// Helpers
// ============================================================================
QString KSBankParser::stringAt(quint32 idx) const {
    if (idx < (quint32)m_stringTable.size())
        return m_stringTable.at(idx);
    return QString("str_%1").arg(idx);
}

QString KSBankParser::formatGUID(const quint8 bytes[16]) {
    return QString("{%1%2%3%4-%5%6-%7%8-%9%10-%11%12%13%14%15%16}")
        .arg(bytes[0],2,16,QChar('0')).arg(bytes[1],2,16,QChar('0'))
        .arg(bytes[2],2,16,QChar('0')).arg(bytes[3],2,16,QChar('0'))
        .arg(bytes[4],2,16,QChar('0')).arg(bytes[5],2,16,QChar('0'))
        .arg(bytes[6],2,16,QChar('0')).arg(bytes[7],2,16,QChar('0'))
        .arg(bytes[8],2,16,QChar('0')).arg(bytes[9],2,16,QChar('0'))
        .arg(bytes[10],2,16,QChar('0')).arg(bytes[11],2,16,QChar('0'))
        .arg(bytes[12],2,16,QChar('0')).arg(bytes[13],2,16,QChar('0'))
        .arg(bytes[14],2,16,QChar('0')).arg(bytes[15],2,16,QChar('0'));
}

bool KSBankParser::isValidBank(const QString& bankPath) const {
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 8) return false;
    quint32 magic = 0;
    file.read(reinterpret_cast<char*>(&magic), 4);
    // Accept FEV2 or legacy RIFF
    return magic == FMOD_BANK_MAGIC_FEV2 || magic == FMOD_BANK_MAGIC_RIFF;
}

bool KSBankParser::isEncrypted(const QString& bankPath) const {
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 12) return false;
    file.seek(8);
    quint32 flags = 0;
    file.read(reinterpret_cast<char*>(&flags), 4);
    return (flags & 0x1) != 0;
}

QStringList KSBankParser::getEventPaths(const QString& bankPath) const {
    const_cast<KSBankParser*>(this)->parse(bankPath);
    QStringList paths;
    if (m_cache.contains(bankPath))
        for (const auto& e : m_cache[bankPath].events) paths << e.path;
    return paths;
}

QStringList KSBankParser::getEventNames(const QString& bankPath) const {
    const_cast<KSBankParser*>(this)->parse(bankPath);
    QStringList names;
    if (m_cache.contains(bankPath))
        for (const auto& e : m_cache[bankPath].events) names << e.name;
    return names;
}

QStringList KSBankParser::getBusPaths(const QString& bankPath) const {
    const_cast<KSBankParser*>(this)->parse(bankPath);
    QStringList paths;
    if (m_cache.contains(bankPath))
        for (const auto& b : m_cache[bankPath].buses) paths << b.path;
    return paths;
}

QStringList KSBankParser::getVCAPaths(const QString& bankPath) const {
    const_cast<KSBankParser*>(this)->parse(bankPath);
    QStringList paths;
    if (m_cache.contains(bankPath))
        for (const auto& v : m_cache[bankPath].vcas) paths << v.path;
    return paths;
}

QByteArray KSBankParser::decryptData(const QByteArray& data, quint32 key) {
    QByteArray decrypted = data;
    quint32* words = reinterpret_cast<quint32*>(decrypted.data());
    int wordCount = decrypted.size() / 4;
    quint32 prev = 0;
    for (int i = 0; i < wordCount; ++i) {
        quint32 cur = words[i];
        words[i] ^= key ^ prev;
        prev = cur;
    }
    return decrypted;
}

// ============================================================================
// FSB5 parsing — extract audio data from FMOD Sound Bank 5 containers
// ============================================================================
bool KSBankParser::extractAudioData(ParsedBankData& bankData) const
{
    // Uses the raw bank data; for already-parsed data, re-parse is needed
    return false;
}

bool KSBankParser::parseFSB5Data(const QByteArray& bankData, ParsedBankData& out) const
{
    // FSB5 data starts after the FEV2 header. Look for FSB5 magic anywhere
    // in the file. Often it's after all metadata chunks.
    for (int i = 0; i < bankData.size() - 4; ++i) {
        quint32 magic = 0;
        memcpy(&magic, bankData.constData() + i, 4);
        if (magic == FMOD_FSB5_MAGIC) {
            // Found FSB5 section
            QByteArray fsbChunk = bankData.mid(i);

            FSB5Header hdr;
            QVector<FSB5SampleHeader> headers;
            if (!readFSB5Headers(fsbChunk, hdr, headers))
                return false;

            if (hdr.numSamples == 0 || headers.isEmpty())
                return true; // No audio data - not an error

            // Decode samples for each sound
            QVector<QVector<float>> decodedSamples;
            if (!decodeFSB5Samples(fsbChunk, hdr, headers, decodedSamples))
                return false;

            // Map decoded samples to sounds by name
            for (int si = 0; si < headers.size() && si < decodedSamples.size(); ++si) {
                for (auto& snd : out.sounds) {
                    if (snd.name == headers[si].name) {
                        snd.samples = decodedSamples[si];
                        snd.hasAudioData = true;
                        snd.channels = headers[si].channels;
                        snd.sampleRate = headers[si].sampleRate;
                        snd.length = headers[si].length;
                        break;
                    }
                }
            }

            return true;
        }
    }
    return false; // No FSB5 section found
}

bool KSBankParser::readFSB5Headers(const QByteArray& fsbData, FSB5Header& hdr,
                                   QVector<FSB5SampleHeader>& headers) const
{
    if (fsbData.size() < 30) return false;

    QDataStream s(fsbData);
    s.setByteOrder(QDataStream::LittleEndian);

    s >> hdr.magic;
    s >> hdr.version;
    s >> hdr.numSamples;
    s >> hdr.sampleHeaderSize;
    s >> hdr.totalDataSize;
    s >> hdr.reserved;

    if (hdr.magic != FMOD_FSB5_MAGIC || hdr.numSamples == 0)
        return false;

    quint32 headerStart = 24; // FSB5 header size (6 uint32)
    if (hdr.version >= 1) {
        // Version >= 1 has additional hash/difficulty fields
        headerStart = 30;
        s.skipRawData(6); // hash + difficulty
    }

    // Read sample headers
    s.device()->seek(headerStart);
    for (quint32 i = 0; i < hdr.numSamples; ++i) {
        FSB5SampleHeader sh;

        // Name: null-terminated string, padded to 4-byte boundary
        QByteArray nameBytes;
        char c;
        do {
            if (s.atEnd()) return false;
            s.readRawData(&c, 1);
            if (c != '\0') nameBytes.append(c);
        } while (c != '\0');
        sh.name = QString::fromUtf8(nameBytes);

        // Pad to 4-byte boundary
        qint64 nameEnd = s.device()->pos();
        qint64 nameStart = nameEnd - nameBytes.size() - 1;
        qint64 padding = (4 - (nameEnd - headerStart) % 4) % 4;
        s.skipRawData(static_cast<int>(padding));

        s >> sh.sampleRate;
        s >> sh.channels;
        s >> sh.format;
        s >> sh.length;
        s >> sh.dataOffset;
        s >> sh.dataSize;
        s >> sh.loopStart;
        s >> sh.loopEnd;
        s >> sh.mode;
        s.skipRawData(4); // variable bits

        headers.append(sh);
    }

    return true;
}

bool KSBankParser::decodeFSB5Samples(const QByteArray& fsbData, const FSB5Header& hdr,
                                     const QVector<FSB5SampleHeader>& headers,
                                     QVector<QVector<float>>& decodedSamples) const
{
    // Calculate where sample data starts
    quint32 sampleDataStart = 0;
    if (hdr.version >= 1)
        sampleDataStart = 30;
    else
        sampleDataStart = 24;

    // Add all sample headers to find data section start
    for (const auto& sh : headers) {
        sampleDataStart += sh.dataOffset; // Actually, use the data offset from header
    }
    // Sample data is at headerStart + numSamples * sampleHeaderSize
    // But we can use the dataOffset from each sample header
    sampleDataStart = 0;
    if (hdr.version >= 1)
        sampleDataStart = 30;
    else
        sampleDataStart = 24;

    // Calculate total header size
    quint32 totalHeaderSize = sampleDataStart;
    for (const auto& sh : headers) {
        // Name length with padding
        quint32 nameLen = (sh.name.toUtf8().size() + 1 + 3) & ~3;
        totalHeaderSize += nameLen + 44; // fixed fields after name
    }

    decodedSamples.clear();
    decodedSamples.resize(headers.size());

    for (int i = 0; i < headers.size(); ++i) {
        const auto& sh = headers[i];
        quint32 dataStart = totalHeaderSize + sh.dataOffset;
        quint32 dataSize = sh.dataSize;

        if (dataStart + dataSize > (quint32)fsbData.size())
            continue;

        QVector<float> samples;

        switch (sh.format) {
        case FSB5_PCM16:
            decodePCM16(fsbData, samples, dataStart, dataSize, sh.channels);
            break;
        case FSB5_PCM8:
            decodePCM8(fsbData, samples, dataStart, dataSize, sh.channels);
            break;
        case FSB5_VORBIS: {
            const unsigned char* vorbisData =
                reinterpret_cast<const unsigned char*>(fsbData.constData() + dataStart);
            int vorbisLen = static_cast<int>(dataSize);
            int error = 0;
            stb_vorbis* v = stb_vorbis_open_memory(vorbisData, vorbisLen, &error, nullptr);
            if (!v) {
                qWarning() << "FSB5: stb_vorbis failed to open" << sh.name
                           << "(error" << error << ") - using placeholder";
                samples.resize(sh.length * sh.channels, 0.0f);
                break;
            }
            stb_vorbis_info info = stb_vorbis_get_info(v);
            unsigned int totalFrames = stb_vorbis_stream_length_in_samples(v);
            int numChannels = info.channels;
            int numFloats = static_cast<int>(totalFrames) * numChannels;
            if (numFloats <= 0) {
                stb_vorbis_close(v);
                samples.resize(sh.length * sh.channels, 0.0f);
                break;
            }
            samples.resize(numFloats);
            int decoded = stb_vorbis_get_samples_float_interleaved(v, numChannels,
                samples.data(), numFloats);
            stb_vorbis_close(v);
            if (decoded > 0 && decoded * numChannels < numFloats)
                samples.resize(decoded * numChannels);
            break;
        }
        default:
            qWarning() << "FSB5: Unsupported format" << sh.format
                       << "for" << sh.name;
            samples.resize(sh.length * sh.channels, 0.0f);
            break;
        }

        decodedSamples[i] = samples;
    }

    return true;
}

bool KSBankParser::decodePCM16(const QByteArray& src, QVector<float>& dst,
                               quint32 offset, quint32 size, quint32 channels) const
{
    Q_UNUSED(channels);
    int sampleCount = size / 2;
    dst.resize(sampleCount);

    const qint16* pcmData = reinterpret_cast<const qint16*>(src.constData() + offset);
    for (int i = 0; i < sampleCount; ++i) {
        dst[i] = qBound(-1.0f, pcmData[i] / 32768.0f, 1.0f);
    }
    return true;
}

bool KSBankParser::decodePCM8(const QByteArray& src, QVector<float>& dst,
                              quint32 offset, quint32 size, quint32 channels) const
{
    Q_UNUSED(channels);
    dst.resize(size);

    const quint8* pcmData = reinterpret_cast<const quint8*>(src.constData() + offset);
    for (quint32 i = 0; i < size; ++i) {
        dst[i] = qBound(-1.0f, (pcmData[i] / 128.0f) - 1.0f, 1.0f);
    }
    return true;
}

// Legacy helpers (kept for API compatibility)
ParsedBankData KSBankParser::parseFromData_legacy(const QByteArray& data) {
    return parseFromData(data);
}

} // namespace audio
} // namespace ks
