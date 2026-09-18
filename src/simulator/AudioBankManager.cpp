#include "AudioBankManager.h"
#include "Audio/BankParserBridge.h"
#include <cstdio>
#include <filesystem>

namespace ks::sim {

bool AudioBankManager::loadBank(const std::string& carDirectory, const std::string& carId)
{
    unloadBank();

    std::string bankPath = carDirectory + "/sfx/" + carId + ".bank";
    if (!std::filesystem::exists(bankPath)) {
        printf("AudioBankManager: Bank not found: %s\n", bankPath.c_str());
        return false;
    }

    printf("AudioBankManager: Loading bank: %s\n", bankPath.c_str());

    auto parsed = ks::audio::parseBankFile(bankPath);
    if (!parsed.valid) {
        printf("AudioBankManager: Failed to parse bank\n");
        return false;
    }

    m_data.carId = carId;
    m_data.bankPath = bankPath;
    m_data.events = std::move(parsed.events);
    m_data.sounds = std::move(parsed.sounds);
    m_data.eventIndex.clear();

    for (int i = 0; i < static_cast<int>(m_data.events.size()); ++i)
        m_data.eventIndex[m_data.events[i].name] = i;

    m_data.loaded = true;

    logBankInfo();

    int audioSounds = 0;
    for (const auto& s : m_data.sounds)
        if (s.hasAudioData) audioSounds++;

    if (audioSounds > 0)
        printf("AudioBankManager: %d sounds with audio data\n", audioSounds);

    return true;
}

void AudioBankManager::unloadBank()
{
    m_data.events.clear();
    m_data.sounds.clear();
    m_data.eventIndex.clear();
    m_data.loaded = false;
}

const ks::audio::BankEventMeta* AudioBankManager::findEvent(const std::string& name) const
{
    if (!m_data.loaded) return nullptr;
    auto it = m_data.eventIndex.find(name);
    if (it == m_data.eventIndex.end()) return nullptr;
    return &m_data.events[it->second];
}

bool AudioBankManager::getEventParameter(const std::string& eventName,
                                         const std::string& paramName,
                                         float& outValue) const
{
    auto* ev = findEvent(eventName);
    if (!ev) return false;

    for (int i = 0; i < ev->parameterCount && i < static_cast<int>(ev->parameterNames.size()); ++i) {
        if (ev->parameterNames[i] == paramName) {
            outValue = (i < static_cast<int>(ev->parameterDefaults.size()))
                       ? ev->parameterDefaults[i] : 0.0f;
            return true;
        }
    }
    return false;
}

std::vector<const ks::audio::BankSoundMeta*> AudioBankManager::getAudioSounds() const
{
    std::vector<const ks::audio::BankSoundMeta*> result;
    if (!m_data.loaded) return result;
    for (const auto& s : m_data.sounds) {
        if (s.hasAudioData)
            result.push_back(&s);
    }
    return result;
}

void AudioBankManager::logBankInfo() const
{
    if (!m_data.loaded) return;
    printf("AudioBankManager: Car '%s' — %d events, %d sounds\n",
           m_data.carId.c_str(), static_cast<int>(m_data.events.size()), static_cast<int>(m_data.sounds.size()));
    for (const auto& ev : m_data.events) {
        printf("  Event: %s (", ev.name.c_str());
        for (int i = 0; i < ev.parameterCount && i < static_cast<int>(ev.parameterNames.size()); ++i) {
            if (i > 0) printf(", ");
            printf("%s", ev.parameterNames[i].c_str());
        }
        printf(")\n");
    }
}

} // namespace ks::sim
