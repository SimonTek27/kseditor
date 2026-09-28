# engine/sim — native runtime (Qt-free)

Only low-level pieces used by `src/simulator`:

| File | Role |
|------|------|
| `NativeRenderer.h` (+ optional `.cpp`) | Vulkan device/swapchain/meshes |
| `GpuProfiler.*` | GPU timestamps |
| `UiRenderer.*` | batched UI verts |

**Do not** put `SimulatorApp`, `SimulationLoop`, or overlays here.
Those live under **`src/simulator/`**.
