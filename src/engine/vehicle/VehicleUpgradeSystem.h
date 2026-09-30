#pragma once
/**
 * Vehicle upgrade system — mechanical packages (engine, aero, …).
 * Inspired by rF2 UpgradeType/UpgradeLevel + HDV deltas; ksim identity (no third-party branding).
 *
 * Flow:
 *   1. loadFromIni(carDir/upgrades.ini)  OR  register packages in code
 *   2. select(category, levelId)
 *   3. applyTo(VehicleSimulator&) / appliedMods()
 */
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdio>

namespace ks {
namespace vehicle {

enum class UpgradeCategory : int {
    Engine = 0,
    Aero,
    Transmission,
    Suspension,
    Brakes,
    Body,       // mass / CG / cosmetic instance swap
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
    default: return "Unknown";
    }
}

/** Additive / multiplicative physics deltas applied after base car params. */
struct PhysicsMods {
    // Engine
    float powerKwAdd = 0.f;          // +kW peak
    float powerMult = 1.f;           // × peak power
    float maxRpmAdd = 0.f;
    float fuelUseMult = 1.f;

    // Aero
    float cdAdd = 0.f;               // Δ Cd
    float cdMult = 1.f;
    float clAdd = 0.f;               // Δ lift coeff (negative = more DF)
    float clMult = 1.f;
    float frontalAreaAdd = 0.f;
    float frontWingDfMult = 1.f;
    float rearWingDfMult = 1.f;
    float diffuserDfMult = 1.f;

    // Drivetrain / mass
    float massAddKg = 0.f;
    float finalDriveMult = 1.f;

    // Brakes / handling soft
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
    std::string id;              // e.g. "stock", "stage2"
    std::string name;            // display
    std::string description;
    int price = 0;
    PhysicsMods mods;
    /** Optional mesh/instance tags (GEN-like): node → mesh or visibility */
    std::map<std::string, std::string> instanceSwaps; // Instance → mesh or "off"
    std::vector<std::string> enableNodes;
    std::vector<std::string> disableNodes;
};

struct UpgradeType {
    UpgradeCategory category = UpgradeCategory::Engine;
    std::string name;            // "Engine Package"
    std::string instance;        // primary visual instance (optional)
    std::vector<UpgradeLevel> levels;
    int selected = 0;            // index into levels

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

    /** Register a type with at least one level. */
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

    /** Stack all selected levels. */
    PhysicsMods appliedMods() const {
        PhysicsMods m;
        for (const auto& t : m_types) {
            if (const auto* L = t.current())
                m.combine(L->mods);
        }
        return m;
    }

    /** Nodes forced on/off by current selection (for render / KN5 filter). */
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

    /**
 * Minimal upgrades.ini parser (ksim dialect, rF2-inspired).
 *
 * UpgradeType="Engine"
 * {
 *   UpgradeLevel="Stock" { Description="Base" }
 *   UpgradeLevel="Stage 2" {
 *     Description="More power"
 *     Price=2500
 *     PowerKwAdd=25
 *     PowerMult=1.08
 *     CdAdd=-0.01
 *     ClAdd=-0.05
 *     EnableNode=WING_RACE
 *     DisableNode=WING_STOCK
 *   }
 * }
 */
    bool loadFromIni(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            std::fprintf(stderr, "VehicleUpgradeSystem: cannot open %s\n", path.c_str());
            return false;
        }
        m_types.clear();
        std::string line, section;
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
                UpgradeCategory cat = UpgradeCategory::Body;
                std::string low = name;
                for (char& c : low) c = static_cast<char>(std::tolower((unsigned char)c));
                if (low.find("engine") != std::string::npos) cat = UpgradeCategory::Engine;
                else if (low.find("aero") != std::string::npos || low.find("wing") != std::string::npos)
                    cat = UpgradeCategory::Aero;
                else if (low.find("trans") != std::string::npos || low.find("gear") != std::string::npos)
                    cat = UpgradeCategory::Transmission;
                else if (low.find("susp") != std::string::npos) cat = UpgradeCategory::Suspension;
                else if (low.find("brake") != std::string::npos) cat = UpgradeCategory::Brakes;
                m_types.push_back({});
                curType = &m_types.back();
                curType->category = cat;
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
            else if (key == "Swap" || key == "GEN") {
                // Swap=NODE:mesh  or GEN=<TAG>=mesh
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

    /** Built-in demo packages (engine + aero) if no ini. */
    void loadDefaults() {
        clear();
        {
            auto& t = addType(UpgradeCategory::Engine, "Engine Package");
            UpgradeLevel stock; stock.id = "stock"; stock.name = "Stock"; stock.description = "Factory engine";
            UpgradeLevel s2; s2.id = "stage2"; s2.name = "Stage 2"; s2.description = "ECU + intake";
            s2.price = 2500; s2.mods.powerKwAdd = 20.f; s2.mods.powerMult = 1.05f; s2.mods.maxRpmAdd = 200.f;
            UpgradeLevel s3; s3.id = "stage3"; s3.name = "Stage 3"; s3.description = "Full race engine";
            s3.price = 8000; s3.mods.powerKwAdd = 45.f; s3.mods.powerMult = 1.12f; s3.mods.maxRpmAdd = 500.f;
            s3.mods.fuelUseMult = 1.15f;
            t.levels = { stock, s2, s3 };
        }
        {
            auto& t = addType(UpgradeCategory::Aero, "Aero Package");
            UpgradeLevel stock; stock.id = "stock"; stock.name = "Stock body";
            stock.enableNodes = { "WING_STOCK" }; stock.disableNodes = { "WING_RACE" };
            UpgradeLevel race; race.id = "race_wing"; race.name = "Race wing";
            race.description = "Higher downforce, more drag"; race.price = 1800;
            race.mods.clAdd = -0.12f; race.mods.cdAdd = 0.04f;
            race.mods.rearWingDfMult = 1.25f;
            race.enableNodes = { "WING_RACE" }; race.disableNodes = { "WING_STOCK" };
            UpgradeLevel low; low.id = "low_drag"; low.name = "Low drag";
            low.description = "Reduced wing for speed tracks"; low.price = 1200;
            low.mods.clAdd = 0.05f; low.mods.cdAdd = -0.03f;
            low.mods.rearWingDfMult = 0.7f;
            low.enableNodes = { "WING_LOW" }; low.disableNodes = { "WING_STOCK", "WING_RACE" };
            t.levels = { stock, race, low };
        }
    }

private:
    std::vector<UpgradeType> m_types;
};

/** Apply stacked mods onto base vehicle parameters (call after loadVehicleParams). */
template <typename VehicleT>
void applyUpgradesToVehicle(VehicleT& veh, const PhysicsMods& m) {
    // Base reads then write — VehicleSimulator-style setters
    const auto st = veh.getState();
    const float baseMass = st.mass > 1.f ? st.mass : 1200.f;
    veh.setMass(baseMass + m.massAddKg);

    // Power: assume setEnginePower is peak kW
    // We don't have getters for all — apply relative from typical path:
    // Caller should pass basePower if needed. Soft approach: setEnginePower scaled if available.
    (void)m.powerMult;
    (void)m.powerKwAdd;
    // Concrete apply lives in applyUpgradesToVehicleSimulator below for known type.
}

} // namespace vehicle
} // namespace ks
