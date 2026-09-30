# Garage spawn · Team roster · Per-event audio volumes

## 1. Per-event volumes (CSP-style)

**File:** `car/extension/audio_volumes.ini` or `car/data/audio_volumes.ini`

```ini
[AUDIO_VOLUME]
ENGINE_EXT = 1.2
ENGINE_INT = 1.0
TURBO = 0.85
WIND = 0.7
TRANSMISSION = 1.1

[AUDIO_PITCH]
ENGINE_EXT = 1.0
```

```cpp
auto vols = ks::sim::CarEventVolumeLoader::loadForCar(carDir);
ks::sim::applyCarEventVolumes(audio, vols);
float g = vols.categoryVolume(SimulatorAudio::SoundCategory::EngineExterior);
```

Keys mirror common CSP `[AUDIO_VOLUME]` names (no CSP dependency).

---

## 2. Team info

**File:** `team.ini`

| Field | Meaning |
|-------|---------|
| `CarCount` | how many cars in the squad |
| `Number` | race / door number |
| `Garage` | box index in pit garage |
| `GarageStart` / `GarageRow` | team block in the lane |
| `Driver` / `Livery` / `Player` | slot metadata |

```cpp
auto team = ks::sim::TeamInfoLoader::loadIni("teams/ksim_racing/team.ini");
// team.carCount, team.raceNumbers(), team.slots[i].garageIndex
```

---

## 3. Garage start (rF2-like)

| Session | Spawn |
|---------|--------|
| **Practice** | Garage box |
| **Qualify** | Garage box |
| **Race** | Grid |
| **Hotlap** | Track |

```cpp
ks::sim::GarageLayout layout = GarageSpawnPolicy::makeLinearRow(20, firstBoxPose, 6.f, pitHeading);
GarageSpawnPolicy::assignTeamToGarage(layout, team);

SpawnRequest req;
req.session = SessionType::Practice; // or Qualify
req.garageIndex = team.slots[0].garageIndex;
WorldPose pose = GarageSpawnPolicy::resolvePose(req, layout, /*grid*/ nullptr);

CarGarageRuntime rt;
beginSessionSpawn(rt, SessionType::Practice, req.garageIndex);
// rt.state == InGarage → release with rt.leaveGarage()
```

Place vehicle at `pose` when session starts; leave garage when player requests exit.
