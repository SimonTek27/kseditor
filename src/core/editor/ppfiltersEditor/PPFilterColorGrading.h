#pragma once

#include <QString>
#include <QVector>
#include <QColor>
#include <QJsonObject>
#include <QPair>
#include <functional>

namespace ks {

struct ColorGradingParams {
    float exposure = 0.0f;
    float gamma = 1.0f;
    float contrast = 1.0f;
    float brightness = 1.0f;
    float saturation = 1.0f;
    float vibrance = 0.0f;

    float hueShift = 0.0f;
    float temperature = 0.0f;
    float tint = 0.0f;

    float liftR = 0.0f, liftG = 0.0f, liftB = 0.0f;
    float gammaR = 1.0f, gammaG = 1.0f, gammaB = 1.0f;
    float gainR = 1.0f, gainG = 1.0f, gainB = 1.0f;

    float shadowsHue = 0.0f, shadowsSat = 0.0f;
    float highlightsHue = 0.0f, highlightsSat = 0.0f;
    float balance = 0.0f;

    float filmGrain = 0.0f;
    float sharpness = 0.0f;

    // Tone curve points (normalized 0-1 input/output)
    QVector<QPair<float, float>> masterCurve;
    QVector<QPair<float, float>> redCurve;
    QVector<QPair<float, float>> greenCurve;
    QVector<QPair<float, float>> blueCurve;

    float lutIntensity = 1.0f;
};

class PPFilterColorGrading {
public:
    PPFilterColorGrading();

    void setParams(const ColorGradingParams& params);
    ColorGradingParams params() const { return m_params; }

    float adjustChannel(float value, float lift, float gamma, float gain) const;
    float applyToneCurve(float value, const QVector<QPair<float, float>>& curve) const;

    QVector<float> generateLUT3D(int size = 33) const;
    bool exportCubeLUT(const QString& path, int size = 33) const;
    bool importCubeLUT(const QString& path);

    QVector<float> applyToImage(const QVector<float>& rgb, int width, int height) const;

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    static ColorGradingParams defaultParams();
    static ColorGradingParams cinematicParams();
    static ColorGradingParams vividParams();
    static ColorGradingParams vintageParams();
    static ColorGradingParams moodyParams();

    int lutSize() const { return m_lutSize; }

private:
    float applyColorTemp(float v, float temp) const;
    float applyVibrance(float value, float maxChannel, float vibrance) const;
    float clamp01(float v) const { return qBound(0.0f, v, 1.0f); }
    QVector<float> interpolateLUT(const QVector<float>& src, int srcSize, int dstSize) const;

    ColorGradingParams m_params;
    int m_lutSize = 33;
};

} // namespace ks
