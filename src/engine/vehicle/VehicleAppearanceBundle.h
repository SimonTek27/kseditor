#pragma once
/**
 * Resolved appearance for a car at a given race round:
 *   nodes + livery + sound bank
 * Merges RaceComponentConfig (_rd*) with selected upgrades (Livery / Sound categories).
 */
#include "RaceComponentConfig.h"
#include "VehicleUpgradeSystem.h"
#include <string>
#include <vector>
#include <map>

namespace ks {
namespace vehicle {

/** Paths relative to car folder or absolute. */
struct LiveryRef {
    std::string id;              // e.g. "rd1_sponsor_a"
    std::string name;            // display
    std::string folder;          // skins/rd1_sponsor_a/ or texture set dir
    std::string diffuse;         // optional single albedo override
    std::string specular;
    std::string normal;
    std::string preview;         // UI thumbnail
    int number = -1;             // race number on door (-1 = keep default)
    std::string driverName;
    std::string teamName;
};

struct SoundPackRef {
    std::string id;              // e.g. "v8_race"
    std::string name;
    std::string bankPath;        // path to bank / folder (AudioBankManager)
    std::string engineIni;       // optional engine sound ini
    std::string soundsIni;       // sounds.ini override
    float engineGain = 1.f;
    float exteriorGain = 1.f;
    float turboGain = 1.f;
    /** Optional sample overrides: category name → file */
    std::map<std::string, std::string> sampleOverrides;
};

struct VehicleAppearanceBundle {
    RaceComponentConfig components;
    LiveryRef livery;
    SoundPackRef sound;
    std::vector<std::string> enableNodes;
    std::vector<std::string> disableNodes;
    std::map<std::string, std::string> instanceSwaps;
    PhysicsMods physics;         // stacked mechanical upgrades

    bool hasLivery() const { return !livery.folder.empty() || !livery.diffuse.empty() || !livery.id.empty(); }
    bool hasSound() const { return !sound.bankPath.empty() || !sound.soundsIni.empty() || !sound.id.empty(); }
};

/**
 * Build appearance for (kn5, round) + optional upgrades selection.
 * Priority for livery/sound:
 *   1. Race _rd*.ini [Livery]/[Sound]
 *   2. Selected UpgradeCategory::Livery / Sound levels
 *   3. Empty (caller keeps defaults)
 */
inline VehicleAppearanceBundle resolveAppearance(
    const std::string& kn5Path,
    int round,
    const VehicleUpgradeSystem* upgrades = nullptr) {

    VehicleAppearanceBundle out;
    out.components = RaceComponentConfigLoader::loadForRound(kn5Path, round);

    // From race config
    out.livery = out.components.livery;
    out.sound = out.components.sound;

    if (upgrades) {
        out.physics = upgrades->appliedMods();
        upgrades->collectNodeOverrides(out.enableNodes, out.disableNodes, out.instanceSwaps);

        // Livery / Sound upgrade categories override race defaults if set
        for (const auto& t : upgrades->types()) {
            const auto* L = t.current();
            if (!L) continue;
            if (t.category == UpgradeCategory::Livery && L->hasLivery())
                out.livery = L->livery;
            if (t.category == UpgradeCategory::Sound && L->hasSound())
                out.sound = L->sound;
            // Engine upgrades may also ship a sound bank (optional field on level)
            if (t.category == UpgradeCategory::Engine && L->hasSound() && !out.hasSound())
                out.sound = L->sound;
        }
    }

    // Merge enable/disable from race nodes already in components;
    // enableNodes from upgrades additive
    for (const auto& n : out.enableNodes)
        out.components.active.insert(n);
    for (const auto& n : out.disableNodes)
        out.components.inactive.insert(n);
    if (!out.enableNodes.empty() || !out.disableNodes.empty())
        out.components.hasExplicitList = true;

    return out;
}

} // namespace vehicle
} // namespace ks
