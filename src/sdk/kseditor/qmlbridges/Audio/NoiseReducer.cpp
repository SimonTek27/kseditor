#include "NoiseReducer.h"
#include <cmath>

NoiseReducer::NoiseReducer(QObject *parent)
    : QObject(parent)
{
}

void NoiseReducer::captureNoiseProfile(const QVector<float> &samples)
{
    if (samples.isEmpty()) {
        m_hasProfile = false;
        m_noiseProfile.clear();
        return;
    }

    const int n = samples.size();
    m_noiseProfile.resize(n);
    for (int i = 0; i < n; ++i)
        m_noiseProfile[i] = std::abs(samples[i]);

    m_hasProfile = true;
}

void NoiseReducer::setReductionAmount(float amount)
{
    m_reductionAmount = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
}

QVector<float> NoiseReducer::reduceNoise(const QVector<float> &samples) const
{
    if (!m_hasProfile || m_noiseProfile.isEmpty() || m_reductionAmount <= 0.0f)
        return samples;

    QVector<float> out(samples.size());
    const int n = samples.size();
    const int pn = m_noiseProfile.size();
    for (int i = 0; i < n; ++i) {
        const float noiseFloor = m_noiseProfile[i % pn] * m_reductionAmount;
        const float s = samples[i];
        if (std::abs(s) <= noiseFloor)
            out[i] = 0.0f;
        else
            out[i] = s > 0.0f ? (s - noiseFloor) : (s + noiseFloor);
    }
    return out;
}

bool NoiseReducer::hasProfile() const
{
    return m_hasProfile;
}
