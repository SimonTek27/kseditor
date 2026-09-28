# KSEngine Qt-free + architecture

**Updated:** 2026-09-28

## Engine scope
**ksengine** = generic open-source **simulator engine** (physics, devices, render,
loop). **Not** Assetto Corsa-specific.

**CSP** and other AC formats live in `src/adapters/assetto_corsa/` and link only
via optional target `ks_adapter_ac`.

See `docs/ENGINE_ARCHITECTURE.md`.

## Qt-free targets
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
# optional AC content:
# include(cmake/CMakeLists_assetto_adapter.cmake) && link ks_adapter_ac
```
