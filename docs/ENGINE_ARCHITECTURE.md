# Architecture: ksengine vs SimulatorApp (ksim)

## Product tree

```
SimulatorApp (ksim)     independent open-source racing simulator
       │
       ├── ksengine          generic simulation runtime
       ├── adapters/content  content format loaders (cars, tracks, surfaces, …)
       └── network           multiplayer / telemetry transport
```

| Branch | Role |
|--------|------|
| **ksim / SimulatorApp** | Standalone product: sessions, HUD, garage, multiplayer, content UX |
| **ksengine** | Generic physics, FFB, Vulkan, tick, NativeUi — no game branding |
| **adapters/content** | File-format bridges (INI, meshes, banks, shared-memory layouts). *May* interoperate with third-party content trees that use the same on-disk layouts; the product does **not** depend on any commercial title’s name, license, or binaries |
| **network** | MP, UDP/TCP, session sync |
| **kseditor** | Qt authoring tool (separate) |

## Independence rule

- **ksim is its own product.** It does not present itself as a rebrand, fork, or official companion of any commercial simulator.
- Compatibility is **technical only**: reading the same *file layouts* where useful for modders and tooling.
- Source, docs, window titles, and public messaging avoid tying the product identity to a third-party trademark.
- Core engine never hard-requires a specific commercial content install.

```
SimulatorApp  →  ksengine
SimulatorApp  →  adapters/content   (optional format packs)
SimulatorApp  →  network
ksengine      ↛  adapters/*
```

## Qt-free
`ksengine` + `ksimulator`: build without Qt (`KSENGINE_QT_FREE=1`).
