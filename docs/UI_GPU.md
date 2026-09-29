# UI GPU + Vulkan pipeline

## Classes
| Class | Role |
|-------|------|
| `UiGpuPass` | CPU batch + hooks |
| `VulkanUiPipeline` | Vk pipeline, atlas R8, host VB/IB, draw |
| `NativeRenderer::drawUi` | calls upload + draw |

## Bootstrap
```cpp
ks::sim::ui::VulkanUiPipeline pipe;
ks::sim::ui::VulkanUiPipeline::CreateInfo ci;
ci.physicalDevice = phys;
ci.device = device;
ci.queue = graphicsQueue;
ci.renderPass = mainPass;   // must allow blending
ci.queueFamily = graphicsFamily;
ci.vertSpvPath = "shaders/ui.vert.spv";
ci.fragSpvPath = "shaders/ui.frag.spv";
if (!pipe.create(ci)) { /* error */ }

pipe.bindTo(*loop.uiGpu());
// each frame, before UI draw:
pipe.setCommandBuffer(cmd);
```

## Compile shaders
```bash
cd src/simulator/ui && bash compile_ui_shaders.sh
```

## Pipeline state
- Topology: triangle list
- Blend: SRC_ALPHA / ONE_MINUS_SRC_ALPHA
- Depth: off
- Cull: none
- Push constant: `vec2 screenSize`
- Descriptor 0: combined image sampler (R8 atlas, nearest)
- Vertex: `UiVertex` { xy, rgba, uv }

## Frame order
```
beginRenderPass
  draw 3D…
  uiPass.uploadFrame(uiRenderer)  // maps host VB/IB
  uiPass.draw()                   // vkCmdDrawIndexed via VulkanUiPipeline
endRenderPass
```
