#include "ColorSpace.h"
#include <cmath>
namespace ks { namespace engine { namespace graphics {
float ColorSpace::linearToSrgb(float x) {
    return x <= 0.0031308f ? 12.92f * x : 1.055f * std::pow(x, 1.f/2.4f) - 0.055f;
}
float ColorSpace::srgbToLinear(float x) {
    return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
}
}}} // namespace
