#pragma once
/**
 * Vehicle upgrade system — mechanical + livery + sound packages.
 * LiveryRef / SoundPackRef defined in RaceComponentConfig.h
 */
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cctype>
#include "RaceComponentConfig.h"

namespace ks {
namespace vehicle {

enum class UpgradeCategory : int {
    Engine = 0,
    Aero,
    Transmission,
    Suspension,
    Brakes,
    Body,
    Livery,
    Sound,
    COUNT
};

inline const char* categoryName(UpgradeCategory c) {
    switch (c) {
    case UpgradeCategory::Engine: return "Engine";
    case UpgradeCategory::Aero: return "Aero";
    case UpgradeCategory::Transmission: return "Transmission";
    case UpgradeCategory::Suspension: return "Suspension";
    case UpgradeCategory::Brakes: return "Brakes";
    case UpgradeCategory::Body: return "Body";
    case UpgradeCategory::Livery: return "Livery";
    case UpgradeCategory::Sound: return "Sound";
    default: return "Unknown";
    }
}

struct PhysicsMods {
    float powerKwAdd = 0.f;
    float powerMult = 1.f;
    float maxRpmAdd = 0.f;
    float fuelUseMult = 1.f;
    float cdAdd = 0.f;
    float cdMult = 1.f;
    float clAdd = 0.f;
    float clMult = 1.f;
    float frontalAreaAdd = 0.f;
    float frontWingDfMult = 1.f;
    float rearWingDfMult = 1.f;
    float diffuserDfMult = 1.f;
    float massAddKg = 0.f;
    float finalDriveMult = 1.f;
    float brakeForceMult = 1.f;
    float gripMult = 1.f;

    PhysicsMods& combine(const PhysicsMods& o) {
        powerKwAdd += o.powerKwAdd;
        powerMult *= o.powerMult;
        maxRpmAdd += o.maxRpmAdd;
        fuelUseMult *= o.fuelUseMult;
        cdAdd += o.cdAdd;
        cdMult *= o.cdMult;
        clAdd += o.clAdd;
        clMult *= o.clMult;
        frontalAreaAdd += o.frontalAreaAdd;
        frontWingDfMult *= o.frontWingDfMult;
        rearWingDfMult *= o.rearWingDfMult;
        diffuserDfMult *= o.diffuserDfMult;
        massAddKg += o.massAddKg;
        finalDriveMult *= o.finalDriveMult;
        brakeForceMult *= o.brakeForceMult;
        gripMult *= o.gripMult;
        return *this;
    }
};

struct UpgradeLevel {
    std::string id;
    std::string name;
    std::string description;
    int price = 0;
    PhysicsMods mods;
    std::map<std::string, std::string> instanceSwaps;
    std::vector<std::string> enableNodes;
    std::vector<std::string> disableNodes;
    LiveryRef livery;
    SoundPackRef sound;

    bool hasLivery() const {
        return !livery.folder.empty() || !livery.diffuse.empty() || !livery.id.empty();
    }
    bool hasSound() const {
        return !sound.bankPath.empty() || !sound.soundsIni.empty() || !sound.id.empty();
    }
};

struct UpgradeType {
    UpgradeCategory category = UpgradeCategory::Engine;
    std::string name;
    std::string instance;
    std::vector<UpgradeLevel> levels;
    int selected = 0;

    const UpgradeLevel* current() const {
        if (levels.empty() || selected < 0 || selected >= (int)levels.size())
            return nullptr;
        return &levels[static_cast<size_t>(selected)];
    }
};

class VehicleUpgradeSystem {
public:
    const std::vector<UpgradeType>& types() const { return m_types; }
    std::vector<UpgradeType>& types() { return m_types; }
    void clear() { m_types.clear(); }

    UpgradeType& addType(UpgradeCategory cat, const std::string& name) {
        UpgradeType t;
        t.category = cat;
        t.name = name;
        m_types.push_back(t);
        return m_types.back();
    }

    bool select(UpgradeCategory cat, int levelIndex) {
        for (auto& t : m_types) {
            if (t.category == cat) {
                if (levelIndex < 0 || levelIndex >= (int)t.levels.size()) return false;
                t.selected = levelIndex;
                return true;
            }
        }
        return false;
    }

    bool selectById(UpgradeCategory cat, const std::string& levelId) {
        for (auto& t : m_types) {
            if (t.category != cat) continue;
            for (int i = 0; i < (int)t.levels.size(); ++i) {
                if (t.levels[static_cast<size_t>(i)].id == levelId) {
                    t.selected = i;
                    return true;
                }
            }
        }
        return false;
    }

    PhysicsMods appliedMods() const {
        PhysicsMods m;
        for (const auto& t : m_types) {
            if (const auto* L = t.current())
                m.combine(L->mods);
        }
        return m;
    }

    LiveryRef selectedLivery() const {
        for (const auto& t : m_types) {
            if (t.category != UpgradeCategory::Livery) continue;
            if (const auto* L = t.current())
                if (L->hasLivery()) return L->livery;
        }
        return {};
    }

    SoundPackRef selectedSound() const {
        for (const auto& t : m_types) {
            if (t.category != UpgradeCategory::Sound) continue;
            if (const auto* L = t.current())
                if (L->hasSound()) return L->sound;
        }
        for (const auto& t : m_types) {
            if (t.category != UpgradeCategory::Engine) continue;
            if (const auto* L = t.current())
                if (L->hasSound()) return L->sound;
        }
        return {};
    }

    void collectNodeOverrides(std::vector<std::string>& enable,
                              std::vector<std::string>& disable,
                              std::map<std::string, std::string>& swaps) const {
        enable.clear(); disable.clear(); swaps.clear();
        for (const auto& t : m_types) {
            const auto* L = t.current();
            if (!L) continue;
            enable.insert(enable.end(), L->enableNodes.begin(), L->enableNodes.end());
            disable.insert(disable.end(), L->disableNodes.begin(), L->disableNodes.end());
            for (const auto& kv : L->instanceSwaps)
                swaps[kv.first] = kv.second;
        }
    }

    bool loadFromIni(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            std::fprintf(stderr, "VehicleUpgradeSystem: cannot open %s\n", path.c_str());
            return false;
        }
        m_types.clear();
        UpgradeType* curType = nullptr;
        UpgradeLevel* curLevel = nullptr;
        int brace = 0;

        auto trim = [](std::string s) {
            while (!s.empty() && (unsigned char)s.front() <= ' ') s.erase(s.begin());
            while (!s.empty() && (unsigned char)s.back() <= ' ') s.pop_back();
            return s;
        };
        auto unquote = [&](std::string s) {
            s = trim(s);
            if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                return s.substr(1, s.size() - 2);
            return s;
        };
        auto catFromName = [](std::string low) -> UpgradeCategory {
            for (char& c : low) c = static_cast<char>(std::tolower((unsigned char)c));
            if (low.find("engine") != std::string::npos) return UpgradeCategory::Engine;
            if (low.find("aero") != std::string::npos || low.find("wing") != std::string::npos)
                return UpgradeCategory::Aero;
            if (low.find("trans") != std::string::npos || low.find("gear") != std::string::npos)
                return UpgradeCategory::Transmission;
            if (low.find("susp") != std::string::npos) return UpgradeCategory::Suspension;
            if (low.find("brake") != std::string::npos) return UpgradeCategory::Brakes;
            if (low.find("liver") != std::string::npos || low.find("skin") != std::string::npos)
                return UpgradeCategory::Livery;
            if (low.find("sound") != std::string::npos || low.find("audio") != std::string::npos)
                return UpgradeCategory::Sound;
            return UpgradeCategory::Body;
        };

        std::string line;
        while (std::getline(in, line)) {
            auto sc = line.find(';');
            if (sc != std::string::npos) line = line.substr(0, sc);
            line = trim(line);
            if (line.empty()) continue;
            if (line == "{") { ++brace; continue; }
            if (line == "}") {
                --brace;
                if (brace <= 1) curLevel = nullptr;
                if (brace <= 0) { curType = nullptr; brace = 0; }
                continue;
            }
            if (line.rfind("UpgradeType", 0) == 0) {
                auto eq = line.find('=');
                std::string name = eq != std::string::npos ? unquote(line.substr(eq + 1)) : "Package";
                m_types.push_back({});
                curType = &m_types.back();
                curType->category = catFromName(name);
                curType->name = name;
                curLevel = nullptr;
                continue;
            }
            if (line.rfind("UpgradeLevel", 0) == 0 && curType) {
                auto eq = line.find('=');
                std::string name = eq != std::string::npos ? unquote(line.substr(eq + 1)) : "Level";
                curType->levels.push_back({});
                curLevel = &curType->levels.back();
                curLevel->name = name;
                curLevel->id = name;
                for (char& c : curLevel->id) {
                    if (c == ' ') c = '_';
                    c = static_cast<char>(std::tolower((unsigned char)c));
                }
                continue;
            }
            if (!curLevel) continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = unquote(line.substr(eq + 1));
            auto& M = curLevel->mods;

            if (key == "Description") curLevel->description = val;
            else if (key == "Price") curLevel->price = std::atoi(val.c_str());
            else if (key == "PowerKwAdd" || key == "PowerAdd") M.powerKwAdd = std::strtof(val.c_str(), nullptr);
            else if (key == "PowerMult") M.powerMult = std::strtof(val.c_str(), nullptr);
            else if (key == "MaxRpmAdd") M.maxRpmAdd = std::strtof(val.c_str(), nullptr);
            else if (key == "FuelUseMult") M.fuelUseMult = std::strtof(val.c_str(), nullptr);
            else if (key == "CdAdd") M.cdAdd = std::strtof(val.c_str(), nullptr);
            else if (key == "CdMult") M.cdMult = std::strtof(val.c_str(), nullptr);
            else if (key == "ClAdd") M.clAdd = std::strtof(val.c_str(), nullptr);
            else if (key == "ClMult") M.clMult = std::strtof(val.c_str(), nullptr);
            else if (key == "FrontalAreaAdd") M.frontalAreaAdd = std::strtof(val.c_str(), nullptr);
            else if (key == "FrontWingDfMult") M.frontWingDfMult = std::strtof(val.c_str(), nullptr);
            else if (key == "RearWingDfMult") M.rearWingDfMult = std::strtof(val.c_str(), nullptr);
            else if (key == "DiffuserDfMult") M.diffuserDfMult = std::strtof(val.c_str(), nullptr);
            else if (key == "MassAddKg" || key == "MassAdd") M.massAddKg = std::strtof(val.c_str(), nullptr);
            else if (key == "FinalDriveMult") M.finalDriveMult = std::strtof(val.c_str(), nullptr);
            else if (key == "BrakeForceMult") M.brakeForceMult = std::strtof(val.c_str(), nullptr);
            else if (key == "GripMult") M.gripMult = std::strtof(val.c_str(), nullptr);
            else if (key == "EnableNode") curLevel->enableNodes.push_back(val);
            else if (key == "DisableNode") curLevel->disableNodes.push_back(val);
            else if (key == "Instance" && curType) curType->instance = val;
            else if (key == "LiveryId" || key == "SkinId") curLevel->livery.id = val;
            else if (key == "LiveryName" || key == "SkinName") curLevel->livery.name = val;
            else if (key == "LiveryFolder" || key == "SkinFolder" || key == "Skin") curLevel->livery.folder = val;
            else if (key == "LiveryDiffuse" || key == "Diffuse") curLevel->livery.diffuse = val;
            else if (key == "LiverySpecular") curLevel->livery.specular = val;
            else if (key == "LiveryNormal") curLevel->livery.normal = val;
            else if (key == "LiveryPreview") curLevel->livery.preview = val;
            else if (key == "RaceNumber" || key == "Number") curLevel->livery.number = std::atoi(val.c_str());
            else if (key == "Driver") curLevel->livery.driverName = val;
            else if (key == "Team") curLevel->livery.teamName = val;
            else if (key == "SoundId" || key == "SoundPack") curLevel->sound.id = val;
            else if (key == "SoundName") curLevel->sound.name = val;
            else if (key == "SoundBank" || key == "Bank" || key == "BankPath") curLevel->sound.bankPath = val;
            else if (key == "EngineIni" || key == "SoundEngineIni") curLevel->sound.engineIni = val;
            else if (key == "SoundsIni") curLevel->sound.soundsIni = val;
            else if (key == "EngineGain") curLevel->sound.engineGain = std::strtof(val.c_str(), nullptr);
            else if (key == "ExteriorGain") curLevel->sound.exteriorGain = std::strtof(val.c_str(), nullptr);
            else if (key == "TurboGain") curLevel->sound.turboGain = std::strtof(val.c_str(), nullptr);
            else if (key.rfind("Sample.", 0) == 0)
                curLevel->sound.sampleOverrides[key.substr(7)] = val;
            else if (key == "Swap" || key == "GEN") {
                auto colon = val.find(':');
                auto eq2 = val.find('=');
                if (colon != std::string::npos)
                    curLevel->instanceSwaps[val.substr(0, colon)] = val.substr(colon + 1);
                else if (eq2 != std::string::npos)
                    curLevel->instanceSwaps[val.substr(0, eq2)] = val.substr(eq2 + 1);
            }
        }
        std::fprintf(stderr, "VehicleUpgradeSystem: loaded %zu types from %s\n",
                     m_types.size(), path.c_str());
        return !m_types.empty();
    }

    void loadDefaults() {
        clear();
        {
            auto& t = addType(UpgradeCategory::Engine, "Engine Package");
            UpgradeLevel stock; stock.id = "stock"; stock.name = "Stock";
            UpgradeLevel s2; s2.id = "stage2"; s2.name = "Stage 2"; s2.price = 2500;
            s2.mods.powerKwAdd = 20.f; s2.mods.powerMult = 1.05f;
            s2.sound.id = "stage2_bank"; s2.sound.bankPath = "sfx/engine_stage2";
            UpgradeLevel s3; s3.id = "stage3"; s3.name = "Stage 3"; s3.price = 8000;
            s3.mods.powerKwAdd = 45.f; s3.mods.powerMult = 1.12f;
            s3.sound.id = "stage3_bank"; s3.sound.bankPath = "sfx/engine_race";
            s3.sound.engineGain = 1.1f;
            t.levels = { stock, s2, s3 };
        }
        {
            auto& t = addType(UpgradeCategory::Aero, "Aero Package");
            UpgradeLevel stock; stock.id = "stock"; stock.name = "Stock body";
            stock.enableNodes = { "WING_STOCK" }; stock.disableNodes = { "WING_RACE" };
            UpgradeLevel race; race.id = "race_wing"; race.name = "Race wing"; race.price = 1800;
            race.mods.clAdd = -0.12f; race.mods.cdAdd = 0.04f;
            race.enableNodes = { "WING_RACE" }; race.disableNodes = { "WING_STOCK" };
            t.levels = { stock, race };
        }
        {
            auto& t = addType(UpgradeCategory::Livery, "Livery");
            UpgradeLevel a; a.id = "factory"; a.name = "Factory";
            a.livery.id = "factory"; a.livery.folder = "skins/factory"; a.livery.number = 1;
            UpgradeLevel b; b.id = "sponsor_red"; b.name = "Sponsor Red"; b.price = 500;
            b.livery.id = "sponsor_red"; b.livery.folder = "skins/sponsor_red";
            b.livery.number = 7; b.livery.teamName = "ksim Racing";
            t.levels = { a, b };
        }
        {
            auto& t = addType(UpgradeCategory::Sound, "Sound Pack");
            UpgradeLevel a; a.id = "stock_sfx"; a.name = "Stock";
            a.sound.id = "stock"; a.sound.bankPath = "sfx/stock"; a.sound.soundsIni = "sfx/stock/sounds.ini";
            UpgradeLevel b; b.id = "race_sfx"; b.name = "Race exhaust"; b.price = 300;
            b.sound.id = "race"; b.sound.bankPath = "sfx/race";
            b.sound.soundsIni = "sfx/race/sounds.ini"; b.sound.engineGain = 1.15f;
            t.levels = { a, b };
        }
    }

private:
    std::vector<UpgradeType> m_types;
};

} // namespace vehicle
} // namespace ks
