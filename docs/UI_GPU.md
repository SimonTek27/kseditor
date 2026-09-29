# UI GPU rendering

## Pipeline
```
NativeUiHub → UiRenderer (CPU verts + atlas UVs)
           → UiGpuPass::uploadFrame / draw
           → NativeRenderer::drawUi
```

## Shaders
- `ui/shaders/ui.vert.glsl` — pixel → NDC, push constant `screenSize`
- `ui/shaders/ui.frag.glsl` — sample R8 atlas as alpha, `outColor.a *= tex.r`

## Platform hooks (real Vulkan)
```cpp
auto* pass = loop.uiGpu();
pass->onUploadAtlas = [&](const uint8_t* r8, int w, int h) {
  // vkCreateImage R8_UNORM, staging copy
};
pass->onUploadDynamic = [&](const UiVertex* v, uint32_t vc,
                            const uint32_t* i, uint32_t ic) {
  // map host-visible VB/IB or staging
};
pass->onDraw = [&](uint32_t vc, uint32_t ic, float sw, float sh) {
  // bind pipeline, push screenSize, vkCmdDrawIndexed
};
```

## Limits
- 65k vertices / 98k indices per frame (grow later if needed)

## Soft path
Without hooks, `UiGpuPass` keeps CPU mirrors and increments soft draw counters — useful for headless tests.
