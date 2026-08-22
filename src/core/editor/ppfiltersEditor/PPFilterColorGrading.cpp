#include "PPFilterColorGrading.h"
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace ks {

PPFilterColorGrading::PPFilterColorGrading()
    : m_params(defaultParams())
{
}

void PPFilterColorGrading::setParams(const ColorGradingParams& params)
{
    m_params = params;
}

float PPFilterColorGrading::adjustChannel(float value, float lift, float gamma, float gain) const
{
    float v = clamp01(value);
    v += lift;
    v = std::pow(v, 1.0f / qMax(0.01f, gamma));
    v *= gain;
    return clamp01(v);
}

float PPFilterColorGrading::applyToneCurve(float value, const QVector<QPair<float, float>>& curve) const
{
    if (curve.isEmpty()) return value;

    float v = clamp01(value);

    if (v <= curve.first().first)
        return curve.first().second;
    if (v >= curve.last().first)
        return curve.last().second;

    for (int i = 0; i < curve.size() - 1; ++i) {
        if (v >= curve[i].first && v <= curve[i + 1].first) {
            float t = (v - curve[i].first) / qMax(0.0001f, curve[i + 1].first - curve[i].first);
            return curve[i].second + t * (curve[i + 1].second - curve[i].second);
        }
    }
    return v;
}

QVector<float> PPFilterColorGrading::generateLUT3D(int size) const
{
    int lutSize = qBound(8, size, 64);
    int totalEntries = lutSize * lutSize * lutSize;
    QVector<float> lut(totalEntries * 3);

    float invSize = 1.0f / (lutSize - 1);

    for (int b = 0; b < lutSize; ++b) {
        for (int g = 0; g < lutSize; ++g) {
            for (int r = 0; r < lutSize; ++r) {
                int idx = (b * lutSize + g) * lutSize + r;
                float ri = r * invSize;
                float gi = g * invSize;
                float bi = b * invSize;

                float outR = ri;
                float outG = gi;
                float outB = bi;

                outR = adjustChannel(outR, m_params.liftR, m_params.gammaR, m_params.gainR);
                outG = adjustChannel(outG, m_params.liftG, m_params.gammaG, m_params.gainG);
                outB = adjustChannel(outB, m_params.liftB, m_params.gammaB, m_params.gainB);

                outR = clamp01(outR * m_params.brightness);
                outG = clamp01(outG * m_params.brightness);
                outB = clamp01(outB * m_params.brightness);

                float lum = 0.2126f * outR + 0.7152f * outG + 0.0722f * outB;

                float satFactor = m_params.saturation + (1.0f - m_params.saturation) * (1.0f - m_params.vibrance);
                outR = lum + (outR - lum) * satFactor;
                outG = lum + (outG - lum) * satFactor;
                outB = lum + (outB - lum) * satFactor;

                outR = clamp01(outR + m_params.hueShift * 0.1f);
                outB = clamp01(outB - m_params.hueShift * 0.1f);

                outR = applyColorTemp(outR, m_params.temperature);
                outB = applyColorTemp(outB, -m_params.temperature);

                outR = applyToneCurve(outR, m_params.redCurve.isEmpty() ? m_params.masterCurve : m_params.redCurve);
                outG = applyToneCurve(outG, m_params.greenCurve.isEmpty() ? m_params.masterCurve : m_params.greenCurve);
                outB = applyToneCurve(outB, m_params.blueCurve.isEmpty() ? m_params.masterCurve : m_params.blueCurve);

                float contrastCenter = 0.5f;
                outR = contrastCenter + (outR - contrastCenter) * m_params.contrast;
                outG = contrastCenter + (outG - contrastCenter) * m_params.contrast;
                outB = contrastCenter + (outB - contrastCenter) * m_params.contrast;

                float shadowsWeight = qMax(0.0f, 1.0f - lum * 2.0f);
                float highlightsWeight = qMax(0.0f, (lum - 0.5f) * 2.0f);
                float splitBalance = m_params.balance * 0.5f + 0.5f;

                if (m_params.shadowsSat > 0) {
                    float ss = 1.0f + (m_params.shadowsSat - 1.0f) * shadowsWeight * (1.0f - splitBalance);
                    outR = lum + (outR - lum) * ss;
                    outG = lum + (outG - lum) * ss;
                    outB = lum + (outB - lum) * ss;
                }
                if (m_params.highlightsSat > 0) {
                    float hs = 1.0f + (m_params.highlightsSat - 1.0f) * highlightsWeight * splitBalance;
                    outR = lum + (outR - lum) * hs;
                    outG = lum + (outG - lum) * hs;
                    outB = lum + (outB - lum) * hs;
                }

                outR = clamp01(outR);
                outG = clamp01(outG);
                outB = clamp01(outB);

                if (m_params.lutIntensity < 1.0f) {
                    outR = ri + (outR - ri) * m_params.lutIntensity;
                    outG = gi + (outG - gi) * m_params.lutIntensity;
                    outB = bi + (outB - bi) * m_params.lutIntensity;
                }

                lut[idx * 3]     = clamp01(outR);
                lut[idx * 3 + 1] = clamp01(outG);
                lut[idx * 3 + 2] = clamp01(outB);
            }
        }
    }

    return lut;
}

bool PPFilterColorGrading::exportCubeLUT(const QString& path, int size) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setRealNumberPrecision(6);

    out << "TITLE \"ksEditor Color Grade\"\n";
    out << "LUT_3D_SIZE " << size << "\n";
    out << "DOMAIN_MIN 0.0 0.0 0.0\n";
    out << "DOMAIN_MAX 1.0 1.0 1.0\n\n";

    auto lut = generateLUT3D(size);
    int entries = size * size * size;
    for (int i = 0; i < entries; ++i) {
        out << lut[i * 3] << " " << lut[i * 3 + 1] << " " << lut[i * 3 + 2] << "\n";
    }

    file.close();
    return true;
}

bool PPFilterColorGrading::importCubeLUT(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    int lutSize = 0;
    QVector<float> values;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (line.startsWith("LUT_3D_SIZE", Qt::CaseInsensitive)) {
            lutSize = line.section(' ', 1).trimmed().toInt();
            continue;
        }

        if (line.startsWith("TITLE") || line.startsWith("DOMAIN"))
            continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            values.append(parts[0].toFloat());
            values.append(parts[1].toFloat());
            values.append(parts[2].toFloat());
        }
    }

    file.close();

    if (lutSize < 2 || values.isEmpty()) return false;

    int expected = lutSize * lutSize * lutSize * 3;
    if (values.size() != expected) return false;

    m_lutSize = lutSize;
    return true;
}

QVector<float> PPFilterColorGrading::applyToImage(const QVector<float>& rgb, int width, int height) const
{
    QVector<float> result;
    result.reserve(rgb.size());

    auto lut = generateLUT3D(33);
    int lutSize = 33;
    float invLut = 1.0f / (lutSize - 1);

    for (int i = 0; i + 2 < rgb.size(); i += 3) {
        float r = clamp01(rgb[i]);
        float g = clamp01(rgb[i + 1]);
        float b = clamp01(rgb[i + 2]);

        float ri = r * (lutSize - 1);
        float gi = g * (lutSize - 1);
        float bi = b * (lutSize - 1);

        int r0 = qMin((int)ri, lutSize - 2);
        int g0 = qMin((int)gi, lutSize - 2);
        int b0 = qMin((int)bi, lutSize - 2);
        float rf = ri - r0;
        float gf = gi - g0;
        float bf = bi - b0;

        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        for (int ch = 0; ch < 3; ++ch) {
            auto fetch = [&](int r, int g, int b) -> float {
                int idx = ((b * lutSize + g) * lutSize + r) * 3;
                return lut[idx + ch];
            };
            float v000 = fetch(r0, g0, b0);
            float v100 = fetch(r0 + 1, g0, b0);
            float v010 = fetch(r0, g0 + 1, b0);
            float v110 = fetch(r0 + 1, g0 + 1, b0);
            float v001 = fetch(r0, g0, b0 + 1);
            float v101 = fetch(r0 + 1, g0, b0 + 1);
            float v011 = fetch(r0, g0 + 1, b0 + 1);
            float v111 = fetch(r0 + 1, g0 + 1, b0 + 1);
            float v00 = lerp(v000, v100, rf);
            float v01 = lerp(v001, v101, rf);
            float v10 = lerp(v010, v110, rf);
            float v11 = lerp(v011, v111, rf);
            float v0 = lerp(v00, v10, gf);
            float v1 = lerp(v01, v11, gf);
            float v = lerp(v0, v1, bf);
            result.append(clamp01(v));
        }
    }

    return result;
}

QJsonObject PPFilterColorGrading::toJson() const
{
    QJsonObject obj;
    obj["exposure"] = m_params.exposure;
    obj["gamma"] = m_params.gamma;
    obj["contrast"] = m_params.contrast;
    obj["brightness"] = m_params.brightness;
    obj["saturation"] = m_params.saturation;
    obj["vibrance"] = m_params.vibrance;
    obj["hueShift"] = m_params.hueShift;
    obj["temperature"] = m_params.temperature;
    obj["tint"] = m_params.tint;
    obj["liftR"] = m_params.liftR; obj["liftG"] = m_params.liftG; obj["liftB"] = m_params.liftB;
    obj["gammaR"] = m_params.gammaR; obj["gammaG"] = m_params.gammaG; obj["gammaB"] = m_params.gammaB;
    obj["gainR"] = m_params.gainR; obj["gainG"] = m_params.gainG; obj["gainB"] = m_params.gainB;
    obj["shadowsHue"] = m_params.shadowsHue; obj["shadowsSat"] = m_params.shadowsSat;
    obj["highlightsHue"] = m_params.highlightsHue; obj["highlightsSat"] = m_params.highlightsSat;
    obj["balance"] = m_params.balance;
    obj["filmGrain"] = m_params.filmGrain;
    obj["sharpness"] = m_params.sharpness;
    obj["lutIntensity"] = m_params.lutIntensity;
    return obj;
}

bool PPFilterColorGrading::fromJson(const QJsonObject& json)
{
    if (json.isEmpty()) return false;
    m_params.exposure = json["exposure"].toDouble(m_params.exposure);
    m_params.gamma = json["gamma"].toDouble(m_params.gamma);
    m_params.contrast = json["contrast"].toDouble(m_params.contrast);
    m_params.brightness = json["brightness"].toDouble(m_params.brightness);
    m_params.saturation = json["saturation"].toDouble(m_params.saturation);
    m_params.vibrance = json["vibrance"].toDouble(m_params.vibrance);
    m_params.hueShift = json["hueShift"].toDouble(m_params.hueShift);
    m_params.temperature = json["temperature"].toDouble(m_params.temperature);
    m_params.tint = json["tint"].toDouble(m_params.tint);
    m_params.liftR = json["liftR"].toDouble(m_params.liftR);
    m_params.liftG = json["liftG"].toDouble(m_params.liftG);
    m_params.liftB = json["liftB"].toDouble(m_params.liftB);
    m_params.gammaR = json["gammaR"].toDouble(m_params.gammaR);
    m_params.gammaG = json["gammaG"].toDouble(m_params.gammaG);
    m_params.gammaB = json["gammaB"].toDouble(m_params.gammaB);
    m_params.gainR = json["gainR"].toDouble(m_params.gainR);
    m_params.gainG = json["gainG"].toDouble(m_params.gainG);
    m_params.gainB = json["gainB"].toDouble(m_params.gainB);
    m_params.shadowsHue = json["shadowsHue"].toDouble(m_params.shadowsHue);
    m_params.shadowsSat = json["shadowsSat"].toDouble(m_params.shadowsSat);
    m_params.highlightsHue = json["highlightsHue"].toDouble(m_params.highlightsHue);
    m_params.highlightsSat = json["highlightsSat"].toDouble(m_params.highlightsSat);
    m_params.balance = json["balance"].toDouble(m_params.balance);
    m_params.filmGrain = json["filmGrain"].toDouble(m_params.filmGrain);
    m_params.sharpness = json["sharpness"].toDouble(m_params.sharpness);
    m_params.lutIntensity = json["lutIntensity"].toDouble(m_params.lutIntensity);
    return true;
}

ColorGradingParams PPFilterColorGrading::defaultParams()
{
    ColorGradingParams p;
    p.gamma = 1.0f;
    p.contrast = 1.0f;
    p.brightness = 1.0f;
    p.saturation = 1.0f;
    p.gammaR = 1.0f; p.gammaG = 1.0f; p.gammaB = 1.0f;
    p.gainR = 1.0f; p.gainG = 1.0f; p.gainB = 1.0f;
    return p;
}

ColorGradingParams PPFilterColorGrading::cinematicParams()
{
    ColorGradingParams p = defaultParams();
    p.contrast = 1.15f;
    p.saturation = 0.85f;
    p.temperature = -8.0f;
    p.liftR = 0.01f; p.liftG = 0.005f; p.liftB = 0.02f;
    p.gammaR = 0.95f; p.gammaG = 1.0f; p.gammaB = 1.05f;
    p.gainR = 0.98f; p.gainG = 1.0f; p.gainB = 1.02f;
    p.shadowsHue = 0.05f; p.shadowsSat = 0.1f;
    p.highlightsHue = 0.0f; p.highlightsSat = 0.05f;
    return p;
}

ColorGradingParams PPFilterColorGrading::vividParams()
{
    ColorGradingParams p = defaultParams();
    p.saturation = 1.3f;
    p.vibrance = 0.3f;
    p.contrast = 1.1f;
    p.temperature = 3.0f;
    p.gainR = 1.02f; p.gainG = 1.0f; p.gainB = 0.98f;
    return p;
}

ColorGradingParams PPFilterColorGrading::vintageParams()
{
    ColorGradingParams p = defaultParams();
    p.saturation = 0.7f;
    p.contrast = 0.95f;
    p.temperature = 12.0f;
    p.gammaR = 1.08f; p.gammaG = 1.0f; p.gammaB = 0.92f;
    p.gainR = 1.05f; p.gainG = 0.98f; p.gainB = 0.92f;
    p.liftR = 0.02f; p.liftG = 0.0f; p.liftB = -0.01f;
    p.shadowsHue = 0.08f; p.shadowsSat = 0.15f;
    p.highlightsHue = 0.0f; p.highlightsSat = -0.05f;
    return p;
}

ColorGradingParams PPFilterColorGrading::moodyParams()
{
    ColorGradingParams p = defaultParams();
    p.contrast = 1.25f;
    p.saturation = 0.6f;
    p.temperature = -15.0f;
    p.brightness = 0.9f;
    p.liftR = -0.01f; p.liftB = 0.03f;
    p.gainR = 0.9f; p.gainB = 1.1f;
    p.shadowsSat = 0.2f;
    p.highlightsHue = -0.05f;
    return p;
}

float PPFilterColorGrading::applyColorTemp(float v, float temp) const
{
    if (temp == 0.0f) return v;
    float factor = temp * 0.005f;
    return clamp01(v + factor * (1.0f - v) * v);
}

float PPFilterColorGrading::applyVibrance(float value, float maxChannel, float vibrance) const
{
    if (vibrance == 0.0f) return value;
    float satLevel = maxChannel - qMin(value, maxChannel);
    float weight = 1.0f - satLevel;
    float boost = vibrance * weight;
    return clamp01(value + boost * (value > 0.5f ? 1.0f - value : value));
}

} // namespace ks
