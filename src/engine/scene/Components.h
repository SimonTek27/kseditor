#pragma once
// Engine-level scene components shared by SimulatorApp, kseditor and any
// other ks::Engine consumer. Nothing here knows about Vulkan, Qt or a
// concrete renderer — only ks::math and std::string.
#include "Registry.h"
#include "../Math/MathTypesFree.h"
#include <string>

namespace ks::ecs {

struct Name {
    std::string value;
};

struct Tag {
    std::string value;
};

struct Transform {
    math::vec3 position;
    math::vec3 rotation;
    math::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MeshInstance {
    std::string meshName;
};

inline math::mat4 worldMatrix(const Transform& t) {
    math::mat4 m = math::mat4::fromPositionRollPitchYaw(
        t.position, t.rotation.x, t.rotation.y, t.rotation.z);
    m(0, 0) *= t.scale.x;
    m(0, 1) *= t.scale.x;
    m(0, 2) *= t.scale.x;
    m(1, 0) *= t.scale.y;
    m(1, 1) *= t.scale.y;
    m(1, 2) *= t.scale.y;
    m(2, 0) *= t.scale.z;
    m(2, 1) *= t.scale.z;
    m(2, 2) *= t.scale.z;
    return m;
}

} // namespace ks::ecs
