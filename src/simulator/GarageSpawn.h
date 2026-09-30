#pragma once
/**
 * Garage spawn policy (rF2-like):
 *   Practice + Qualify → car starts in assigned garage box (in pit / garage).
 *   Race → grid slot (not implemented here — provided by session grid).
 *
 * World pose for a garage index is supplied by the track (pit box transforms).
 */
#include "RaceSession.h"
#include "TeamInfo.h"
#include <cmath>
#include <vector>
#include <string>

namespace ks {
namespace sim {

struct WorldPose {
    float x = 0, y = 0, z = 0;
    float heading = 0; // rad, yaw
};

struct GarageBox {
    int index = 0;
    WorldPose pose;
    bool occupied = false;
    int teamCarRaceNumber = -1; // who is assigned
};

struct GarageLayout {
    std::vector<GarageBox> boxes;
    float pitLaneHeading = 0.f;

    const GarageBox* find(int index) const {
        for (const auto& b : boxes)
            if (b.index == index) return &b;
        return nullptr;
    }

    GarageBox* find(int index) {
        for (auto& b : boxes)
            if (b.index == index) return &b;
        return nullptr;
    }
};

enum class SpawnLocation : uint8_t {
    Garage = 0,  // inside box / pit garage
    PitLane,     // exit line
    Grid,        // race start
    Track        // free / hotlap
};

struct SpawnRequest {
    SessionType session = SessionType::Practice;
    int garageIndex = 0;
    int gridPosition = 1; // 1-based for race
    SpawnLocation forced = SpawnLocation::Garage; // ignored if usePolicy
    bool usePolicy = true;
};

class GarageSpawnPolicy {
public:
    /** rF2-like default: Practice & Qualify start in garage. */
    static SpawnLocation locationForSession(SessionType type) {
        switch (type) {
        case SessionType::Practice:
        case SessionType::Qualify:
            return SpawnLocation::Garage;
        case SessionType::Race:
            return SpawnLocation::Grid;
        case SessionType::Hotlap:
            return SpawnLocation::Track;
        default:
            return SpawnLocation::Garage;
        }
    }

    static bool startsInGarage(SessionType type) {
        return locationForSession(type) == SpawnLocation::Garage;
    }

    /**
 * Resolve world pose for a car slot.
 * @param gridPoses optional race grid (index 0 = P1)
 */
    static WorldPose resolvePose(
        const SpawnRequest& req,
        const GarageLayout& layout,
        const std::vector<WorldPose>* gridPoses = nullptr) {

        SpawnLocation loc = req.usePolicy ? locationForSession(req.session) : req.forced;

        if (loc == SpawnLocation::Garage || loc == SpawnLocation::PitLane) {
            if (const auto* box = layout.find(req.garageIndex)) {
                WorldPose p = box->pose;
                if (loc == SpawnLocation::PitLane) {
                    // nudge forward along pit heading (~8 m)
                    const float h = layout.pitLaneHeading != 0.f ? layout.pitLaneHeading : p.heading;
                    p.x += std::sin(h) * 8.f;
                    p.z += std::cos(h) * 8.f;
                    p.heading = h;
                }
                return p;
            }
            // fallback origin
            return {};
        }

        if (loc == SpawnLocation::Grid && gridPoses && !gridPoses->empty()) {
            int i = std::max(0, req.gridPosition - 1);
            if (i >= (int)gridPoses->size()) i = (int)gridPoses->size() - 1;
            return (*gridPoses)[static_cast<size_t>(i)];
        }

        return {}; // track free spawn — caller places on line
    }

    /** Assign team slots onto layout boxes (marks occupied). */
    static void assignTeamToGarage(GarageLayout& layout, const TeamInfo& team) {
        for (const auto& slot : team.slots) {
            if (!slot.active) continue;
            auto* box = layout.find(slot.garageIndex);
            if (!box) {
                // create virtual box at index with empty pose
                GarageBox b;
                b.index = slot.garageIndex;
                b.occupied = true;
                b.teamCarRaceNumber = slot.raceNumber;
                layout.boxes.push_back(b);
            } else {
                box->occupied = true;
                box->teamCarRaceNumber = slot.raceNumber;
            }
        }
    }

    /** Build linear garage row from a start pose + spacing (metres along heading). */
    static GarageLayout makeLinearRow(int count, WorldPose first, float spacingM = 6.f,
                                      float pitHeading = 0.f) {
        GarageLayout L;
        L.pitLaneHeading = pitHeading != 0.f ? pitHeading : first.heading;
        const float h = L.pitLaneHeading;
        for (int i = 0; i < count; ++i) {
            GarageBox b;
            b.index = i;
            b.pose = first;
            b.pose.x += std::sin(h + 1.5707963f) * (spacingM * float(i)); // lateral along pit
            b.pose.z += std::cos(h + 1.5707963f) * (spacingM * float(i));
            b.pose.heading = h;
            L.boxes.push_back(b);
        }
        return L;
    }
};

/** Session phase: car sits in garage until released (practice/qualy). */
enum class GarageState : uint8_t {
    InGarage = 0,   // stationary in box — rF2 practice/qualy start
    Leaving,        // rolling out
    OnTrack,
    Returning
};

struct CarGarageRuntime {
    GarageState state = GarageState::InGarage;
    int garageIndex = 0;
    int raceNumber = 0;
    bool engineRunning = false;

    void enterGarage() {
        state = GarageState::InGarage;
        engineRunning = false;
    }
    void leaveGarage() {
        state = GarageState::Leaving;
        engineRunning = true;
    }
    void onTrack() { state = GarageState::OnTrack; }
};

/** Call when configuring a new session for a player car. */
inline void beginSessionSpawn(CarGarageRuntime& car, SessionType type, int garageIndex) {
    car.garageIndex = garageIndex;
    if (GarageSpawnPolicy::startsInGarage(type)) {
        car.enterGarage();
    } else {
        car.state = GarageState::OnTrack;
        car.engineRunning = true;
    }
}

} // namespace sim
} // namespace ks
