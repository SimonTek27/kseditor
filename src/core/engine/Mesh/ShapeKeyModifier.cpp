#include "ModifierSystem.h"
#include "ShapeKeyData.h"

namespace ks {

ShapeKeyModifier::ShapeKeyModifier() : DeformModifier("Shape Key") {}

void ShapeKeyModifier::addTarget(const QString& name) {
    ShapeKeyTarget t;
    t.name = name;
    targets.append(t);
}

void ShapeKeyModifier::removeTarget(int index) {
    if (index >= 0 && index < targets.size()) {
        targets.removeAt(index);
    }
}

void ShapeKeyModifier::setTargetWeight(int index, float weight) {
    if (index >= 0 && index < targets.size()) {
        targets[index].weight = qBound(targets[index].min, weight, targets[index].max);
    }
}

MeshData ShapeKeyModifier::apply(const MeshData& input) {
    if (input.shapeKeyNames.isEmpty() || input.shapeKeyDeltas.isEmpty()) {
        return input;
    }

    MeshData result = input;

    for (int i = 0; i < result.vertices.size(); ++i) {
        result.vertices[i].position = ShapeKeyManager::getBasePosition(result, i);
    }

    for (int k = 1; k < result.shapeKeyNames.size(); ++k) {
        if (result.shapeKeyMute.value(k, false)) continue;
        float w = result.shapeKeyWeights.value(k, 0.0f);
        if (qFuzzyIsNull(w)) continue;
        if (k < result.shapeKeyDeltas.size()) {
            const auto& deltas = result.shapeKeyDeltas[k];
            int n = qMin(deltas.size(), result.vertices.size());
            for (int i = 0; i < n; ++i) {
                result.vertices[i].position += deltas[i] * w;
            }
        }
    }

    for (const auto& target : targets) {
        if (target.mute || qFuzzyIsNull(target.weight)) continue;
        int idx = ShapeKeyManager::getShapeKeyIndexByName(result, target.name);
        if (idx < 0 || idx >= result.shapeKeyDeltas.size()) continue;
        const auto& deltas = result.shapeKeyDeltas[idx];
        int n = qMin(deltas.size(), result.vertices.size());
        for (int i = 0; i < n; ++i) {
            result.vertices[i].position += deltas[i] * target.weight;
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

QMap<QString, QVariant> ShapeKeyModifier::writeParameters() const {
    QMap<QString, QVariant> params;
    QVariantList targetsList;
    for (const auto& t : targets) {
        QVariantMap tm;
        tm["name"] = t.name;
        tm["weight"] = t.weight;
        tm["min"] = t.min;
        tm["max"] = t.max;
        tm["mute"] = t.mute;
        targetsList.append(tm);
    }
    params["targets"] = targetsList;
    return params;
}

void ShapeKeyModifier::readParameters(const QMap<QString, QVariant>& params) {
    if (params.contains("targets")) {
        targets.clear();
        for (const auto& tv : params["targets"].toList()) {
            QVariantMap tm = tv.toMap();
            ShapeKeyTarget t;
            t.name = tm["name"].toString();
            t.weight = tm["weight"].toFloat();
            t.min = tm["min"].toFloat();
            t.max = tm["max"].toFloat();
            t.mute = tm["mute"].toBool();
            targets.append(t);
        }
    }
}

} // namespace ks
