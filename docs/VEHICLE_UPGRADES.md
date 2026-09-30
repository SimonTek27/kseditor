# Vehicle upgrades + race component configs

## 1. Mechanical upgrades (rF2-inspired)

**API:** `src/engine/vehicle/VehicleUpgradeSystem.h`

| Category | Typical effect |
|----------|----------------|
| Engine | PowerKwAdd, PowerMult, MaxRpmAdd |
| Aero | Cd/Cl, wing DF mult, EnableNode/DisableNode |
| Transmission | FinalDriveMult |
| Suspension / Brakes | GripMult, BrakeForceMult |
| Body | MassAddKg, instance swaps |

### upgrades.ini (next to car data)

```ini
UpgradeType="Engine"
{
  UpgradeLevel="Stock" { Description="Factory" }
  UpgradeLevel="Stage 2" {
    Description="ECU + intake"
    Price=2500
    PowerKwAdd=20
    PowerMult=1.05
    MaxRpmAdd=200
  }
}

UpgradeType="Aero"
{
  UpgradeLevel="Stock" {
    EnableNode=WING_STOCK
    DisableNode=WING_RACE
  }
  UpgradeLevel="Race wing" {
    Price=1800
    ClAdd=-0.12
    CdAdd=0.04
    RearWingDfMult=1.25
    EnableNode=WING_RACE
    DisableNode=WING_STOCK
  }
}
```

### Apply

```cpp
ks::vehicle::VehicleUpgradeSystem ups;
ups.loadFromIni(carDir + "/upgrades.ini"); // or loadDefaults()
ups.selectById(ks::vehicle::UpgradeCategory::Engine, "stage2");
ups.selectById(ks::vehicle::UpgradeCategory::Aero, "race_wing");

ks::vehicle::BaselineVehicleParams base{1200, 260, 8500, 0.35, 2.2};
ks::vehicle::applyUpgradesToVehicleSimulator(veh, base, ups);
```

---

## 2. Race component configs (KN5 node subsets)

**API:** `src/engine/vehicle/RaceComponentConfig.h`

The **KN5** holds the full car (complete node catalog).  
A sidecar **INI** turns nodes on/off for a specific race weekend.

### File naming

| File | Meaning |
|------|---------|
| `myCar_rd1.ini` | Round / race 1 |
| `myCar_rd1-2.ini` | Same layout for races 1 and 2 |
| `myCar_rd1-2-rd6.ini` | Races 1, 2 and 6 |
| `myCar_rd3.ini` | Race 3 only |

### INI example

```ini
[Meta]
Description=High DF package
Rounds=1,2

[Nodes]
GEO_WHEEL_LF=1
GEO_WHEEL_RF=1
GEO_WING_RACE=1
GEO_WING_STOCK=0
COCKPIT=1
; or:
; Active=GEO_WHEEL_LF,GEO_WING_RACE,COCKPIT
; Inactive=GEO_WING_STOCK
```

### Code

```cpp
auto cfg = ks::vehicle::RaceComponentConfigLoader::loadForRound("cars/myCar.kn5", /*round*/ 1);
auto visible = ks::vehicle::RaceComponentConfigLoader::filterNodes(catalog, cfg);
```

Upgrades can also force `EnableNode` / `DisableNode` (merged with race config by the app).

---

## Product identity

- Format-compatible ideas only; no AC/rF2 branding in UI.
- Adapters may map external content; ksim uses neutral names (Engine Package, Race wing).
