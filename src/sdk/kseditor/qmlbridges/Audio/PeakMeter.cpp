#include "PeakMeter.h"
#include <cmath>

PeakMeter::PeakMeter(QObject *parent)
    : QObject(parent)
{
    m_peak.fill(0.0f, m_channels);
    m_rms.fill(0.0f, m_channels);
}

void PeakMeter::setSampleRate(int sampleRate)
{
    m_sampleRate = sampleRate > 0 ? sampleRate : 44100;
}

void PeakMeter::setChannelCount(int channels)
{
    m_channels = channels > 0 ? channels : 2;
    m_peak.resize(m_channels);
    m_rms.resize(m_channels);
}

void PeakMeter::setPeakDecay(float decayMs)
{
    m_peakDecayMs = decayMs > 0.0f ? decayMs : 1000.0f;
}

void PeakMeter::process(const float *samples, int frameCount)
{
    if (!samples || frameCount <= 0)
        return;

    const int ch = m_channels > 0 ? m_channels : 2;
    const float decayPerSample = std::exp(-1.0f / (m_peakDecayMs * 0.001f * static_cast<float>(m_sampleRate) * static_cast<float>(frameCount)));

    for (int c = 0; c < ch; ++c) {
        float peak = 0.0f;
        float sumSq = 0.0f;
        for (int i = 0; i < frameCount; ++i) {
            const float s = std::abs(samples[i * ch + c]);
            if (s > peak) peak = s;
            sumSq += s * s;
        }
        m_peak[c] = std::max(peak, m_peak[c] * decayPerSample);
        m_rms[c] = std::sqrt(sumSq / static_cast<float>(frameCount));
    }

    const float left = m_peak.value(0, 0.0f);
    const float right = m_peak.value(1, left);
    emit levelsChanged(left, right);
}

void PeakMeter::process(const QVector<float> &samples)
{
    if (samples.isEmpty())
        return;
    const int ch = m_channels > 0 ? m_channels : 2;
    const int frames = samples.size() / ch;
    if (frames > 0)
        process(samples.constData(), frames);
}

float PeakMeter::getPeakLevel(int channel) const
{
    return m_peak.value(channel, 0.0f);
}

float PeakMeter::getRMSLevel(int channel) const
{
    return m_rms.value(channel, 0.0f);
}

float PeakMeter::getLeftRMS() const
{
    return getRMSLevel(0);
}

float PeakMeter::getRightRMS() const
{
    return getRMSLevel(m_channels > 1 ? 1 : 0);
}
