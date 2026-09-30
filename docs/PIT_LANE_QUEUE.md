# Pit lane queue

## Ruoli

| Role | Significato |
|------|-------------|
| **Leaving** | dal box verso pista |
| **Entering** | dalla pista verso il box |
| **Holding** | fermo in servizio |

## Stati

| Status | Significato |
|--------|-------------|
| **Waiting** | in coda, non può muoversi |
| **ClearedToMove** | gap libero, può partire |
| **Moving** | in movimento sulla pit |
| **Done** | uscito dalla coda |

## Regole

1. **Uscita garage**: `requestLeave` → solo il primo con gap ≥ `releaseGapM` (default 12 m) è **Cleared**.
2. **Spaziatura**: minimo `minSpacingM` (8 m) sull’asse pit.
3. **Priorità**: player boost + garage index più basso preferito in leave.
4. **Ingresso**: FIFO lungo l’asse; bloccato se qualcuno è entro min spacing.
5. **Garage exit**: `shouldBlockGarageExit` → `pathBlocked` finché non Cleared.

## Flusso con GarageExit

```text
requestLeave(carId)
    ▼
GarageExit: Preparing / BoxClear
    ▼  pathBlocked = queue.shouldBlockGarageExit()
ClearedToMove
    ▼
RollingOut / PitLane → markMoving, limiter da suggestedMaxSpeedMs
    ▼
OnTrack → markDone
```

## Codice

```cpp
PitLaneQueue queue;
queue.setAxis({ pitOriginX, pitOriginZ, pitHeading });
queue.setConfig(cfg);

// player vuole uscire
queue.requestLeave(playerId, raceNum, garageIdx, true, x, z);

queue.setSimTime(t);
queue.updateCar(id, x, z, speed);
queue.update(dt);

in.pathBlocked = queue.shouldBlockGarageExit(id);
auto out = exitCtrl.update(dt, in);
integratePitQueueWithGarageExit(queue, exitCtrl, id, in, throttle);
```

File: `src/simulator/PitLaneQueue.h`
