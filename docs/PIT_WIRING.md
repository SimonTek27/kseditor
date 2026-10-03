# Pit lane stack wiring

## Tick order (inside physics step)

```
updateLapAndSurface
m_features.tick(...)
updatePitLane(dt)      // queue + OBB collision FIRST
updateGarageExit(dt)   // pathBlocked from queue/collision
```

In `SimulationLoop.cpp` after FeatureHub tick:

```cpp
updatePitLane((float)m_physicsDt);
updateGarageExit((float)m_physicsDt);
```

Also in `initialize()`:

```cpp
startFeatureServices(false);
configurePitAxis(0.f, 0.f, 0.f, 120.f); // until track provides real pit spline
```

## configurePitAxis

Call after loading track/garage boxes with the real pit corridor:

```cpp
loop.configurePitAxis(pitStartX, pitStartZ, pitHeadingRad, pitLengthM);
```

Sets the same `PitAxis` on both `PitLaneQueue` and `PitLaneCollision`.

## Data flow

```
GarageExit Preparing/RollingOut/PitLane
    → PitLaneQueue.requestLeave + updateCar
    → shouldBlockGarageExit → pathBlocked
    → PitLaneCollision.upsert + step
    → suggestedMaxSpeedMs / damageImpulse
```

## Modules

| Module | Role |
|--------|------|
| PitLaneQueue | spacing ≥8 m, ClearedToMove, leave/enter |
| PitLaneCollision | 2D OBB + corridor walls |
| GarageExit | box state machine |
| configurePitAxis | shared axis for queue + collision |
