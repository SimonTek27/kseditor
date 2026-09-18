#pragma once

#include <string>

namespace ks::sim {

struct SkidsConfig {
    float entryPoint = 0.25f;
    float mixVolume = 0.9f;
    float pitchBase = 0.66f;
    float pitchGain = 0.12f;
    float distanceScale = 10.0f;
};

struct WindConfig {
    float speedGain0 = 0.0045f;
    float speedGain1 = 0.000016f;
    float volumeGain = 0.98f;
    float pitchReference = 60.0f;
    float pitchGain = 0.35f;
    float distanceScale = 10.0f;
};

struct TurboConfig {
    float refPitch = 0.5f;
    float pitchGain = 0.3f;
    float volume = 0.4f;
    float minVolume = 0.0f;
    float maxVolumeAt = 1.0f;
    float distanceScale = 2.0f;
    float velocityGain = 0.5f;
    float fullGasLimit = 1.5f;
    float zeroGasLimit = 0.8f;
    float gain = 1.0f;
};

struct BackfireConfig {
    float maxGas = 0.4f;
    float minRpm = 2500.0f;
    float maxRpm = 8000.0f;
    float volumeIn = 0.000005f;
    float volumeOut = 0.00005f;
    float volumeScaleOut = 0.00002f;
};

struct SoundsIniData {
    SkidsConfig skids;
    WindConfig wind;
    TurboConfig turbo;
    TurboConfig turboBow;
    BackfireConfig backfire;
    float bodyWorkMin = 0.1f;
    std::string enginePosition = "rear";
    float engineVolumeIn = 0.2f;
    float engineVolumeOut = 0.2f;
    float engineVolumeScaleOut = 35.0f;
    bool valid = false;
};

class SoundsIniParser {
public:
    bool parse(const std::string& filePath);
    bool parseFromString(const std::string& content);
    const SoundsIniData& data() const { return m_data; }

private:
    float parseFloat(const std::string& value, float defaultVal = 0.0f);
    std::string trim(const std::string& s);
    std::string toLower(const std::string& s);
    SoundsIniData m_data;
};

} // namespace ks::sim
