# KSEngine Qt-free

**Updated:** 2026-09-29

## Status
`src/engine` link-set scan: **no real Qt includes** (only historical false-positive comments cleaned).

## Hardening
- `KsQtFreeGuard.h` — `#error` if `QT_VERSION` appears under `KSENGINE_QT_FREE`
- `Engine.h` includes the guard
- CMake: `AUTOMOC/AUTOUIC/AUTORCC OFF`, no Qt link, excludes `.ui`/`.qrc`/editor monoliths

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON ..
cmake --build . --target ksengine
```

## Not in ksengine (by design)
Editor (`MainWindow`, `src/sdk`), 3dprint/scanners, Blueprint UI, AC adapters.
