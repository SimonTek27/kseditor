#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <cmath>

namespace ks {

class LoudnessMeter : public QObject
{
    Q_OBJECT
public:
    enum State {
        State_Init,
        State_Measuring,
        State_Done
    };

    explicit LoudnessMeter(QObject* parent = nullptr) : QObject(parent) {}
    ~LoudnessMeter() {}

    void setSampleRate(int rate) { m_sampleRate = rate; }
    int sampleRate() const { return m_sampleRate; }

    void setBlockSize(int size) { m_blockSize = size; }
    int blockSize() const { return m_blockSize; }

    void processBlock(const QVector<float>& block) {
        if (block.isEmpty()) return;
        m_state = State_Measuring;

        float sumSq = 0.0f;
        float peak = 0.0f;
        for (float s : block) {
            sumSq += s * s;
            float absS = fabsf(s);
            if (absS > peak) peak = absS;
        }

        float rms = sqrtf(sumSq / block.size());
        float lufs = -0.691f + 10.0f * log10f(qMax(rms, 1e-10f));

        m_integratedLoudness = lufs;
        m_momentary = lufs;
        m_truePeak = 20.0f * log10f(qMax(peak, 1e-10f));
    }

    float integratedLoudness() const { return m_integratedLoudness; }
    float truePeak() const { return m_truePeak; }
    float momentaryLoudness() const { return m_momentary; }

    State state() const { return m_state; }
    bool ismeasuring() const { return m_state == State_Measuring; }

    void reset() {
        m_state = State_Init;
        m_integratedLoudness = -23.0f;
        m_truePeak = -60.0f;
        m_momentary = -23.0f;
    }

private:
    int m_sampleRate = 48000;
    int m_blockSize = 4000;
    State m_state = State_Init;
    float m_integratedLoudness = -23.0f;
    float m_truePeak = -60.0f;
    float m_momentary = -23.0f;
};

class LoudnessNormalizer : public QObject
{
    Q_OBJECT
public:
    struct NormalizeResult {
        float inputLufs;
        float gainDb;
        float outputLufs;
        float truePeak;
    };

    explicit LoudnessNormalizer(QObject* parent = nullptr) : QObject(parent) {}
    ~LoudnessNormalizer() {}

    void setTargetLoudness(float lufs) { m_targetLoudness = lufs; }
    float targetLoudness() const { return m_targetLoudness; }

    void setTruePeakLimit(float dbtp) { m_truePeakLimit = dbtp; }
    float truePeakLimit() const { return m_truePeakLimit; }

    static QVector<float> applyGain(const QVector<float>& input, float gainDb) {
        float linear = powf(10.0f, gainDb / 20.0f);
        QVector<float> output(input.size());
        for (int i = 0; i < input.size(); ++i)
            output[i] = input[i] * linear;
        return output;
    }

    QVector<float> normalize(const QVector<float>& input, float measuredLufs) {
        float gainDb = m_targetLoudness - measuredLufs;
        return applyGain(input, gainDb);
    }

    NormalizeResult normalizeWithAnalysis(const QVector<float>& input) {
        float sumSq = 0.0f;
        for (float s : input) sumSq += s * s;
        float rms = sqrtf(sumSq / qMax(input.size(), 1));
        float inputLufs = -0.691f + 10.0f * log10f(qMax(rms, 1e-10f));
        float gainDb = m_targetLoudness - inputLufs;
        return { inputLufs, gainDb, m_targetLoudness, 0.0f };
    }

private:
    float m_targetLoudness = -16.0f;
    float m_truePeakLimit = -1.0f;
};

inline float lufsToDb(float lufs) {
    return lufs + 23.0f;
}

inline float dbToLufs(float db) {
    return db - 23.0f;
}

} // namespace ks
