#include "Audio/BankParserBridge.h"
#include <filesystem>

#if HAS_KSNET
#include "plugins/simulators/kunos/assettocorsa/acFiles/AudioBankParser.h"
#endif

namespace ks::audio {

ParsedBank parseBankFile(const std::string& bankPath)
{
    ParsedBank result;
    result.filePath = bankPath;
    result.name = std::filesystem::path(bankPath).stem().string();

#if HAS_KSNET
    QString qPath = QString::fromStdString(bankPath);

    auto bank = KSAudioBankParser::parseBank(qPath);
    result.valid = !bank.name.isEmpty();
    result.encrypted = bank.isEncrypted;
    result.version = static_cast<int>(bank.version);
    result.name = bank.name.toStdString();
    result.eventCount = static_cast<int>(bank.numEvents);
    result.soundCount = static_cast<int>(bank.numSounds);

    auto events = KSAudioBankParser::getEvents(qPath);
    result.events.reserve(events.size());
    for (const auto& ev : events) {
        BankEventMeta meta;
        meta.name = ev.name.toStdString();
        meta.parameterCount = ev.parameters.size();
        for (auto it = ev.parameters.constBegin(); it != ev.parameters.constEnd(); ++it) {
            meta.parameterNames.push_back(it.key().toStdString());
            meta.parameterDefaults.push_back(it.value());
        }
        result.events.push_back(std::move(meta));
    }

    auto sounds = KSAudioBankParser::getSounds(qPath);
    result.sounds.reserve(sounds.size());
    for (const auto& snd : sounds) {
        BankSoundMeta meta;
        meta.name = snd.name.toStdString();
        meta.sampleRate = static_cast<int>(snd.sampleRate);
        meta.channels = static_cast<int>(snd.channels);
        meta.length = static_cast<int>(snd.length);
        meta.hasAudioData = !snd.data.isEmpty();
        result.sounds.push_back(std::move(meta));
    }
#else
    result.valid = false;
#endif

    return result;
}

void extractBankAudio(ParsedBank& bank)
{
#if HAS_KSNET
    if (!bank.valid) return;
    for (auto& snd : bank.sounds) {
        if (snd.hasAudioData && snd.samples.empty()) {
            snd.hasAudioData = false;
        }
    }
#else
    (void)bank;
#endif
}

} // namespace ks::audio
