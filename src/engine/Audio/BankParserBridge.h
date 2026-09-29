#pragma once
#include <string>
#include <vector>

namespace ks { namespace engine { namespace audio {
class BankParserBridge {
public:
    static BankParserBridge& instance() { static BankParserBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace

namespace ks { namespace audio {

/** One FMOD event inside a .bank file, with its exposed parameters. */
struct BankEventMeta {
    std::string name;
    int parameterCount = 0;
    std::vector<std::string> parameterNames;
    std::vector<float> parameterDefaults;
};

/** One FMOD sound referenced by the bank (may or may not carry sample data). */
struct BankSoundMeta {
    std::string name;
    bool hasAudioData = false;
    unsigned int sampleCount = 0;
};

struct ParsedBank {
    bool valid = false;
    std::vector<BankEventMeta> events;
    std::vector<BankSoundMeta> sounds;
};

/**
 * Reads an FMOD .bank file.
 *
 * Qt-free build: the previous implementation went through the Qt editor's
 * bank tooling, which is not available here. Until a standalone FMOD bank
 * reader exists, this reports `valid = false` so callers degrade gracefully
 * (AudioBankManager::loadBank() just reports "bank not loaded").
 */
ParsedBank parseBankFile(const std::string& path);

}} // namespace ks::audio
