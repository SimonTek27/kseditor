# Collisioni in pit lane

## Modello

- **Car–car**: OBB 2D (lunghezza × larghezza) su XZ, SAT + separazione soft + impulso
- **Car–muro**: corridoio ±`pitHalfWidthM` dall’asse pit
- Velocità tipiche basse → `restitution` 0.15, `maxImpulse` limitato

## Pipeline frame

```text
queue.updateCar / update
    ▼
syncPitCollisionFromQueue(col, queue, pitHeading)
    ▼
col.step(dt)          // contacts + resolve pose/vel
    ▼
applyPitCollisionResults → vehicle poses
    ▼
damageImpulseFor(id)  → DamageSystem se relSpeed > threshold
```

## Codice

```cpp
PitLaneCollision col;
col.setAxis(queue.axis());
PitLaneCollisionConfig cfg;
cfg.pitHalfWidthM = 3.5f;
cfg.damageSpeedThreshold = 2.5f;
col.setConfig(cfg);

col.onContact = [&](const PitCollisionContact& c) {
    if (c.relSpeed > cfg.damageSpeedThreshold)
        damage.applyImpulse(c.impulse, /*zone*/…);
};

// each car
col.upsert({ id, x, z, heading, vx, vz, 4.6f, 2.0f, mass });
col.step(dt);

// write back
for (auto& b : col.bodies())
    vehicle.setPosVel(b.carId, b.x, b.z, b.vx, b.vz);
```

## Parametri utili

| Campo | Default | Ruolo |
|-------|---------|--------|
| `pitHalfWidthM` | 3.5 | semi-larghezza corridoio |
| `minSpacing` (queue) | 8 | evita overlap prima del contatto |
| `restitution` | 0.15 | rimbalzo soft |
| `damageSpeedThreshold` | 2.5 m/s | sotto = solo push |

File: `src/simulator/PitLaneCollision.h`
