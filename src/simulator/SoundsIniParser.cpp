#include "SoundsIniParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ks::sim {

std::string SoundsIniParser::trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string SoundsIniParser::toLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

float SoundsIniParser::parseFloat(const std::string& value, float defaultVal)
{
    std::string trimmed = trim(value);
    if (trimmed.empty()) return defaultVal;
    try {
        return std::stof(trimmed);
    } catch (...) {
        return defaultVal;
    }
}

bool SoundsIniParser::parseFromString(const std::string& content)
{
    m_data = SoundsIniData();
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);

        line = trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = toLower(line.substr(1, line.size() - 2));
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = toLower(trim(line.substr(0, eqPos)));
        std::string value = trim(line.substr(eqPos + 1));

        if (currentSection == "skids") {
            if (key == "entry_point") m_data.skids.entryPoint = parseFloat(value, 0.25f);
            else if (key == "mix_volume") m_data.skids.mixVolume = parseFloat(value, 0.9f);
            else if (key == "pitch_base") m_data.skids.pitchBase = parseFloat(value, 0.66f);
            else if (key == "pitch_gain") m_data.skids.pitchGain = parseFloat(value, 0.12f);
            else if (key == "distance_scale") m_data.skids.distanceScale = parseFloat(value, 10.0f);
        }
        else if (currentSection == "wind") {
            if (key == "speed_gain_0") m_data.wind.speedGain0 = parseFloat(value, 0.0045f);
            else if (key == "speed_gain_1") m_data.wind.speedGain1 = parseFloat(value, 0.000016f);
            else if (key == "volume_gain") m_data.wind.volumeGain = parseFloat(value, 0.98f);
            else if (key == "pitch_reference") m_data.wind.pitchReference = parseFloat(value, 60.0f);
            else if (key == "pitch_gain") m_data.wind.pitchGain = parseFloat(value, 0.35f);
            else if (key == "distance_scale") m_data.wind.distanceScale = parseFloat(value, 10.0f);
        }
        else if (currentSection == "tyre_rolling") {
        }
        else if (currentSection == "backfire") {
            if (key == "maxgas") m_data.backfire.maxGas = parseFloat(value, 0.4f);
            else if (key == "minrpm") m_data.backfire.minRpm = parseFloat(value, 2500.0f);
            else if (key == "maxrpm") m_data.backfire.maxRpm = parseFloat(value, 8000.0f);
            else if (key == "volume_in") m_data.backfire.volumeIn = parseFloat(value, 0.000005f);
            else if (key == "volume_out") m_data.backfire.volumeOut = parseFloat(value, 0.00005f);
            else if (key == "volume_scale_out") m_data.backfire.volumeScaleOut = parseFloat(value, 0.00002f);
        }
        else if (currentSection == "body_work") {
            if (key == "min") m_data.bodyWorkMin = parseFloat(value, 0.1f);
        }
        else if (currentSection == "turbo") {
            if (key == "ref_pitch") m_data.turbo.refPitch = parseFloat(value, 0.5f);
            else if (key == "pitch_gain") m_data.turbo.pitchGain = parseFloat(value, 0.3f);
            else if (key == "volume") m_data.turbo.volume = parseFloat(value, 0.4f);
            else if (key == "min_volume") m_data.turbo.minVolume = parseFloat(value, 0.0f);
            else if (key == "max_volume_at") m_data.turbo.maxVolumeAt = parseFloat(value, 1.0f);
            else if (key == "distance_scale") m_data.turbo.distanceScale = parseFloat(value, 2.0f);
            else if (key == "velocity_gain") m_data.turbo.velocityGain = parseFloat(value, 0.5f);
        }
        else if (currentSection == "turbo_bow") {
            if (key == "full_gas_limit") m_data.turboBow.fullGasLimit = parseFloat(value, 1.5f);
            else if (key == "zero_gas_limit") m_data.turboBow.zeroGasLimit = parseFloat(value, 0.8f);
            else if (key == "ref_pitch") m_data.turboBow.refPitch = parseFloat(value, 0.8f);
            else if (key == "pitch_gain") m_data.turboBow.pitchGain = parseFloat(value, 0.3f);
            else if (key == "volume") m_data.turboBow.volume = parseFloat(value, 0.15f);
            else if (key == "min_volume") m_data.turboBow.minVolume = parseFloat(value, 0.0f);
            else if (key == "max_volume_at") m_data.turboBow.maxVolumeAt = parseFloat(value, 1.0f);
            else if (key == "distance_scale") m_data.turboBow.distanceScale = parseFloat(value, 2.0f);
            else if (key == "gain") m_data.turboBow.gain = parseFloat(value, 1.0f);
        }
        else if (currentSection == "engine") {
            if (key == "position") m_data.enginePosition = trim(value);
            else if (key == "volume_in") m_data.engineVolumeIn = parseFloat(value, 0.2f);
            else if (key == "volume_out") m_data.engineVolumeOut = parseFloat(value, 0.2f);
            else if (key == "volume_scale_out") m_data.engineVolumeScaleOut = parseFloat(value, 35.0f);
        }
    }

    m_data.valid = true;
    return true;
}

bool SoundsIniParser::parse(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseFromString(buffer.str());
}

} // namespace ks::sim
