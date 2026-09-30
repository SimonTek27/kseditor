# Vehicle audio hook — race / upgrade sound packs

## Flow

```text
loadCarAudio(carDir)           // baseline bank + data/sounds.ini + sfx/*.wav
    ↓
resolveAppearance(kn5, round, &ups)
    ↓
applyAppearanceAudio(audio, appearance, carDir)
    → applySoundPack(bank, soundsIni, gains, sampleOverrides)
```

## Code

```cpp
#include "simulator/VehicleAudioHook.h"
#include "engine/vehicle/VehicleAppearanceBundle.h"

ks::sim::SimulatorAudio audio;
audio.initialize();
audio.loadCarAudio(carDir);

auto appearance = ks::vehicle::resolveAppearance(kn5Path, raceRound, &ups);
ks::sim::applyAppearanceAudio(audio, appearance, carDir);
```

Or low-level:

```cpp
audio.applySoundPack(
    resolvedBankPath,
    resolvedSoundsIni,
    resolvedEngineIni,
    pack.engineGain,
    pack.exteriorGain,
    pack.turboGain,
    pack.sampleOverrides,
    carDir);
```

## What it does

| Input | Effect |
|-------|--------|
| `Bank` / `SoundBank` | `AudioBankManager::loadBank` |
| `SoundsIni` | Replace `m_soundsIni` tuning |
| `EngineGain` | Scale `m_engineVolume` |
| `ExteriorGain` | Scale `m_environmentVolume` |
| `TurboGain` | Scale turbo volume in sounds.ini |
| `Sample.*` | Recorded in `m_extConfig` for overrides |

## Files

- `src/simulator/VehicleAudioHook.h`
- `src/simulator/SimulatorAudio_SoundPack.cpp`
- `SimulatorAudio::applySoundPack` in `SimulatorAudio.h`

Add `SimulatorAudio_SoundPack.cpp` to the simulator CMake target if not globbed.
