#include "KSAudioBankParser.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

// ─── KSAudioBankParser ──────────────────────────────────────────────────

KSAudioBankParser::AudioBank KSAudioBankParser::parseBank(const QString& bankPath)
{
    AudioBank bank;
    bank.path = bankPath;
    bank.name = QFileInfo(bankPath).completeBaseName();

    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "KSAudioBankParser: Cannot open" << bankPath;
        return bank;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic;
    stream >> magic;
    if (magic != 0x4B53424B) { // "KSBK"
        qWarning() << "KSAudioBankParser: Invalid magic" << Qt::hex << magic;
        file.close();
        return bank;
    }

    stream >> bank.version;

    quint32 flags;
    stream >> flags;
    bank.isEncrypted = (flags & 1) != 0;

    stream >> bank.numSounds;
    stream >> bank.numEvents;
    stream >> bank.numGroups;

    file.close();
    return bank;
}

bool KSAudioBankParser::extractSounds(const QString& bankPath, const QString& outputDir)
{
    QDir().mkpath(outputDir);
    QVector<AudioSound> sounds = getSounds(bankPath);
    for (int i = 0; i < sounds.size(); ++i) {
        QString outPath = outputDir + "/" + sounds[i].name;
        if (outPath.isEmpty()) outPath = outputDir + QString("/sound_%1.wav").arg(i);
        if (!exportSound(sounds[i], outPath)) return false;
    }
    return true;
}

bool KSAudioBankParser::extractSound(const QString& bankPath, int soundIndex, const QString& outputPath)
{
    QVector<AudioSound> sounds = getSounds(bankPath);
    if (soundIndex < 0 || soundIndex >= sounds.size()) return false;
    return exportSound(sounds[soundIndex], outputPath);
}

QVector<KSAudioBankParser::AudioSound> KSAudioBankParser::getSounds(const QString& bankPath)
{
    QVector<AudioSound> sounds;
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) return sounds;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic;
    stream >> magic;
    if (magic != 0x4B53424B) { file.close(); return sounds; }

    quint32 version, flags;
    stream >> version;
    stream >> flags;

    quint32 numSounds, numEvents, numGroups;
    stream >> numSounds;
    stream >> numEvents;
    stream >> numGroups;

    // Skip group and event headers
    file.seek(file.pos() + numGroups * 32 + numEvents * 64);

    for (quint32 i = 0; i < numSounds; ++i) {
        AudioSound sound;
        sound.index = i;

        quint32 nameLen;
        stream >> nameLen;
        QByteArray nameBytes(nameLen, Qt::Uninitialized);
        file.read(nameBytes.data(), nameLen);
        sound.name = QString::fromUtf8(nameBytes);

        stream >> sound.length;
        stream >> sound.sampleRate;
        stream >> sound.channels;
        stream >> sound.format;
        stream >> sound.duration;
        sound.data = file.read(sound.length);

        sounds.append(sound);
    }

    file.close();
    return sounds;
}

KSAudioBankParser::AudioSound KSAudioBankParser::getSound(const QString& bankPath, int index)
{
    QVector<AudioSound> sounds = getSounds(bankPath);
    if (index >= 0 && index < sounds.size()) return sounds[index];
    return AudioSound();
}

bool KSAudioBankParser::exportSound(const AudioSound& sound, const QString& outputPath)
{
    if (sound.data.isEmpty()) return false;

    if (outputPath.endsWith(".wav", Qt::CaseInsensitive)) {
        return convertToWav(sound.data, sound.sampleRate, sound.channels, outputPath);
    }
    if (outputPath.endsWith(".ogg", Qt::CaseInsensitive)) {
        return convertToOgg(sound.data, sound.sampleRate, sound.channels, outputPath);
    }

    QFile file(outputPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(sound.data);
        file.close();
        return true;
    }
    return false;
}

QVector<KSAudioBankParser::AudioEvent> KSAudioBankParser::getEvents(const QString& bankPath)
{
    QVector<AudioEvent> events;
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) return events;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic;
    stream >> magic;
    if (magic != 0x4B53424B) { file.close(); return events; }

    quint32 version, flags, numSounds, numEvents, numGroups;
    stream >> version; stream >> flags;
    stream >> numSounds; stream >> numEvents; stream >> numGroups;

    for (quint32 i = 0; i < numEvents; ++i) {
        AudioEvent event;
        event.index = i;

        quint32 nameLen;
        stream >> nameLen;
        QByteArray nameBytes(nameLen, Qt::Uninitialized);
        file.read(nameBytes.data(), nameLen);
        event.name = QString::fromUtf8(nameBytes);

        quint32 numSounds;
        stream >> numSounds;
        for (quint32 s = 0; s < numSounds; ++s) {
            quint32 si; stream >> si;
            event.soundIndices.append(si);
        }

        quint32 numParams;
        stream >> numParams;
        for (quint32 p = 0; p < numParams; ++p) {
            quint32 keyLen; stream >> keyLen;
            QByteArray keyBytes(keyLen, Qt::Uninitialized);
            file.read(keyBytes.data(), keyLen);
            float val; stream >> val;
            event.parameters[QString::fromUtf8(keyBytes)] = val;
        }

        events.append(event);
    }

    file.close();
    return events;
}

bool KSAudioBankParser::convertToWav(const QByteArray& pcmData, quint32 sr, quint32 ch, const QString& outPath)
{
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    quint16 bps = 16;
    quint32 dataSize = pcmData.size();
    quint32 fileSize = 36 + dataSize;

    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    s.writeRawData("RIFF", 4);
    s << fileSize;
    s.writeRawData("WAVE", 4);
    s.writeRawData("fmt ", 4);
    s << (quint32)16;
    s << (quint16)1;
    s << (quint16)ch;
    s << sr;
    s << (quint32)(sr * ch * bps / 8);
    s << (quint16)(ch * bps / 8);
    s << bps;
    s.writeRawData("data", 4);
    s << dataSize;
    file.write(pcmData);
    file.close();
    return true;
}

bool KSAudioBankParser::convertToOgg(const QByteArray& pcmData, quint32 sr, quint32 ch, const QString& outPath, int quality)
{
    Q_UNUSED(quality);
    // Simple OGG wrapper — stores PCM in a minimal OGG container
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    // OGG page header
    file.write("OggS", 4);
    quint8 version = 0; file.write((const char*)&version, 1);
    quint8 type = 2; file.write((const char*)&type, 1); // bos + eos
    qint64 granule = pcmData.size() / (ch * 2);
    file.write((const char*)&granule, 8);
    quint32 serial = 1; file.write((const char*)&serial, 4);
    quint32 pageNo = 0; file.write((const char*)&pageNo, 4);
    quint32 crc = 0; file.write((const char*)&crc, 4);
    quint8 segments = 1; file.write((const char*)&segments, 1);
    quint8 segLen = qMin(255, pcmData.size() / ch);
    file.write((const char*)&segLen, 1);
    file.write(pcmData);

    file.close();
    qWarning() << "KSAudioBankParser: OGG export uses basic PCM wrapper, not true Vorbis encoding";
    return true;
}

bool KSAudioBankParser::isValidBank(const QString& bankPath)
{
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    quint32 magic;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> magic;
    file.close();
    return magic == 0x4B53424B;
}

bool KSAudioBankParser::isEncrypted(const QString& bankPath)
{
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 magic, version, flags;
    stream >> magic; stream >> version; stream >> flags;
    file.close();
    return (flags & 1) != 0;
}

QString KSAudioBankParser::getFormatName(quint32 format)
{
    switch (format) {
    case 0: return "PCM16";
    case 1: return "PCM24";
    case 2: return "PCM32";
    case 3: return "Compressed";
    default: return QString("Unknown (%1)").arg(format);
    }
}

float KSAudioBankParser::calculateDuration(quint32 sampleCount, quint32 sampleRate)
{
    if (sampleRate == 0) return 0.0f;
    return sampleCount / (float)sampleRate;
}

quint32 KSAudioBankParser::calculateSampleCount(float duration, quint32 sampleRate)
{
    return static_cast<quint32>(duration * sampleRate);
}

bool KSAudioBankParser::parseBankHeader(QDataStream& stream, AudioBank& bank)
{
    quint32 magic;
    stream >> magic;
    if (magic != 0x4B53424B) return false;
    stream >> bank.version;
    quint32 flags; stream >> flags;
    bank.isEncrypted = (flags & 1) != 0;
    stream >> bank.numSounds;
    stream >> bank.numEvents;
    stream >> bank.numGroups;
    return true;
}

bool KSAudioBankParser::parseSoundEntries(QDataStream& stream, QVector<AudioSound>& sounds)
{
    for (auto& sound : sounds) {
        quint32 nameLen;
        stream >> nameLen;
        QByteArray nameBytes(nameLen, Qt::Uninitialized);
        if (stream.device()->read(nameBytes.data(), nameLen) != nameLen) return false;
        sound.name = QString::fromUtf8(nameBytes);
        stream >> sound.length;
        stream >> sound.sampleRate;
        stream >> sound.channels;
        stream >> sound.format;
        stream >> sound.duration;
        sound.data = stream.device()->read(sound.length);
        if (static_cast<quint32>(sound.data.size()) != sound.length) return false;
    }
    return true;
}

bool KSAudioBankParser::parseEventEntries(QDataStream& stream, QVector<AudioEvent>& events)
{
    for (auto& event : events) {
        quint32 nameLen;
        stream >> nameLen;
        QByteArray nameBytes(nameLen, Qt::Uninitialized);
        if (stream.device()->read(nameBytes.data(), nameLen) != nameLen) return false;
        event.name = QString::fromUtf8(nameBytes);
        quint32 numSounds;
        stream >> numSounds;
        for (quint32 i = 0; i < numSounds; ++i) {
            quint32 si; stream >> si;
            event.soundIndices.append(si);
        }
        quint32 numParams;
        stream >> numParams;
        for (quint32 i = 0; i < numParams; ++i) {
            quint32 keyLen; stream >> keyLen;
            QByteArray keyBytes(keyLen, Qt::Uninitialized);
            if (stream.device()->read(keyBytes.data(), keyLen) != keyLen) return false;
            float val; stream >> val;
            event.parameters[QString::fromUtf8(keyBytes)] = val;
        }
    }
    return true;
}

// ─── KSAudioBankManager ─────────────────────────────────────────────────

KSAudioBankManager::KSAudioBankManager(const QString& carPath)
    : m_carPath(carPath) {}

bool KSAudioBankManager::loadBank(const QString& bankPath)
{
    m_bank = KSAudioBankParser::parseBank(bankPath);
    m_sounds = KSAudioBankParser::getSounds(bankPath);
    m_events = KSAudioBankParser::getEvents(bankPath);
    return m_bank.numSounds > 0;
}

bool KSAudioBankManager::extractAllSounds(const QString& outputDir)
{
    QDir().mkpath(outputDir);
    for (int i = 0; i < m_sounds.size(); ++i) {
        QString name = m_sounds[i].name;
        if (name.isEmpty()) name = QString("sound_%1.wav").arg(i);
        if (!KSAudioBankParser::exportSound(m_sounds[i], outputDir + "/" + name))
            return false;
    }
    return true;
}

bool KSAudioBankManager::extractSound(int index, const QString& outputPath)
{
    if (index < 0 || index >= m_sounds.size()) return false;
    return KSAudioBankParser::exportSound(m_sounds[index], outputPath);
}

float KSAudioBankManager::getTotalDuration() const
{
    float total = 0.0f;
    for (const auto& s : m_sounds)
        total += s.duration;
    return total;
}

quint64 KSAudioBankManager::getTotalSize() const
{
    quint64 total = 0;
    for (const auto& s : m_sounds)
        total += s.length;
    return total;
}
