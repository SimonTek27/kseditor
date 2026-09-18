#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ks::audio {

struct BankEventMeta {
    std::string name;
    std::string path;
    int parameterCount = 0;
    std::vector<std::string> parameterNames;
    std::vector<float> parameterDefaults;
    bool loops = false;
    float defaultVolume = 1.0f;
};

struct BankSoundMeta {
    std::string name;
    int sampleRate = 0;
    int channels = 0;
    int length = 0;
    std::string format;
    bool hasAudioData = false;
    std::vector<float> samples;
};

struct ParsedBank {
    bool valid = false;
    bool encrypted = false;
    int version = 0;
    std::string name;
    std::string filePath;
    std::vector<BankEventMeta> events;
    std::vector<BankSoundMeta> sounds;
    int eventCount = 0;
    int soundCount = 0;
};

ParsedBank parseBankFile(const std::string& bankPath);
void extractBankAudio(ParsedBank& bank);

} // namespace ks::audio
