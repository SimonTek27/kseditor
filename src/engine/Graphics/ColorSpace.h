#pragma once
namespace ks { namespace engine { namespace graphics {
struct ColorSpace {
    static float linearToSrgb(float x);
    static float srgbToLinear(float x);
};
}}} // namespace
