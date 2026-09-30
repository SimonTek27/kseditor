#pragma once
/**
 * Resolved appearance for a car at a given race round:
 *   nodes + livery + sound + physics mods
 */
#include "RaceComponentConfig.h"
#include "VehicleUpgradeSystem.h"
#include <string>
#include <vector>
#include <map>

namespace ks {
namespace vehicle {

struct VehicleAppearanceBundle {
    RaceComponentConfig components;
    LiveryRef livery;   // from RaceComponentConfig.h
    SoundPackRef sound;
    std::vector<std::string> enableNodes;
    std::vector<std::string> disableNodes;
    std::map<std::string, std::string> instanceSwaps;
    PhysicsMods physics;

    bool hasLivery() const {
        return !livery.folder.empty() || !livery.diffuse.empty() || !livery.id.empty();
    }
    bool hasSound() const {
        return !sound.bankPath.empty() || !sound.soundsIni.empty() || !sound.id.empty();
    }
};

namespace detail {

inline LiveryRef toRaceLivery(const ks::vehicle::LiveryRef& /*placeholder*/) {
    return {};
}

// Copy from UpgradeSystem livery (same field layout) into RaceComponentConfig::LiveryRef
template <typename UpLiv>
inline LiveryRef copyLivery(const UpLiv& u) {
    LiveryRef L;
    L.id = u.id;
    L.name = u.name;
    L.folder = u.folder;
    L.diffuse = u.diffuse;
    L.specular = u.specular;
    L.normal = u.normal;
    L.preview = u.preview;
    L.number = u.number;
    L.driverName = u.driverName;
    L.teamName = u.teamName;
    return L;
}

template <typename UpSnd>
inline SoundPackRef copySound(const UpSnd& u) {
    SoundPackRef S;
    S.id = u.id;
    S.name = u.name;
    S.bankPath = u.bankPath;
    S.engineIni = u.engineIni;
    S.soundsIni = u.soundsIni;
    S.engineGain = u.engineGain;
    S.exteriorGain = u.exteriorGain;
    S.turboGain = u.turboGain;
    S.sampleOverrides = u.sampleOverrides;
    return S;
}

} // namespace detail

/**
 * Priority for livery/sound:
 *   1. Selected UpgradeCategory::Livery / Sound (or Engine-embedded sound)
 *   2. Race _rd*.ini [Livery]/[Sound]
 *   3. Empty → keep car defaults in the app
 */
inline VehicleAppearanceBundle resolveAppearance(
    const std::string& kn5Path,
    int round,
    const VehicleUpgradeSystem* upgrades = nullptr) {

    VehicleAppearanceBundle out;
    out.components = RaceComponentConfigLoader::loadForRound(kn5Path, round);
    out.livery = out.components.livery;
    out.sound = out.components.sound;

    if (upgrades) {
        out.physics = upgrades->appliedMods();
        upgrades->collectNodeOverrides(out.enableNodes, out.disableNodes, out.instanceSwaps);

        for (const auto& t : upgrades->types()) {
            const auto* L = t.current();
            if (!L) continue;
            if (t.category == UpgradeCategory::Livery && L->hasLivery())
                out.livery = detail::copyLivery(L->livery);
            if (t.category == UpgradeCategory::Sound && L->hasSound())
                out.sound = detail::copySound(L->sound);
            if (t.category == UpgradeCategory::Engine && L->hasSound() &&
                out.sound.bankPath.empty() && out.sound.id.empty())
                out.sound = detail::copySound(L->sound);
        }
    }

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
