#pragma once

#include "Audio/BankParserBridge.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ks::sim {

struct BankAudioData {
    std::string carId;
    std::string bankPath;
    std::vector<ks::audio::BankEventMeta> events;
    std::vector<ks::audio::BankSoundMeta> sounds;
    std::unordered_map<std::string, int> eventIndex;
    bool loaded = false;
};

class AudioBankManager {
public:
    bool loadBank(const std::string& carDirectory, const std::string& carId);
    void unloadBank();
    bool isLoaded() const { return m_data.loaded; }

    const BankAudioData* data() const { return m_data.loaded ? &m_data : nullptr; }

    const ks::audio::BankEventMeta* findEvent(const std::string& name) const;
    bool getEventParameter(const std::string& eventName,
                          const std::string& paramName,
                          float& outValue) const;
    std::vector<const ks::audio::BankSoundMeta*> getAudioSounds() const;

    void logBankInfo() const;

private:
    BankAudioData m_data;
};

} // namespace ks::sim
