# Damage telemetry integration

## Channels

| Output | Fields |
|--------|--------|
| **Shared memory** `carDamage[5]` | front, rear, left, right, overall (0..1 damage) |
| **UDP/TCP** packet v2 | overall, engineHealth, power/drag/DF mult, susp[4], warn |
| **Race HUD Engineer** | overall %, power mult, engine health bar |

## Mapping AC `carDamage[5]`

```
[0] front body  = avg(FL, FC, FR zones)
[1] rear body   = avg(RL, RC, RR)
[2] left body   = avg(FL, RL, LeftSide)
[3] right body  = avg(FR, RR, RightSide)
[4] overall     = DamageSystem::overallDamage()
```

## Helper

```cpp
auto d = ks::physics::sampleDamage(vehicle.damage());
// d.carDamage[5], d.powerMult, d.engineSeized, ...
```

## SimulationLoop

Each tick `publishSharedMemory` / `publishUdpTelemetry` / `syncUiFromVehicle` fill from `sampleDamage`.
