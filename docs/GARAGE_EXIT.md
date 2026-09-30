# Uscita dal garage — logica dettagliata

## State machine

```text
InGarage
   │ requestLeave
   ▼
Preparing ──► (engine off + autoStart) ──► EngineStart
   │
   ▼
BoxClear ◄── path busy (wait / timeout → Blocked)
   │ path free
   ▼
RollingOut  (pit limiter ON, hold released)
   │ distance ≥ rollOutDistanceM
   ▼
PitLane     (limiter ON)
   │ along pit ≥ pitLaneLengthM
   ▼
TrackEntry  (limiter fade)
   │
   ▼
OnTrack

Returning ──► (near box + slow) ──► InGarage
Blocked   ──► recovery when condition clears + still requesting
```

## Condizioni di uscita

| Check | Effetto |
|-------|---------|
| Session Practice/Qualify | uscita sempre consentita |
| Session Race | solo se `allowExitInRace` e pit aperta |
| `pitLaneOpen` | altrimenti Blocked / PitClosed |
| Motore acceso | obbligatorio se `requireEngineOn` |
| Cono di uscita libero | altrimenti wait in BoxClear |
| `requestCancel` | torna InGarage |

## Output verso simulazione

| Flag | Uso |
|------|-----|
| `holdControls` | ignora gas in box |
| `snapToBox` | tiene la pose sul box |
| `pitLimiterActive` | applica `applyPitLimiter` |
| `engineShouldRun` | avvia motore |
| `allowDrive` | integra fisica |
| `statusText` | HUD |

## Esempio integrazione

```cpp
GarageExitController exit;
exit.bindBox(slot.garageIndex, boxPose, pitHeading);

GarageExitInput in;
in.requestLeave = playerPressedLeaveGarage;
in.engineRunning = …;
in.speedMs = …;
in.posX/Z = …;
in.pathBlocked = isExitPathBlocked(box, heading, 15.f, 2.5f, ox, oz, n, x, z);
in.session = SessionType::Practice;
in.pitLaneOpen = session.state().pitLaneOpen;

auto out = exit.update(dt, in);
if (out.holdControls) { throttle = brake = 0; }
if (out.snapToBox) { vehicle.setPose(out.boxPose); }
if (out.pitLimiterActive)
    throttle = GarageExitController::applyPitLimiter(speed, throttle, out.pitLimiterMaxMs);
```

## Config consigliata

```cpp
GarageExitConfig cfg;
cfg.pitLimiterKmh = 60.f;
cfg.rollOutDistanceM = 12.f;
cfg.pitLaneLengthM = 80.f;
cfg.prepareTimeoutSec = 10.f;
cfg.holdCarWhileInGarage = true;
cfg.autoStartEngineOnRequest = true;
```

File: `src/simulator/GarageExit.h`
