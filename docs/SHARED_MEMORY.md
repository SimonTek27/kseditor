# Shared memory publish (ksim / ksengine)

Adapter path only — product identity remains **ksim**. Layout matches public AC pages so existing overlays can attach.

## Names

| Page | Windows | Linux (POSIX) |
|------|---------|----------------|
| Physics | `Local\\acpmf_physics` | `/acpmf_physics` |
| Graphics | `Local\\acpmf_graphics` | `/acpmf_graphics` |
| Static | `Local\\acpmf_static` | `/acpmf_static` |

## Flow

```
SimulationLoop::tick
  → publishSharedMemory()
      → AcLiveInput from VehicleSimulator + LapSectorTimer + weather
      → AcSharedMemoryPublisher::publish(live)
          → fillPhysics / fillGraphics / fillStatic
```

## Enable

```cpp
loop.setSharedMemoryEnabled(true); // default on after initialize()
```

Fallback: if mapping fails, soft in-process mirror still updates `physics()`/`graphics()` pointers for same-process readers/tests.

## Main fields published

**Physics:** gas, brake, fuel, gear, rpm, speed, velocity, G, tyre temps/wear/pressure, FFB, air/road temp, pit limiter  
**Graphics:** status, session, lap times (ms + wchar strings), position, sector, pit, spline, coordinates  
**Static:** car/track names, maxRpm, maxFuel, sectorCount, spline length  
