#pragma once
/**
 * Per-event car audio volumes / pitch (CSP-style [AUDIO_VOLUME] / [AUDIO_PITCH]).
 * Independent product identity — same control surface, neutral naming.
 *
 * File: car/extension/audio_volumes.ini  or  car/data/audio_volumes.ini
 *
 * [AUDIO_VOLUME]
 * ENGINE_EXT = 1.2
 * ENGINE_INT = 1.0
 * TURBO = 0.8
 * ...
 *
 * [AUDIO_PITCH]
 * ENGINE_EXT = 1.0
 */
#include "SimulatorAudio.h"
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <cmath>

namespace ks {
namespace sim {

struct CarEventVolumes {
    // Multipliers (1.0 = default). Keys match CSP-style event names (upper).
    std::unordered_map<std::string, float> volume;
    std::unordered_map<std::string, float> pitch;
    bool loaded = false;

    float vol(const char* key, float def = 1.f) const {
        auto it = volume.find(key);
        return it != volume.end() ? it->second : def;
    }
    float pit(const char* key, float def = 1.f) const {
        auto it = pitch.find(key);
        return it != pitch.end() ? it->second : def;
    }

    /** Map to SimulatorAudio::SoundCategory gain. */
    float categoryVolume(SimulatorAudio::SoundCategory cat) const {
        using C = SimulatorAudio::SoundCategory;
        switch (cat) {
        case C::EngineInterior: return vol("ENGINE_INT");
        case C::EngineExterior: return vol("ENGINE_EXT");
        case C::Turbo: return vol("TURBO");
        case C::Wastegate: return vol("TURBO"); // fallback
        case C::Blowoff: return vol("TURBO");
        case C::Wind: return vol("WIND");
        case C::Transmission: return vol("TRANSMISSION");
        case C::TransmissionExt: return vol("TRANSMISSION");
        case C::SkidAsphalt: return vol("SKID_EXT", vol("DIRT"));
        case C::SkidGrass: return vol("DIRT");
        case C::SkidGravel: return vol("DIRT");
        case C::SkidKerb: return vol("SKID_EXT", vol("DIRT"));
        case C::SkidWet: return vol("SKID_EXT");
        case C::GearShift: return vol("GEAR_EXT", vol("GEAR_INT"));
        case C::GearClonk: return vol("GEAR_INT", vol("GEAR_GRIND"));
        case C::Brakes: return vol("BODYWORK"); // no dedicated key → soft map
        case C::Bodywork: return vol("BODYWORK");
        case C::Backfire: return vol("BACKFIRE_EXT", vol("BACKFIRE_INT"));
        case C::Limiter: return vol("LIMITER");
        case C::Starter: return vol("ENGINE_INT");
        case C::Ignition: return vol("ENGINE_INT");
        default: return 1.f;
        }
    }

    float categoryPitch(SimulatorAudio::SoundCategory cat) const {
        using C = SimulatorAudio::SoundCategory;
        switch (cat) {
        case C::EngineInterior: return pit("ENGINE_INT");
        case C::EngineExterior: return pit("ENGINE_EXT");
        case C::Turbo: return pit("TURBO");
        case C::Wind: return pit("WIND");
        case C::Transmission: return pit("TRANSMISSION");
        case C::GearShift: return pit("GEAR_EXT", pit("GEAR_INT"));
        case C::GearClonk: return pit("GEAR_INT");
        case C::Bodywork: return pit("BODYWORK");
        case C::Backfire: return pit("BACKFIRE_EXT");
        case C::Limiter: return pit("LIMITER");
        default: return 1.f;
        }
    }
};

class CarEventVolumeLoader {
public:
    static CarEventVolumes loadFile(const std::string& path) {
        CarEventVolumes out;
        std::ifstream in(path);
        if (!in) return out;

        auto trim = [](std::string s) {
            while (!s.empty() && (unsigned char)s.front() <= ' ') s.erase(s.begin());
            while (!s.empty() && (unsigned char)s.back() <= ' ') s.pop_back();
            return s;
        };
        auto upper = [](std::string s) {
            for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        };

        std::string line, section;
        while (std::getline(in, line)) {
            auto sc = line.find(';');
            if (sc != std::string::npos) line = line.substr(0, sc);
            line = trim(line);
            if (line.empty()) continue;
            if (line.front() == '[' && line.back() == ']') {
                section = upper(line.substr(1, line.size() - 2));
                continue;
            }
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = upper(trim(line.substr(0, eq)));
            float v = std::strtof(trim(line.substr(eq + 1)).c_str(), nullptr);
            if (section == "AUDIO_VOLUME" || section == "VOLUME")
                out.volume[key] = std::clamp(v, 0.f, 4.f);
            else if (section == "AUDIO_PITCH" || section == "PITCH")
                out.pitch[key] = std::clamp(v, 0.25f, 4.f);
        }
        out.loaded = !out.volume.empty() || !out.pitch.empty();
        if (out.loaded)
            std::fprintf(stderr, "CarEventVolumes: loaded %zu vol + %zu pitch from %s\n",
                         out.volume.size(), out.pitch.size(), path.c_str());
        return out;
    }

    /** Search car folder for audio_volumes.ini / extension config. */
    static CarEventVolumes loadForCar(const std::string& carDirectory) {
        const char* candidates[] = {
            "/extension/audio_volumes.ini",
            "/extension/ext_config.ini",
            "/data/audio_volumes.ini",
            "/audio_volumes.ini",
        };
        for (const char* rel : candidates) {
            auto v = loadFile(carDirectory + rel);
            if (v.loaded) return v;
        }
        return {};
    }
};

/**
 * Apply volumes to SimulatorAudio playback gains / engine volume.
 * Call after loadCarAudio / applySoundPack.
 */
inline void applyCarEventVolumes(SimulatorAudio& audio, const CarEventVolumes& v) {
    if (!v.loaded) return;

    // Engine master: geometric mean of INT/EXT if both set
    const float ei = v.vol("ENGINE_INT", 1.f);
    const float ee = v.vol("ENGINE_EXT", 1.f);
    audio.setEngineVolume(audio.engineVolume() * std::sqrt(std::max(0.f, ei * ee)));

    // Environment: wind + body soft average
    const float wind = v.vol("WIND", 1.f);
    const float body = v.vol("BODYWORK", 1.f);
    audio.setEnvironmentVolume(audio.masterVolume() > 0
        ? audio.masterVolume() * 0.75f * std::sqrt(std::max(0.f, wind * body))
        : 0.6f * std::sqrt(std::max(0.f, wind * body)));

    // Store per-category in ext path via setEngineVolume already done;
    // finer control: document that render path should multiply categoryVolume().
    (void)audio;
}

/** Runtime query for mixers: gain for a category after volumes loaded. */
struct CarAudioMixState {
    CarEventVolumes volumes;

    float gain(SimulatorAudio::SoundCategory cat) const {
        return volumes.categoryVolume(cat);
    }
    float pitch(SimulatorAudio::SoundCategory cat) const {
        return volumes.categoryPitch(cat);
    }
};

} // namespace sim
} // namespace ks
