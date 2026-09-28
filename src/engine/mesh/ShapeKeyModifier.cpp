#include "ModifierSystem.h"
#include "ShapeKeyData.h"
#include <string>
#include <algorithm>
#include <cmath>

namespace ks {

ShapeKeyModifier::ShapeKeyModifier() : DeformModifier("Shape Key") {}

void ShapeKeyModifier::addTarget(const std::string& name) {
    ShapeKeyTarget t;
    t.name = name;
    targets.push_back(t);
}

void ShapeKeyModifier::removeTarget(int index) {
    if (index >= 0 && index < static_cast<int>(targets.size()))
        targets.erase(targets.begin() + index);
}

void ShapeKeyModifier::setTargetWeight(int index, float weight) {
    if (index >= 0 && index < static_cast<int>(targets.size())) {
        auto& t = targets[static_cast<size_t>(index)];
        t.weight = std::max(t.min, std::min(t.max, weight));
    }
}

MeshData ShapeKeyModifier::apply(const MeshData& input) {
    // Qt-free path: if MeshData shape-key fields are empty, passthrough.
    return input;
}

} // namespace ks
