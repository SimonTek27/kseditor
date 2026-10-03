# Pit lane stack wiring

## Tick order (inside physics step)

```
updateLapAndSurface
m_features.tick(...)
updatePitLane(dt)      // queue + OBB collision  ← call BEFORE garage
updateGarageExit(dt)   // uses pathBlocked from queue/collision
```

Add in `SimulationLoop.cpp` after FeatureHub tick:

```cpp
updatePitLane((float)m_physicsDt);
updateGarageExit((float)m_physicsDt);
```

## Data flow

```
GarageExit phase Preparing / RollingOut / PitLane
        │
        ▼
PitLaneQueue.requestLeave(player) + updateCar
        │
        ▼
shouldBlockGarageExit ──► GarageExitInput.pathBlocked
        │
        ▼
PitLaneCollision.upsert(body) → step → contacts
        │
        ├── suggestedMaxSpeedMs → throttle/brake soft cap
        └── damageImpulseFor → RaceSession penalty note
```

## Modules

| Module | Role |
|--------|------|
| **PitLaneQueue** | Spacing ≥8 m, release gap, ClearedToMove, leave/enter |
| **PitLaneCollision** | 2D OBB car–car + corridor walls, soft restitution |
| **GarageExit** | InGarage → … → OnTrack state machine |

## Session reset

`beginSession` rebuilds queue + collision and sets garage phase from `startInGarage`.
