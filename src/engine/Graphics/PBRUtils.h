#pragma once
#include "GfxTypes.h"
#include <cmath>

namespace ks {
namespace engine {
namespace graphics {

struct PBRUtils {
    static float distributionGGX(float NdotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float d = (NdotH * NdotH) * (a2 - 1.f) + 1.f;
        return a2 / (3.14159265f * d * d + 1e-7f);
    }
    static Vec3 fresnelSchlick(float cosTheta, const Vec3& F0) {
        float f = std::pow(1.f - cosTheta, 5.f);
        return Vec3{F0.x + (1.f - F0.x) * f, F0.y + (1.f - F0.y) * f, F0.z + (1.f - F0.z) * f};
    }
};

} // namespace graphics
} // namespace engine
} // namespace ks
