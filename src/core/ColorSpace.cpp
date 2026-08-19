#include "ColorSpace.h"
#include <cmath>

namespace ks {

// sRGB to linear conversion using the standard OETF
static float srgbToLinearComponent(float c) {
    float cNorm = c / 255.0f;
    return cNorm <= 0.04045f ? cNorm / 12.92f : std::pow((cNorm + 0.055f) / 1.055f, 2.4f);
}

// Linear to sRGB conversion using the standard inverse OETF
static float linearToSrgbComponent(float c) {
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

QColor ColorSpace::convert(const QColor& color, Space from, Space to) {
    if (from == to) return color;
    // Convert via the linear float path so both directions stay consistent.
    QVector3D linear = convert(QVector3D(color.redF(), color.greenF(), color.blueF()), from, Space::LINEAR);
    QVector3D out = convert(linear, Space::LINEAR, to);
    return QColor::fromRgbF(qBound(0.0f, out.x(), 1.0f),
                            qBound(0.0f, out.y(), 1.0f),
                            qBound(0.0f, out.z(), 1.0f));
}

QVector3D ColorSpace::convert(const QVector3D& v, Space from, Space to) {
    if (from == to) return v;

    QVector3D linear = v;
    if (from == Space::SRGB || from == Space::REC709) {
        linear = QVector3D(srgbToLinearComponent(v.x() * 255.0f),
                           srgbToLinearComponent(v.y() * 255.0f),
                           srgbToLinearComponent(v.z() * 255.0f));
    }
    // ACESCG and LINEAR are both treated as linear working spaces here.

    if (to == Space::SRGB || to == Space::REC709) {
        return QVector3D(linearToSrgbComponent(linear.x()),
                         linearToSrgbComponent(linear.y()),
                         linearToSrgbComponent(linear.z()));
    }
    return linear;
}

QColor ColorSpace::toSceneReferenced(const QColor& color) {
    return convert(color, Space::SRGB, Space::LINEAR);
}

QColor ColorSpace::fromSceneReferenced(const QColor& color) {
    return convert(color, Space::LINEAR, Space::SRGB);
}

ColorSpace::Space ColorSpace::displaySpace() {
    return Space::SRGB;
}

ColorSpace::Space ColorSpace::workingSpace() {
    return Space::SRGB;
}

} // namespace ks
