/**
 * SimulatorAudio::applySoundPack — race/upgrade sound banks.
 */
#include "SimulatorAudio.h"
#include <cstdio>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace ks {
namespace sim {

namespace {

SimulatorAudio::SoundCategory categoryFromName(const std::string& name) {
    std::string n = name;
    for (char& c : n)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (n.find("engineint") != std::string::npos || n == "engine_interior")
        return SimulatorAudio::SoundCategory::EngineInterior;
    if (n.find("engineext") != std::string::npos || n == "engine_exterior")
        return SimulatorAudio::SoundCategory::EngineExterior;
    if (n.find("turbo") != std::string::npos) return SimulatorAudio::SoundCategory::Turbo;
    if (n.find("waste") != std::string::npos) return SimulatorAudio::SoundCategory::Wastegate;
    if (n.find("blow") != std::string::npos) return SimulatorAudio::SoundCategory::Blowoff;
    if (n.find("wind") != std::string::npos) return SimulatorAudio::SoundCategory::Wind;
    if (n.find("trans") != std::string::npos) return SimulatorAudio::SoundCategory::Transmission;
    if (n.find("skid") != std::string::npos) return SimulatorAudio::SoundCategory::SkidAsphalt;
    if (n.find("gear") != std::string::npos) return SimulatorAudio::SoundCategory::GearShift;
    if (n.find("brake") != std::string::npos) return SimulatorAudio::SoundCategory::Brakes;
    if (n.find("body") != std::string::npos) return SimulatorAudio::SoundCategory::Bodywork;
    if (n.find("backfire") != std::string::npos) return SimulatorAudio::SoundCategory::Backfire;
    if (n.find("limiter") != std::string::npos) return SimulatorAudio::SoundCategory::Limiter;
    return SimulatorAudio::SoundCategory::Count;
}

} // namespace

bool SimulatorAudio::applySoundPack(
    const std::string& bankPath,
    const std::string& soundsIniPath,
    const std::string& /*engineIniPath*/,
    float engineGain,
    float exteriorGain,
    float turboGain,
    const std::unordered_map<std::string, std::string>& sampleOverrides,
    const std::string& carDirectory)
{
    if (!m_initialized.load()) {
        std::fprintf(stderr, "SimulatorAudio::applySoundPack: not initialized\n");
        return false;
    }

    bool any = false;

    if (!bankPath.empty()) {
        if (!m_bankManager)
            m_bankManager = std::make_unique<AudioBankManager>();

        std::string dir = bankPath;
        std::string carId = "pack";
        try {
            fs::path p(bankPath);
            if (fs::exists(p) && fs::is_regular_file(p)) {
                dir = p.parent_path().string();
                carId = p.stem().string();
            } else if (fs::exists(p) && fs::is_directory(p)) {
                dir = p.string();
                carId = p.filename().string();
                if (carId.empty() || carId == "." || carId == "..")
                    carId = "pack";
            } else {
                // path may not exist yet — still try parent as dir
                auto slash = bankPath.find_last_of("/\\");
                if (slash != std::string::npos) {
                    dir = bankPath.substr(0, slash);
                    carId = bankPath.substr(slash + 1);
                }
            }
        } catch (...) {
            auto slash = bankPath.find_last_of("/\\");
            if (slash != std::string::npos) {
                dir = bankPath.substr(0, slash);
                carId = bankPath.substr(slash + 1);
            }
        }

        bool ok = m_bankManager->loadBank(dir, carId);
        if (!ok && !carDirectory.empty())
            ok = m_bankManager->loadBank(carDirectory, carId);
        if (ok) {
            any = true;
            std::fprintf(stderr, "SimulatorAudio: sound pack bank loaded dir=%s id=%s\n",
                         dir.c_str(), carId.c_str());
        } else {
            std::fprintf(stderr, "SimulatorAudio: sound pack bank not found at %s\n",
                         bankPath.c_str());
        }
    }

    if (!soundsIniPath.empty()) {
        SoundsIniParser iniParser;
        if (iniParser.parse(soundsIniPath)) {
            m_soundsIni = iniParser.data();
            any = true;
            std::fprintf(stderr, "SimulatorAudio: pack sounds.ini loaded (%s)\n",
                         soundsIniPath.c_str());
        } else {
            std::fprintf(stderr, "SimulatorAudio: pack sounds.ini parse failed (%s)\n",
                         soundsIniPath.c_str());
        }
    }

    if (engineGain > 0.f) {
        m_engineVolume.store(m_engineVolume.load() * engineGain);
        any = true;
    }
    if (exteriorGain > 0.f && exteriorGain != 1.f) {
        m_environmentVolume.store(m_environmentVolume.load() * exteriorGain);
        any = true;
    }
    if (turboGain > 0.f && m_soundsIni.valid) {
        m_soundsIni.turbo.volume *= turboGain;
        m_soundsIni.turbo.gain *= turboGain;
        any = true;
    }

    for (const auto& kv : sampleOverrides) {
        m_extConfig.parameterOverrides["sample." + kv.first] = { kv.second, 1.f };
        (void)categoryFromName(kv.first);
        any = true;
        std::fprintf(stderr, "SimulatorAudio: sample override %s → %s\n",
                     kv.first.c_str(), kv.second.c_str());
    }

    if (any)
        m_loaded.store(true);

    return any;
}

} // namespace sim
} // namespace ks
