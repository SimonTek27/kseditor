# SimulatorApp ↔ AC + CSP parity

```
SimulatorApp  ≈  AC + CSP (open source)
       │
       ├── ksengine
       ├── adapters/ac
       └── network
```

| AC / CSP | Branch |
|----------|--------|
| Physics feel | ksengine |
| Content / SM / CSP files | adapters/ac |
| Multiplayer / UDP apps | network |
| Sessions / HUD / garage | SimulatorApp |

## Done
- Qt-free engine + sim
- Shared memory publisher, surfaces.ini, lap/sector
- NetworkManager / UDP telemetry present under simulator

## Next
1. Wire SM + surfaces + laps in SimulationLoop
2. AC content root + browser
3. Network: stable car-state protocol + lobby
4. KN5 scene; CSP flags in app render path
