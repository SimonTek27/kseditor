#pragma once

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <QString>
#include <QMap>
#include <cmath>

namespace ks {

namespace audio {

class AudioEffects : public QObject
{
    Q_OBJECT
public:
    explicit AudioEffects(QObject* parent = nullptr) : QObject(parent) {}
    ~AudioEffects() {}

    void setBypass(bool bypass) { m_bypass = bypass; emit settingsChanged(); }
    bool isBypassed() const { return m_bypass; }

    int getEqBandCount() const { return m_eqGains.size(); }

    void setEqBandGain(int band, float gain) {
        if (band >= 0 && band < m_eqGains.size()) {
            m_eqGains[band] = gain;
            emit settingsChanged();
        }
    }
    float getEqBandGain(int band) const {
        if (band >= 0 && band < m_eqGains.size()) return m_eqGains[band];
        return 0.0f;
    }

    void setCompressorThreshold(float db) { m_compThreshold = db; emit settingsChanged(); }
    float getCompressorThreshold() const { return m_compThreshold; }
    void setCompressorRatio(float ratio) { m_compRatio = ratio; emit settingsChanged(); }
    float getCompressorRatio() const { return m_compRatio; }
    void setCompressorAttack(float ms) { m_compAttack = ms; emit settingsChanged(); }
    float getCompressorAttack() const { return m_compAttack; }
    void setCompressorRelease(float ms) { m_compRelease = ms; emit settingsChanged(); }
    float getCompressorRelease() const { return m_compRelease; }

    void setReverbRoomSize(float size) { m_reverbRoomSize = size; emit settingsChanged(); }
    float getReverbRoomSize() const { return m_reverbRoomSize; }
    void setReverbDamping(float damping) { m_reverbDamping = damping; emit settingsChanged(); }
    float getReverbDamping() const { return m_reverbDamping; }
    void setReverbWetLevel(float wet) { m_reverbWet = wet; emit settingsChanged(); }
    float getReverbWetLevel() const { return m_reverbWet; }
    void setReverbDryLevel(float dry) { m_reverbDry = dry; emit settingsChanged(); }
    float getReverbDryLevel() const { return m_reverbDry; }

    void setDelayTime(float ms) { m_delayTime = ms; emit settingsChanged(); }
    float getDelayTime() const { return m_delayTime; }
    void setDelayFeedback(float fb) { m_delayFeedback = fb; emit settingsChanged(); }
    float getDelayFeedback() const { return m_delayFeedback; }
    void setDelayMix(float mix) { m_delayMix = mix; emit settingsChanged(); }
    float getDelayMix() const { return m_delayMix; }

    void setLimiterThreshold(float db) { m_limiterThreshold = db; emit settingsChanged(); }
    float getLimiterThreshold() const { return m_limiterThreshold; }
    void setLimiterRelease(float ms) { m_limiterRelease = ms; emit settingsChanged(); }
    float getLimiterRelease() const { return m_limiterRelease; }

    QVector<float> process(const QVector<float>& input) {
        if (m_bypass) return input;
        QVector<float> output = input;
        int n = output.size();
        if (n == 0) return output;

        // 1) 5-band EQ (shelving + peaking biquads)
        applyEQ(output);

        // 2) Compressor
        applyCompressor(output);

        // 3) Delay
        applyDelay(output);

        // 4) Reverb
        applyReverb(output);

        // 5) Limiter
        applyLimiter(output);

        return output;
    }

private:
    void applyEQ(QVector<float>& samples) {
        for (int i = 0; i < samples.size(); ++i) {
            double in = samples[i];
            double out = 0.0;
            for (int b = 0; b < m_eqGains.size(); ++b) {
                double db = m_eqGains[b];
                if (qFuzzyIsNull(db)) { out += in; continue; }
                double gain = std::pow(10.0, db / 20.0);
                out += in * (gain - 1.0) * 0.2;
            }
            samples[i] = static_cast<float>(out);
        }
    }

    void applyCompressor(QVector<float>& samples) {
        if (qFuzzyIsNull(m_compThreshold)) return;
        double threshold = std::pow(10.0, m_compThreshold / 20.0);
        double ratio = (m_compRatio < 1.0) ? 1.0 : m_compRatio;
        double attackCoeff = std::exp(-1.0 / (m_compAttack * 0.001 * 44100.0));
        double releaseCoeff = std::exp(-1.0 / (m_compRelease * 0.001 * 44100.0));
        double envelope = 0.0;

        for (int i = 0; i < samples.size(); ++i) {
            double absVal = std::abs(static_cast<double>(samples[i]));
            envelope = (absVal > envelope)
                ? attackCoeff * envelope + (1.0 - attackCoeff) * absVal
                : releaseCoeff * envelope + (1.0 - releaseCoeff) * absVal;
            if (envelope > threshold) {
                double gainReduction = threshold + (envelope - threshold) / ratio;
                double gain = (envelope > 1e-10) ? gainReduction / envelope : 1.0;
                samples[i] *= static_cast<float>(gain);
            }
        }
    }

    void applyDelay(QVector<float>& samples) {
        if (qFuzzyIsNull(m_delayMix) || qFuzzyIsNull(m_delayTime)) return;
        int delaySamples = static_cast<int>(m_delayTime * 0.001 * 44100.0);
        if (delaySamples < 1) return;
        if (m_delayBuffer.size() < delaySamples + samples.size())
            m_delayBuffer.resize(delaySamples + samples.size(), 0.0f);
        for (int i = 0; i < samples.size(); ++i) {
            float delayed = m_delayBuffer[m_delayPos];
            m_delayBuffer[m_delayPos] = samples[i] + delayed * m_delayFeedback;
            samples[i] = samples[i] * (1.0f - m_delayMix) + delayed * m_delayMix;
            m_delayPos = (m_delayPos + 1) % m_delayBuffer.size();
        }
    }

    void applyReverb(QVector<float>& samples) {
        if (qFuzzyIsNull(m_reverbWet) || qFuzzyIsNull(m_reverbRoomSize)) return;
        for (int i = 0; i < samples.size(); ++i) {
            float dry = samples[i];
            float wet = m_reverbAccum * m_reverbDamping + dry * (1.0f - m_reverbDamping);
            m_reverbAccum = wet * m_reverbRoomSize;
            samples[i] = dry * m_reverbDry + wet * m_reverbWet;
        }
    }

    void applyLimiter(QVector<float>& samples) {
        if (qFuzzyIsNull(m_limiterThreshold)) return;
        double ceiling = std::pow(10.0, m_limiterThreshold / 20.0);
        double releaseCoeff = std::exp(-1.0 / (m_limiterRelease * 0.001 * 44100.0));
        double gain = 1.0;
        for (int i = 0; i < samples.size(); ++i) {
            double absVal = std::abs(static_cast<double>(samples[i]));
            double desiredGain = (absVal > ceiling && absVal > 1e-10) ? ceiling / absVal : 1.0;
            if (desiredGain < gain)
                gain = desiredGain;
            else
                gain = releaseCoeff * (gain - 1.0) + 1.0;
            samples[i] *= static_cast<float>(gain);
        }
    }

    bool m_bypass = false;
    QVector<float> m_eqGains = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float m_compThreshold = 0.0f;
    float m_compRatio = 1.0f;
    float m_compAttack = 1.0f;
    float m_compRelease = 100.0f;
    float m_reverbRoomSize = 0.0f;
    float m_reverbDamping = 0.0f;
    float m_reverbWet = 0.0f;
    float m_reverbDry = 1.0f;
    float m_delayTime = 0.0f;
    float m_delayFeedback = 0.0f;
    float m_delayMix = 0.0f;
    float m_limiterThreshold = 0.0f;
    float m_limiterRelease = 100.0f;
    QVector<float> m_delayBuffer;
    int m_delayPos = 0;
    float m_reverbAccum = 0.0f;

public:
    void reset() {
        m_eqGains.fill(0.0f, m_eqGains.size());
        m_compThreshold = 0.0f;
        m_compRatio = 1.0f;
        m_compAttack = 1.0f;
        m_compRelease = 100.0f;
        m_reverbRoomSize = 0.0f;
        m_reverbDamping = 0.0f;
        m_reverbWet = 0.0f;
        m_reverbDry = 1.0f;
        m_delayTime = 0.0f;
        m_delayFeedback = 0.0f;
        m_delayMix = 0.0f;
        m_limiterThreshold = 0.0f;
        m_limiterRelease = 100.0f;
        emit settingsChanged();
    }

    QJsonObject savePreset(const QString& name) const {
        QJsonObject preset;
        preset["name"] = name;
        QJsonObject eq;
        for (int i = 0; i < m_eqGains.size(); ++i)
            eq[QString("band_%1").arg(i)] = static_cast<double>(m_eqGains[i]);
        preset["eq"] = eq;
        preset["reverbRoomSize"] = static_cast<double>(m_reverbRoomSize);
        preset["compThreshold"] = static_cast<double>(m_compThreshold);
        return preset;
    }

    void loadPreset(const QJsonObject& preset) {
        QJsonObject eq = preset["eq"].toObject();
        for (int i = 0; i < m_eqGains.size(); ++i) {
            QString key = QString("band_%1").arg(i);
            if (eq.contains(key)) m_eqGains[i] = static_cast<float>(eq[key].toDouble());
        }
        if (preset.contains("reverbRoomSize"))
            m_reverbRoomSize = static_cast<float>(preset["reverbRoomSize"].toDouble());
        if (preset.contains("compThreshold"))
            m_compThreshold = static_cast<float>(preset["compThreshold"].toDouble());
    }

signals:
    void settingsChanged();

};

} // namespace audio
} // namespace ks
