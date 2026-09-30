# Vehicle upgrades — physics, livery, sound, race components

## Resolve one bundle per race

```cpp
ks::vehicle::VehicleUpgradeSystem ups;
ups.loadFromIni(carDir + "/upgrades.ini");
ups.selectById(UpgradeCategory::Engine, "stage2");
ups.selectById(UpgradeCategory::Livery, "sponsor_red");
ups.selectById(UpgradeCategory::Sound, "race_exhaust");

auto appearance = ks::vehicle::resolveAppearance("cars/myCar.kn5", /*round*/ 1, &ups);
// appearance.components  → nodes ON/OFF
// appearance.livery      → folder / diffuse / number
// appearance.sound       → bankPath / soundsIni / gains
// appearance.physics     → power / aero mods
```

**Priority livery & sound:** upgrade selection overrides `_rd*.ini` if set; else race config; else car default.

---

## Race INI (`myCar_rd1-2.ini`)

```ini
[Meta]
Rounds=1,2

[Nodes]
WING_RACE=1
WING_STOCK=0

[Livery]
Id=rd1_sponsor
Folder=skins/rd1_sponsor
Number=21
Driver=Player
Team=ksim Racing

[Sound]
Id=v8_race_weekend
Bank=sfx/v8_race
SoundsIni=sfx/v8_race/sounds.ini
EngineGain=1.12
```

---

## upgrades.ini categories

| Type | Keys |
|------|------|
| Engine / Aero / … | PowerKwAdd, ClAdd, EnableNode, … |
| **Livery** | LiveryFolder, LiveryDiffuse, RaceNumber, Team, Driver |
| **Sound** | SoundBank, SoundsIni, EngineGain, Sample.EngineInterior=… |
| Engine (optional) | SoundBank on Stage 2/3 so engine package swaps SFX |

---

## Wire to runtime

1. **Render** — load textures from `appearance.livery.folder` / `diffuse`
2. **Audio** — `SimulatorAudio` / `AudioBankManager` load `appearance.sound.bankPath` + `soundsIni`
3. **Physics** — `applyUpgradesToVehicleSimulator(veh, base, appearance.physics)`
4. **Nodes** — `filterNodes(catalog, appearance.components)`

Product identity: neutral names (Livery, Sound Pack), no third-party branding in UI.
