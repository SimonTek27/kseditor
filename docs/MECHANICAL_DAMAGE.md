# Danni meccanici veicolo

## Due layer

1. **Impact** — `DamageSystem::processCollision` / `applyMechanicalImpact` (zone body + engine/susp/aero)
2. **Wear continuo** — `applyMechanicalWear` ogni frame da telemetria

## Wear continuo

| Sistema | Trigger | Effetto |
|---------|---------|---------|
| **Motore** | over-rev, surriscaldo, olio basso | health↓, powerLoss, seize |
| **Cambio** | frizione in slip, power-shift | clutchDamage, gearDamage, stuck |
| **Sospensioni** | bottom-out, cordoli | geometry, arm, toe/camber, broken |
| **Freni** | pad wear, temp | fade, discDamage |

## Impatto

```cpp
applyMechanicalImpact(dmg, energy, localX, localY, localZ, nx, ny, nz);
// oppure da pit:
applyPitContactDamage(dmg, contact.impulse, mass, localX, localZ);
```

## Loop simulazione

```cpp
// collisioni / muri / pit
if (hit) applyMechanicalImpact(...);

// ogni frame
MechTelemetry t;
t.rpm = …; t.maxRpm = …; t.coolantTempC = …;
t.brakeTempC[i] = …; t.suspensionTravel[i] = …; t.curbLoad[i] = …;
applyMechanicalWear(damageSystem, dt, t);

auto m = sampleMechanicalEffects(damageSystem);
vehicle.setPowerScale(m.power);
vehicle.setAeroScale(m.downforce, m.drag);
vehicle.setBrakeScale(m.braking);
if (m.engineDead) cutIgnition();
```

## File

- `src/engine/physics/DamageSystem.h/.cpp` — zone + componenti + repair
- `src/engine/physics/MechanicalDamage.h` — wear + impact bridge
- `docs/DAMAGE_RF2_STYLE.md` — overview precedente
