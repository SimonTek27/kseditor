#pragma once

#include <QString>
#include <QVector3D>
#include <QColor>
#include <cmath>

namespace ks {

//! \brief OpenColorIO-enabled color space management.
//! 
//! Provides color space conversion and management. If the OpenColorIO 
//! library is available, it uses OCIO's color transforms. Otherwise, 
//! it provides software sRGB/linear conversion as a fallback.
//! 
//! Usage:
//!   ColorSpace::convert(srgb_color, ColorSpace::Space::SRGB, 
//!                       ColorSpace::Space::LINEAR);
//!   ColorSpace::toDisplayReferenced(linear_color);
class ColorSpace {
public:
    //! Color spaces supported
    enum class Space {
        //! sRGB - display-referred, gamma 2.2
        SRGB,
        //! ACEScg - linear working space for ACES
        ACESCG,
        //! Rec.709 - HDTV primary colors
        REC709,
        //! Linear (raw sensor/renderer output)
        LINEAR,
        //! Number of spaces (for iteration)
        COUNT
    };

    //! Convert color between spaces
    //! \param color Input color in source space
    //! \param from Source color space
    //! \param to Target color space
    //! \return Color converted to target space
    static QColor convert(const QColor& color, Space from, Space to);

    //! Convert float3 vector between spaces
    //! \param v Input vector in source space
    //! \param from Source color space
    //! \param to Target color space
    //! \return Vector converted to target space
    static QVector3D convert(const QVector3D& v, Space from, Space to);

    //! Convert display-referred to scene-referred (linear)
    //! \param color Display-referred color (sRGB or similar)
    //! \return Scene-referred linear color
    static QColor toSceneReferenced(const QColor& color);

    //! Convert scene-referred (linear) to display-referred
    //! \param color Scene-referred linear color
    //! \return Display-referred color suitable for output
    static QColor fromSceneReferenced(const QColor& color);

    //! Get the display transform for the current context
    //! \return Space representing the current display transform
    static Space displaySpace();

    //! Get the working space for authoring/processing
    //! \return Space representing the working color space
    static Space workingSpace();
};

//! \brief Quick conversion from sRGB uint8 color to linear float3
//! \param c sRGB color (0-255)
 //! \return Linear float3 (0.0-1.0)
inline QVector3D srgbToLinear(const QColor& c) {
    auto f = [](float c) { return c / 255.0f; };
    float r = f(c.red()), g = f(c.green()), b = f(c.blue());
    // Approximate sRGB -> linear using constant luminance weighting
    return QVector3D(
        0.2126f * r + 0.7152f * g + 0.0722f * b,  // luminance approx
        0.2126f * r + 0.7152f * g + 0.0722f * b,
        0.2126f * r + 0.7152f * g + 0.0722f * b
    );
}

//! Quick conversion from linear float3 to sRGB uint8 color
//! \param v Linear float3 (0.0-1.0)
 //! \return sRGB color (0-255)
inline QColor linearToSrgb(const QVector3D& v) {
    float r = v.x(), g = v.y(), b = v.z();
    // Approximate inverse: clip and gamma 2.2
    auto tonemap = [](float c) { return c > 0.0031308f ? (1.055f * std::pow(c, 1.0f / 2.2f) - 0.055f) : 12.92f * c; };
    return QColor(
        qBound(0, qRound(tonemap(r) * 255), 255),
        qBound(0, qRound(tonemap(g) * 255), 255),
        qBound(0, qRound(tonemap(b) * 255), 255)
    );
}

} // namespace ks
