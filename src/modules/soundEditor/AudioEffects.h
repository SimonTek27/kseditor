#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QtCore/qmath.h>
#include <cmath>

namespace ks {

namespace audio {

class ChorusEffect : public QObject
{
    Q_OBJECT
public:
    explicit ChorusEffect(QObject* parent = nullptr) : QObject(parent), m_delayBuffer(MAX_DELAY_SAMPLES, 0.0f) {}
    ~ChorusEffect() {}

    void setRate(float hz) { m_rate = qBound(0.1f, hz, 20.0f); }
    float rate() const { return m_rate; }
    void setDepth(float depth) { m_depth = qBound(0.0f, depth, 1.0f); }
    float depth() const { return m_depth; }
    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float lfoIncrement = 2.0f * M_PI * m_rate / sampleRate;
        int maxDelaySamples = static_cast<int>(30.0f * sampleRate / 1000.0f);

        for (int i = 0; i < input.size(); ++i) {
            float lfoValue = sinf(m_lfoPhase);
            m_lfoPhase += lfoIncrement;
            if (m_lfoPhase > 2.0f * M_PI) m_lfoPhase -= 2.0f * M_PI;

            float delayMs = 20.0f + lfoValue * m_depth * 10.0f;
            int delaySamples = static_cast<int>(delayMs * sampleRate / 1000.0f);
            delaySamples = qMin(delaySamples, maxDelaySamples);

            int readPos = m_writePos - delaySamples;
            if (readPos < 0) readPos += MAX_DELAY_SAMPLES;

            float delayed = m_delayBuffer[readPos];
            m_delayBuffer[m_writePos] = input[i];
            m_writePos = (m_writePos + 1) % MAX_DELAY_SAMPLES;

            output[i] = input[i] * (1.0f - m_mix) + delayed * m_mix;
        }
        return output;
    }

private:
    float m_rate = 1.5f;
    float m_depth = 0.5f;
    float m_mix = 0.5f;
    static const int MAX_DELAY_SAMPLES = 4096;
    QVector<float> m_delayBuffer;
    int m_writePos = 0;
    float m_lfoPhase = 0.0f;
};

class FlangerEffect : public QObject
{
    Q_OBJECT
public:
    explicit FlangerEffect(QObject* parent = nullptr) : QObject(parent), m_delayBuffer(MAX_DELAY, 0.0f) {}
    ~FlangerEffect() {}

    void setRate(float hz) { m_rate = qBound(0.1f, hz, 10.0f); }
    float rate() const { return m_rate; }
    void setDepth(float depth) { m_depth = qBound(0.0f, depth, 1.0f); }
    float depth() const { return m_depth; }
    void setFeedback(float feedback) { m_feedback = qBound(-0.95f, feedback, 0.95f); }
    float feedback() const { return m_feedback; }
    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }
    void setDelay(float ms) { m_delay = qBound(0.0f, ms, 20.0f); }
    float delay() const { return m_delay; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float lfoIncrement = 2.0f * M_PI * m_rate / sampleRate;
        int baseDelaySamples = static_cast<int>(m_delay * sampleRate / 1000.0f);
        int maxDepthSamples = static_cast<int>(m_depth * 10.0f * sampleRate / 1000.0f);

        for (int i = 0; i < input.size(); ++i) {
            float lfoValue = sinf(m_lfoPhase);
            m_lfoPhase += lfoIncrement;
            if (m_lfoPhase > 2.0f * M_PI) m_lfoPhase -= 2.0f * M_PI;

            int delaySamples = baseDelaySamples + static_cast<int>(lfoValue * maxDepthSamples);
            delaySamples = qBound(0, delaySamples, MAX_DELAY - 1);

            int readPos = m_writePos - delaySamples;
            if (readPos < 0) readPos += MAX_DELAY;

            float delayed = m_delayBuffer[readPos];
            float fb = delayed * m_feedback;
            m_delayBuffer[m_writePos] = input[i] + fb;
            m_writePos = (m_writePos + 1) % MAX_DELAY;

            output[i] = input[i] * (1.0f - m_mix) + delayed * m_mix;
        }
        return output;
    }

private:
    float m_rate = 0.5f;
    float m_depth = 0.5f;
    float m_feedback = 0.5f;
    float m_mix = 0.5f;
    float m_delay = 5.0f;
    static const int MAX_DELAY = 4096;
    QVector<float> m_delayBuffer;
    int m_writePos = 0;
    float m_lfoPhase = 0.0f;
};

class NoiseGate : public QObject
{
    Q_OBJECT
public:
    explicit NoiseGate(QObject* parent = nullptr) : QObject(parent) {}

    void setThreshold(float db) { m_threshold = db; }
    float threshold() const { return m_threshold; }
    void setRatio(float ratio) { m_ratio = qBound(1.0f, ratio, 20.0f); }
    float ratio() const { return m_ratio; }
    void setAttack(float ms) { m_attack = qBound(0.1f, ms, 100.0f); }
    float attack() const { return m_attack; }
    void setRelease(float ms) { m_release = qBound(10.0f, ms, 1000.0f); }
    float release() const { return m_release; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float attackCoeff = expf(-1.0f / (m_attack * sampleRate / 1000.0f));
        float releaseCoeff = expf(-1.0f / (m_release * sampleRate / 1000.0f));

        for (int i = 0; i < input.size(); ++i) {
            float db = 20.0f * log10f(qAbs(input[i]) + 1e-10f);
            float targetGain = (db > m_threshold) ? 1.0f : 0.0f;
            m_envelope = (m_envelope < targetGain) ? attackCoeff * m_envelope + (1.0f - attackCoeff) * targetGain
                                                    : releaseCoeff * m_envelope + (1.0f - releaseCoeff) * targetGain;
            output[i] = input[i] * m_envelope;
        }
        return output;
    }

private:
    float m_threshold = -40.0f;
    float m_ratio = 10.0f;
    float m_attack = 5.0f;
    float m_release = 100.0f;
    float m_envelope = 0.0f;
};

class NoiseReduction : public QObject
{
    Q_OBJECT
public:
    explicit NoiseReduction(QObject* parent = nullptr) : QObject(parent) {}

    void setNoiseFloor(float db) { m_noiseFloor = db; }
    float noiseFloor() const { return m_noiseFloor; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        Q_UNUSED(sampleRate)
        QVector<float> output(input.size());
        float thresholdLinear = powf(10.0f, m_noiseFloor / 20.0f);

        for (int i = 0; i < input.size(); ++i) {
            float absVal = qAbs(input[i]);
            output[i] = (absVal > thresholdLinear) ? input[i] : input[i] * 0.1f;
        }
        return output;
    }

private:
    float m_noiseFloor = -60.0f;
};

class PitchShifter : public QObject
{
    Q_OBJECT
public:
    explicit PitchShifter(QObject* parent = nullptr) : QObject(parent) {}

    void setPitch(float semitones) { m_pitch = qBound(-12.0f, semitones, 12.0f); }
    float pitch() const { return m_pitch; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        float ratio = powf(2.0f, m_pitch / 12.0f);
        int outputSize = static_cast<int>(input.size() / ratio);
        QVector<float> output(outputSize);

        for (int i = 0; i < outputSize; ++i) {
            float srcPos = i * ratio;
            int idx = static_cast<int>(srcPos);
            float frac = srcPos - idx;
            if (idx >= input.size()) {
                output[i] = 0.0f;
            } else if (idx < input.size() - 1) {
                output[i] = input[idx] * (1.0f - frac) + input[idx + 1] * frac;
            } else {
                output[i] = input[idx];
            }
        }
        return output;
    }

private:
    float m_pitch = 0.0f;
};

class SFXReverb : public QObject
{
    Q_OBJECT
public:
    explicit SFXReverb(QObject* parent = nullptr) : QObject(parent) {
        m_combs.resize(8);
        m_alls.resize(4);
        for (int i = 0; i < 8; ++i) m_combs[i].resize(m_combDelays[i]);
        for (int i = 0; i < 4; ++i) m_alls[i].resize(m_allDelays[i]);
    }

    void setRoomSize(float size) { m_roomSize = qBound(0.0f, size, 1.0f); }
    float roomSize() const { return m_roomSize; }
    void setDamping(float damping) { m_damping = qBound(0.0f, damping, 1.0f); }
    float damping() const { return m_damping; }
    void setWetLevel(float wet) { m_wet = qBound(0.0f, wet, 1.0f); }
    float wetLevel() const { return m_wet; }
    void setDryLevel(float dry) { m_dry = qBound(0.0f, dry, 1.0f); }
    float dryLevel() const { return m_dry; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        Q_UNUSED(sampleRate)
        QVector<float> output(input.size(), 0.0f);

        for (int i = 0; i < input.size(); ++i) {
            float sample = input[i];
            float wet = 0.0f;

            for (int c = 0; c < 8; ++c) {
                float& buf = m_combs[c][m_combPos[c]];
                float temp = buf;
                buf = temp * m_damping + sample;
                wet += temp;
                m_combPos[c] = (m_combPos[c] + 1) % m_combs[c].size();
            }

            for (int a = 0; a < 4; ++a) {
                float& buf = m_alls[a][m_allPos[a]];
                float temp = buf;
                buf = temp * 0.5f + sample;
                wet += temp;
                m_allPos[a] = (m_allPos[a] + 1) % m_alls[a].size();
            }

            output[i] = sample * m_dry + wet * m_wet * 0.125f;
        }
        return output;
    }

private:
    float m_roomSize = 0.5f;
    float m_damping = 0.5f;
    float m_wet = 0.3f;
    float m_dry = 0.7f;

    int m_combPos[8] = {0};
    int m_allPos[4] = {0};

    const int m_combDelays[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    const int m_allDelays[4] = {556, 441, 341, 225};

    QVector<QVector<float>> m_combs;
    QVector<QVector<float>> m_alls;
};

// ---------------------------------------------------------------------------
// Effect chain public API (implemented in AudioEffects.cpp)
// ---------------------------------------------------------------------------

QStringList availableEffectTypes();
QStringList effectParameters(const QString& type);

int     addEffect(const QString& type);
void    removeEffect(int index);
void    clearEffects();
int     effectCount();
QString effectTypeAt(int index);
bool    isEffectBypassed(int index);
void    bypassEffect(int index, bool bypassed);

void    setEffectParam(int index, const QString& param, double value);
double  getEffectParam(int index, const QString& param);
QMap<QString, double> allEffectParams(int index);
void    resetEffect(int index);
void    moveEffect(int from, int to);

QVector<float> processEffects(const QVector<float>& input, int sampleRate);

// Master chain (EQ + compressor + delay + reverb + limiter)
void    setMasterBypass(bool bypass);
bool    masterBypassed();
void    setEqBand(int band, float gain);
float   getEqBand(int band);
int     eqBandCount();
void    setCompressor(float threshold, float ratio, float attack, float release);
void    setReverb(float roomSize, float damping, float wet, float dry);
void    setDelay(float time, float feedback, float mix);
void    setLimiter(float threshold, float release);
void    resetMasterChain();
QJsonObject saveMasterPreset(const QString& name);
void    loadMasterPreset(const QJsonObject& preset);

} // namespace audio
} // namespace ks