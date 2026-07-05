#pragma once

#include <QObject>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace ks {
namespace audio {

class SpectralGate : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float threshold READ threshold WRITE setThreshold)
    Q_PROPERTY(float floor READ floor WRITE setFloor)
    Q_PROPERTY(float attack READ attack WRITE setAttack)
    Q_PROPERTY(float release READ release WRITE setRelease)
    Q_PROPERTY(float lowCut READ lowCut WRITE setLowCut)
    Q_PROPERTY(float highCut READ highCut WRITE setHighCut)
public:
    explicit SpectralGate(QObject* parent = nullptr) : QObject(parent) {}
    ~SpectralGate() {}

    void setThreshold(float db) { m_threshold = db; }
    float threshold() const { return m_threshold; }
    void setFloor(float db) { m_floor = db; }
    float floor() const { return m_floor; }
    void setAttack(float ms) { m_attack = qBound(0.1f, ms, 500.0f); }
    float attack() const { return m_attack; }
    void setRelease(float ms) { m_release = qBound(10.0f, ms, 2000.0f); }
    float release() const { return m_release; }
    void setLowCut(float hz) { m_lowCut = qBound(20.0f, hz, 20000.0f); }
    float lowCut() const { return m_lowCut; }
    void setHighCut(float hz) { m_highCut = qBound(20.0f, hz, 20000.0f); }
    float highCut() const { return m_highCut; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float thresholdLin = std::pow(10.0f, m_threshold / 20.0f);
        float floorLin = std::pow(10.0f, m_floor / 20.0f);
        float attackCoeff = std::exp(-1.0f / (m_attack * 0.001f * sampleRate));
        float releaseCoeff = std::exp(-1.0f / (m_release * 0.001f * sampleRate));
        float lowNorm = m_lowCut / (sampleRate * 0.5f);
        float highNorm = m_highCut / (sampleRate * 0.5f);

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];
            float absS = std::abs(s);

            float envelope = (absS > m_env)
                ? attackCoeff * m_env + (1.0f - attackCoeff) * absS
                : releaseCoeff * m_env + (1.0f - releaseCoeff) * absS;
            m_env = envelope;

            float gain = 1.0f;
            if (envelope < thresholdLin && envelope > 0.0001f) {
                float dbDiff = 20.0f * std::log10(envelope / thresholdLin);
                float reduction = std::max(0.0f, 1.0f + dbDiff / (-m_floor));
                gain = std::max(floorLin / thresholdLin, reduction);
            }

            output[i] = s * gain;
        }

        return output;
    }

    void reset() { m_env = 0.0f; }

private:
    float m_threshold = -40.0f;
    float m_floor = -80.0f;
    float m_attack = 5.0f;
    float m_release = 100.0f;
    float m_lowCut = 20.0f;
    float m_highCut = 20000.0f;
    float m_env = 0.0f;
};

class FormantFilterState {
public:
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    void designBP(float freq, float bw, int sampleRate) {
        float w0 = 2.0f * M_PI * freq / sampleRate;
        float alpha = std::sin(w0) * std::sinh(std::log(2.0f) * 0.5f * bw * w0 / std::sin(w0));
        float norm = 1.0f + alpha;
        b0 = alpha / norm;
        b1 = 0;
        b2 = -alpha / norm;
        a1 = -2.0f * std::cos(w0) / norm;
        a2 = (1.0f - alpha) / norm;
    }

    float process(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }
};

class FormantFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float shift READ shift WRITE setShift)
    Q_PROPERTY(float formant1Freq READ formant1Freq WRITE setFormant1Freq)
    Q_PROPERTY(float formant1Gain READ formant1Gain WRITE setFormant1Gain)
    Q_PROPERTY(float formant1BW READ formant1BW WRITE setFormant1BW)
    Q_PROPERTY(float formant2Freq READ formant2Freq WRITE setFormant2Freq)
    Q_PROPERTY(float formant2Gain READ formant2Gain WRITE setFormant2Gain)
    Q_PROPERTY(float formant2BW READ formant2BW WRITE setFormant2BW)
    Q_PROPERTY(float formant3Freq READ formant3Freq WRITE setFormant3Freq)
    Q_PROPERTY(float formant3Gain READ formant3Gain WRITE setFormant3Gain)
    Q_PROPERTY(float formant3BW READ formant3BW WRITE setFormant3BW)
    Q_PROPERTY(float formant4Freq READ formant4Freq WRITE setFormant4Freq)
    Q_PROPERTY(float formant4Gain READ formant4Gain WRITE setFormant4Gain)
    Q_PROPERTY(float formant4BW READ formant4BW WRITE setFormant4BW)
    Q_PROPERTY(float formant5Freq READ formant5Freq WRITE setFormant5Freq)
    Q_PROPERTY(float formant5Gain READ formant5Gain WRITE setFormant5Gain)
    Q_PROPERTY(float formant5BW READ formant5BW WRITE setFormant5BW)
public:
    explicit FormantFilter(QObject* parent = nullptr) : QObject(parent) {
        m_sr = 44100;
        m_formants[0] = {800, 0, 100};
        m_formants[1] = {2200, 0, 120};
        m_formants[2] = {3500, 0, 150};
        m_formants[3] = {4500, 0, 200};
        m_formants[4] = {5500, 0, 300};
        for (int f = 0; f < 5; ++f) updateFilter(f);
    }

    void setShift(float st) { m_shift = qBound(-12.0f, st, 12.0f); rebuildAll(); }
    float shift() const { return m_shift; }

    void setFormant1Freq(float hz) { m_formants[0].freq = qBound(100.0f, hz, 4000.0f); updateFilter(0); }
    float formant1Freq() const { return m_formants[0].freq; }
    void setFormant1Gain(float db) { m_formants[0].gain = qBound(-24.0f, db, 24.0f); }
    float formant1Gain() const { return m_formants[0].gain; }
    void setFormant1BW(float hz) { m_formants[0].bw = qBound(20.0f, hz, 1000.0f); updateFilter(0); }
    float formant1BW() const { return m_formants[0].bw; }

    void setFormant2Freq(float hz) { m_formants[1].freq = qBound(500.0f, hz, 6000.0f); updateFilter(1); }
    float formant2Freq() const { return m_formants[1].freq; }
    void setFormant2Gain(float db) { m_formants[1].gain = qBound(-24.0f, db, 24.0f); }
    float formant2Gain() const { return m_formants[1].gain; }
    void setFormant2BW(float hz) { m_formants[1].bw = qBound(20.0f, hz, 1000.0f); updateFilter(1); }
    float formant2BW() const { return m_formants[1].bw; }

    void setFormant3Freq(float hz) { m_formants[2].freq = qBound(1000.0f, hz, 8000.0f); updateFilter(2); }
    float formant3Freq() const { return m_formants[2].freq; }
    void setFormant3Gain(float db) { m_formants[2].gain = qBound(-24.0f, db, 24.0f); }
    float formant3Gain() const { return m_formants[2].gain; }
    void setFormant3BW(float hz) { m_formants[2].bw = qBound(20.0f, hz, 1000.0f); updateFilter(2); }
    float formant3BW() const { return m_formants[2].bw; }

    void setFormant4Freq(float hz) { m_formants[3].freq = qBound(2000.0f, hz, 12000.0f); updateFilter(3); }
    float formant4Freq() const { return m_formants[3].freq; }
    void setFormant4Gain(float db) { m_formants[3].gain = qBound(-24.0f, db, 24.0f); }
    float formant4Gain() const { return m_formants[3].gain; }
    void setFormant4BW(float hz) { m_formants[3].bw = qBound(20.0f, hz, 1000.0f); updateFilter(3); }
    float formant4BW() const { return m_formants[3].bw; }

    void setFormant5Freq(float hz) { m_formants[4].freq = qBound(3000.0f, hz, 16000.0f); updateFilter(4); }
    float formant5Freq() const { return m_formants[4].freq; }
    void setFormant5Gain(float db) { m_formants[4].gain = qBound(-24.0f, db, 24.0f); }
    float formant5Gain() const { return m_formants[4].gain; }
    void setFormant5BW(float hz) { m_formants[4].bw = qBound(20.0f, hz, 1000.0f); updateFilter(4); }
    float formant5BW() const { return m_formants[4].bw; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        if (input.isEmpty()) return input;
        if (m_sr != sampleRate) { m_sr = sampleRate; rebuildAll(); }
        QVector<float> output(input.size());

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];
            float out = 0.0f;
            for (int f = 0; f < 5; ++f) {
                float filtered = m_filters[f].process(s);
                float gainLin = std::pow(10.0f, m_formants[f].gain / 20.0f);
                out += filtered * gainLin;
            }
            output[i] = out * 0.2f;
        }
        return output;
    }

    void reset() {
        for (int f = 0; f < 5; ++f)
            m_filters[f] = FormantFilterState();
        rebuildAll();
    }

    void setSampleRate(int sr) { m_sr = sr; }

private:
    struct Formant {
        float freq, gain, bw;
    };

    void updateFilter(int idx) {
        m_filters[idx].designBP(m_formants[idx].freq, m_formants[idx].bw, m_sr);
    }

    void rebuildAll() {
        for (int f = 0; f < 5; ++f) {
            float shiftedFreq = m_formants[f].freq * std::pow(2.0f, m_shift / 12.0f);
            m_filters[f].designBP(shiftedFreq, m_formants[f].bw, m_sr);
        }
    }

    float m_shift = 0.0f;
    int m_sr = 44100;
    Formant m_formants[5];
    FormantFilterState m_filters[5];
};

class RingMod : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float frequency READ frequency WRITE setFrequency)
    Q_PROPERTY(float mix READ mix WRITE setMix)
public:
    explicit RingMod(QObject* parent = nullptr) : QObject(parent) {}
    ~RingMod() {}

    void setFrequency(float hz) { m_freq = qBound(1.0f, hz, 5000.0f); }
    float frequency() const { return m_freq; }
    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float inc = 2.0f * M_PI * m_freq / sampleRate;

        for (int i = 0; i < input.size(); ++i) {
            float carrier = std::sin(m_phase);
            m_phase += inc;
            if (m_phase > 2.0f * M_PI) m_phase -= 2.0f * M_PI;

            float wet = input[i] * carrier;
            output[i] = input[i] * (1.0f - m_mix) + wet * m_mix;
        }
        return output;
    }

    void reset() { m_phase = 0.0f; }

private:
    float m_freq = 440.0f;
    float m_mix = 0.5f;
    float m_phase = 0.0f;
};

class AutoWah : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int filterType READ filterType WRITE setFilterType)
    Q_PROPERTY(float freqMin READ freqMin WRITE setFreqMin)
    Q_PROPERTY(float freqMax READ freqMax WRITE setFreqMax)
    Q_PROPERTY(float resonance READ resonance WRITE setResonance)
    Q_PROPERTY(float sensitivity READ sensitivity WRITE setSensitivity)
    Q_PROPERTY(float attack READ attack WRITE setAttack)
    Q_PROPERTY(float release READ release WRITE setRelease)
    Q_PROPERTY(float depth READ depth WRITE setDepth)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(bool envelopeFollower READ envelopeFollower WRITE setEnvelopeFollower)
    Q_PROPERTY(float lfoRate READ lfoRate WRITE setLfoRate)
public:
    explicit AutoWah(QObject* parent = nullptr) : QObject(parent) {}
    ~AutoWah() {}

    void setFilterType(int t) { m_filterType = qBound(0, t, 3); }
    int filterType() const { return m_filterType; }
    void setFreqMin(float hz) { m_freqMin = qBound(50.0f, hz, 5000.0f); }
    float freqMin() const { return m_freqMin; }
    void setFreqMax(float hz) { m_freqMax = qBound(100.0f, hz, 15000.0f); }
    float freqMax() const { return m_freqMax; }
    void setResonance(float r) { m_resonance = qBound(0.0f, r, 1.0f); }
    float resonance() const { return m_resonance; }
    void setSensitivity(float s) { m_sensitivity = qBound(0.0f, s, 1.0f); }
    float sensitivity() const { return m_sensitivity; }
    void setAttack(float ms) { m_attack = qBound(0.1f, ms, 200.0f); }
    float attack() const { return m_attack; }
    void setRelease(float ms) { m_release = qBound(5.0f, ms, 1000.0f); }
    float release() const { return m_release; }
    void setDepth(float d) { m_depth = qBound(0.0f, d, 1.0f); }
    float depth() const { return m_depth; }
    void setMix(float m) { m_mix = qBound(0.0f, m, 1.0f); }
    float mix() const { return m_mix; }
    void setEnvelopeFollower(bool e) { m_envFollow = e; }
    bool envelopeFollower() const { return m_envFollow; }
    void setLfoRate(float hz) { m_lfoRate = qBound(0.05f, hz, 20.0f); }
    float lfoRate() const { return m_lfoRate; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        double lfoInc = 2.0 * M_PI * m_lfoRate / sampleRate;
        double attCoeff = std::exp(-1.0 / (m_attack * 0.001 * sampleRate));
        double relCoeff = std::exp(-1.0 / (m_release * 0.001 * sampleRate));

        for (int i = 0; i < input.size(); ++i) {
            double s = input[i];

            double absS = std::abs(s);
            m_env = (absS > m_env)
                ? attCoeff * m_env + (1.0 - attCoeff) * absS
                : relCoeff * m_env + (1.0 - relCoeff) * absS;

            double envPos = std::min(1.0, m_env * (1.0 + m_sensitivity * 9.0));

            double lfo = 0.5 + 0.5 * std::sin(m_lfoPhase);
            m_lfoPhase += lfoInc;
            if (m_lfoPhase > 2.0 * M_PI) m_lfoPhase -= 2.0 * M_PI;

            double control = m_envFollow ? envPos : lfo;
            control *= m_depth;

            double freq = m_freqMin + (m_freqMax - m_freqMin) * control;
            double Q = 1.0 + m_resonance * 15.0;
            double w0 = 2.0 * M_PI * freq / sampleRate;
            double alpha = std::sin(w0) / (2.0 * Q);

            double b0, b1, b2, a1, a2;
            if (m_filterType == 0) {
                b0 = alpha; b1 = 0; b2 = -alpha;
                a1 = -2.0 * std::cos(w0);
                a2 = 1.0 - alpha;
            } else if (m_filterType == 1) {
                double norm = 1.0 + alpha;
                b0 = (1.0 - std::cos(w0)) / 2.0 / norm;
                b1 = (1.0 - std::cos(w0)) / norm;
                b2 = b0;
                a1 = -2.0 * std::cos(w0) / norm;
                a2 = (1.0 - alpha) / norm;
            } else if (m_filterType == 2) {
                double norm = 1.0 + alpha;
                b0 = (1.0 + std::cos(w0)) / 2.0 / norm;
                b1 = -(1.0 + std::cos(w0)) / norm;
                b2 = b0;
                a1 = -2.0 * std::cos(w0) / norm;
                a2 = (1.0 - alpha) / norm;
            } else {
                double norm = 1.0 + alpha;
                b0 = 1.0 + alpha * std::pow(10.0, 6.0 / 20.0);
                b1 = -2.0 * std::cos(w0);
                b2 = 1.0 - alpha;
                a1 = b1 / norm;
                a2 = (1.0 - alpha) / norm;
            }

            double norm = 1.0 + alpha;
            b0 /= norm; b1 /= norm; b2 /= norm; a1 /= norm; a2 /= norm;

            double y = b0 * s + b1 * m_x1 + b2 * m_x2 - a1 * m_y1 - a2 * m_y2;
            m_x2 = m_x1; m_x1 = s;
            m_y2 = m_y1; m_y1 = y;

            output[i] = static_cast<float>(s * (1.0 - m_mix) + y * m_mix);
        }
        return output;
    }

    void reset() {
        m_env = 0.0;
        m_lfoPhase = 0.0;
        m_x1 = m_x2 = m_y1 = m_y2 = 0.0;
    }

private:
    int m_filterType = 0;
    float m_freqMin = 400.0f;
    float m_freqMax = 4000.0f;
    float m_resonance = 0.5f;
    float m_sensitivity = 0.5f;
    float m_attack = 20.0f;
    float m_release = 50.0f;
    float m_depth = 0.8f;
    float m_mix = 0.7f;
    bool m_envFollow = true;
    float m_lfoRate = 0.5f;
    double m_env = 0.0;
    double m_lfoPhase = 0.0;
    double m_x1 = 0, m_x2 = 0, m_y1 = 0, m_y2 = 0;
};

class BitCrusher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bitDepth READ bitDepth WRITE setBitDepth)
    Q_PROPERTY(int sampleRateReduction READ sampleRateReduction WRITE setSampleRateReduction)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(float noiseShaping READ noiseShaping WRITE setNoiseShaping)
public:
    explicit BitCrusher(QObject* parent = nullptr) : QObject(parent) {}
    ~BitCrusher() {}

    void setBitDepth(int bits) { m_bitDepth = qBound(1, bits, 24); }
    int bitDepth() const { return m_bitDepth; }
    void setSampleRateReduction(int factor) { m_srReduction = qBound(1, factor, 100); }
    int sampleRateReduction() const { return m_srReduction; }
    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }
    void setNoiseShaping(float ns) { m_noiseShaping = qBound(0.0f, ns, 1.0f); }
    float noiseShaping() const { return m_noiseShaping; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        Q_UNUSED(sampleRate)
        QVector<float> output(input.size());
        float quantSteps = std::pow(2.0f, m_bitDepth - 1);
        float invSteps = 1.0f / quantSteps;

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];

            m_holdCounter++;
            if (m_holdCounter >= m_srReduction) {
                m_holdCounter = 0;
                m_heldSample = s;
            }

            float sample = m_heldSample + m_error * m_noiseShaping;
            sample = std::round(sample * quantSteps) * invSteps;
            m_error = s - sample;

            output[i] = s * (1.0f - m_mix) + sample * m_mix;
        }
        return output;
    }

    void reset() {
        m_heldSample = 0.0f;
        m_holdCounter = 0;
        m_error = 0.0f;
    }

private:
    int m_bitDepth = 8;
    int m_srReduction = 4;
    float m_mix = 0.5f;
    float m_noiseShaping = 0.5f;
    float m_heldSample = 0.0f;
    int m_holdCounter = 0;
    float m_error = 0.0f;
};

class Ducker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float threshold READ threshold WRITE setThreshold)
    Q_PROPERTY(float depth READ depth WRITE setDepth)
    Q_PROPERTY(float attack READ attack WRITE setAttack)
    Q_PROPERTY(float release READ release WRITE setRelease)
    Q_PROPERTY(float sidechainGain READ sidechainGain WRITE setSidechainGain)
    Q_PROPERTY(float mix READ mix WRITE setMix)
public:
    explicit Ducker(QObject* parent = nullptr) : QObject(parent) {}
    ~Ducker() {}

    void setThreshold(float db) { m_threshold = db; }
    float threshold() const { return m_threshold; }
    void setDepth(float db) { m_depth = qBound(0.0f, db, 60.0f); }
    float depth() const { return m_depth; }
    void setAttack(float ms) { m_attack = qBound(0.1f, ms, 500.0f); }
    float attack() const { return m_attack; }
    void setRelease(float ms) { m_release = qBound(10.0f, ms, 2000.0f); }
    float release() const { return m_release; }
    void setSidechainGain(float g) { m_sidechainGain = qBound(-12.0f, g, 12.0f); }
    float sidechainGain() const { return m_sidechainGain; }
    void setMix(float m) { m_mix = qBound(0.0f, m, 1.0f); }
    float mix() const { return m_mix; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float scGain = std::pow(10.0f, m_sidechainGain / 20.0f);
        float thresholdLin = std::pow(10.0f, m_threshold / 20.0f);
        float depthLin = std::pow(10.0f, -m_depth / 20.0f);
        float attackCoeff = std::exp(-1.0f / (m_attack * 0.001f * sampleRate));
        float releaseCoeff = std::exp(-1.0f / (m_release * 0.001f * sampleRate));

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];
            float sc = std::abs(s) * scGain;

            m_env = (sc > m_env)
                ? attackCoeff * m_env + (1.0f - attackCoeff) * sc
                : releaseCoeff * m_env + (1.0f - releaseCoeff) * sc;

            float gain = 1.0f;
            if (m_env > thresholdLin) {
                float ratio = thresholdLin / m_env;
                float reduction = 1.0f - (1.0f - ratio) * (1.0f - depthLin);
                gain = std::max(depthLin, reduction);
            }

            output[i] = s * (gain * (1.0f - m_mix) + 1.0f * m_mix);
        }
        return output;
    }

    void reset() { m_env = 0.0f; }

private:
    float m_threshold = -20.0f;
    float m_depth = 12.0f;
    float m_attack = 1.0f;
    float m_release = 200.0f;
    float m_sidechainGain = 0.0f;
    float m_mix = 0.5f;
    float m_env = 0.0f;
};

class DeEsser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float threshold READ threshold WRITE setThreshold)
    Q_PROPERTY(float frequency READ frequency WRITE setFrequency)
    Q_PROPERTY(float bandwidth READ bandwidth WRITE setBandwidth)
    Q_PROPERTY(float reduction READ reduction WRITE setReduction)
    Q_PROPERTY(float attack READ attack WRITE setAttack)
    Q_PROPERTY(float release READ release WRITE setRelease)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(bool listenMode READ listenMode WRITE setListenMode)
public:
    explicit DeEsser(QObject* parent = nullptr) : QObject(parent) {}
    ~DeEsser() {}

    void setThreshold(float db) { m_threshold = db; }
    float threshold() const { return m_threshold; }
    void setFrequency(float hz) { m_freq = qBound(1000.0f, hz, 12000.0f); }
    float frequency() const { return m_freq; }
    void setBandwidth(float bw) { m_bw = qBound(0.5f, bw, 4.0f); }
    float bandwidth() const { return m_bw; }
    void setReduction(float db) { m_reduction = qBound(0.0f, db, 40.0f); }
    float reduction() const { return m_reduction; }
    void setAttack(float ms) { m_attack = qBound(0.1f, ms, 50.0f); }
    float attack() const { return m_attack; }
    void setRelease(float ms) { m_release = qBound(10.0f, ms, 500.0f); }
    float release() const { return m_release; }
    void setMix(float m) { m_mix = qBound(0.0f, m, 1.0f); }
    float mix() const { return m_mix; }
    void setListenMode(bool l) { m_listenMode = l; }
    bool listenMode() const { return m_listenMode; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        double w0 = 2.0 * M_PI * m_freq / sampleRate;
        double Q = m_bw;
        double alpha = std::sin(w0) / (2.0 * Q);
        double norm = 1.0 + alpha;
        double b0 = alpha / norm;
        double b1 = 0;
        double b2 = -alpha / norm;
        double a1 = -2.0 * std::cos(w0) / norm;
        double a2 = (1.0 - alpha) / norm;

        double thresholdLin = std::pow(10.0, m_threshold / 20.0);
        double reductionLin = std::pow(10.0, -m_reduction / 20.0);
        double attCoeff = std::exp(-1.0 / (m_attack * 0.001 * sampleRate));
        double relCoeff = std::exp(-1.0 / (m_release * 0.001 * sampleRate));

        for (int i = 0; i < input.size(); ++i) {
            double s = input[i];
            double bp = b0 * s + b1 * m_bpX1 + b2 * m_bpX2 - a1 * m_bpY1 - a2 * m_bpY2;
            m_bpX2 = m_bpX1; m_bpX1 = s;
            m_bpY2 = m_bpY1; m_bpY1 = bp;

            double absBP = std::abs(bp);
            m_env = (absBP > m_env)
                ? attCoeff * m_env + (1.0 - attCoeff) * absBP
                : relCoeff * m_env + (1.0 - relCoeff) * absBP;

            double gain = 1.0;
            if (m_env > thresholdLin) {
                double excess = m_env - thresholdLin;
                double maxReduction = 1.0 - reductionLin;
                double reduction = std::min(1.0, excess / (thresholdLin * 2.0)) * maxReduction;
                gain = 1.0 - reduction;
            }

            double processed = s * gain;
            if (m_listenMode) {
                output[i] = static_cast<float>(bp * gain * 6.0);
            } else {
                output[i] = static_cast<float>(s * (1.0 - m_mix) + processed * m_mix);
            }
        }
        return output;
    }

    void reset() {
        m_env = 0.0;
        m_bpX1 = m_bpX2 = m_bpY1 = m_bpY2 = 0.0;
    }

private:
    float m_threshold = -30.0f;
    float m_freq = 7000.0f;
    float m_bw = 2.0f;
    float m_reduction = 15.0f;
    float m_attack = 1.0f;
    float m_release = 50.0f;
    float m_mix = 0.5f;
    bool m_listenMode = false;
    double m_env = 0.0;
    double m_bpX1 = 0, m_bpX2 = 0, m_bpY1 = 0, m_bpY2 = 0;
};

class SaturationDistortion : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float drive READ drive WRITE setDrive)
    Q_PROPERTY(float bias READ bias WRITE setBias)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(int type READ type WRITE setType)
    Q_PROPERTY(float outputGain READ outputGain WRITE setOutputGain)
public:
    explicit SaturationDistortion(QObject* parent = nullptr) : QObject(parent) {}
    ~SaturationDistortion() {}

    void setDrive(float d) { m_drive = qBound(0.0f, d, 1.0f); }
    float drive() const { return m_drive; }
    void setBias(float b) { m_bias = qBound(-0.5f, b, 0.5f); }
    float bias() const { return m_bias; }
    void setMix(float m) { m_mix = qBound(0.0f, m, 1.0f); }
    float mix() const { return m_mix; }
    void setType(int t) { m_type = qBound(0, t, 4); }
    int type() const { return m_type; }
    void setOutputGain(float g) { m_outputGain = qBound(-12.0f, g, 12.0f); }
    float outputGain() const { return m_outputGain; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        Q_UNUSED(sampleRate)
        QVector<float> output(input.size());
        float gain = std::pow(10.0f, m_outputGain / 20.0f);

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i] + m_bias;
            float driveAmt = 1.0f + m_drive * 30.0f;
            float wet = s * driveAmt;

            switch (m_type) {
            case 0: // Soft clip
                wet = std::tanh(wet);
                break;
            case 1: // Hard clip
                wet = std::max(-1.0f, std::min(1.0f, wet));
                break;
            case 2: // Tube
                wet = wet / (1.0f + std::abs(wet));
                break;
            case 3: // Exponential
                wet = (wet >= 0) ? 1.0f - std::exp(-wet) : -(1.0f - std::exp(wet));
                break;
            case 4: // Square
                wet = (wet > 0) ? 1.0f : -1.0f;
                break;
            }

            wet *= gain;
            output[i] = s * (1.0f - m_mix) + wet * m_mix;
        }
        return output;
    }

    void reset() {}

private:
    float m_drive = 0.3f;
    float m_bias = 0.0f;
    float m_mix = 0.5f;
    int m_type = 0;
    float m_outputGain = 0.0f;
};

class TremoloModulation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float rate READ rate WRITE setRate)
    Q_PROPERTY(float depth READ depth WRITE setDepth)
    Q_PROPERTY(float shape READ shape WRITE setShape)
    Q_PROPERTY(int waveform READ waveform WRITE setWaveform)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(bool stereoPhase READ stereoPhase WRITE setStereoPhase)
public:
    explicit TremoloModulation(QObject* parent = nullptr) : QObject(parent) {}
    ~TremoloModulation() {}

    void setRate(float hz) { m_rate = qBound(0.05f, hz, 20.0f); }
    float rate() const { return m_rate; }
    void setDepth(float d) { m_depth = qBound(0.0f, d, 1.0f); }
    float depth() const { return m_depth; }
    void setShape(float s) { m_shape = qBound(0.0f, s, 1.0f); }
    float shape() const { return m_shape; }
    void setWaveform(int w) { m_waveform = qBound(0, w, 2); }
    int waveform() const { return m_waveform; }
    void setMix(float m) { m_mix = qBound(0.0f, m, 1.0f); }
    float mix() const { return m_mix; }
    void setStereoPhase(bool sp) { m_stereoPhase = sp; }
    bool stereoPhase() const { return m_stereoPhase; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        QVector<float> output(input.size());
        float inc = 2.0f * M_PI * m_rate / sampleRate;
        bool stereo = m_stereoPhase && input.size() >= 2;

        for (int i = 0; i < input.size(); ++i) {
            float lfo;
            float phase = m_phase;
            if (stereo && (i % 2 == 1)) phase += M_PI_2;

            switch (m_waveform) {
            case 0: lfo = 0.5f + 0.5f * std::sin(phase); break;
            case 1: lfo = std::abs(std::sin(phase)); break;
            case 2: lfo = phase / (2.0f * M_PI); break;
            default: lfo = 0.5f + 0.5f * std::sin(phase); break;
            }

            lfo = std::pow(lfo, 1.0f + m_shape * 4.0f);
            float mod = 1.0f - lfo * m_depth;
            float wet = input[i] * mod;

            output[i] = input[i] * (1.0f - m_mix) + wet * m_mix;

            if (i % 2 == (stereo ? 0 : 0)) {
                m_phase += inc;
                if (m_phase > 2.0f * M_PI) m_phase -= 2.0f * M_PI;
            }
        }
        return output;
    }

    void reset() { m_phase = 0.0f; }

private:
    float m_rate = 4.0f;
    float m_depth = 0.5f;
    float m_shape = 0.0f;
    int m_waveform = 0;
    float m_mix = 0.5f;
    bool m_stereoPhase = false;
    float m_phase = 0.0f;
};

class MultiBandSplitter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bandCount READ bandCount WRITE setBandCount)
    Q_PROPERTY(float crossover0 READ crossover0 WRITE setCrossover0)
    Q_PROPERTY(float crossover1 READ crossover1 WRITE setCrossover1)
    Q_PROPERTY(float crossover2 READ crossover2 WRITE setCrossover2)
    Q_PROPERTY(float band0Gain READ band0Gain WRITE setBand0Gain)
    Q_PROPERTY(float band1Gain READ band1Gain WRITE setBand1Gain)
    Q_PROPERTY(float band2Gain READ band2Gain WRITE setBand2Gain)
    Q_PROPERTY(float band3Gain READ band3Gain WRITE setBand3Gain)
    Q_PROPERTY(bool band0Mute READ band0Mute WRITE setBand0Mute)
    Q_PROPERTY(bool band1Mute READ band1Mute WRITE setBand1Mute)
    Q_PROPERTY(bool band2Mute READ band2Mute WRITE setBand2Mute)
    Q_PROPERTY(bool band3Mute READ band3Mute WRITE setBand3Mute)
    Q_PROPERTY(bool band0Solo READ band0Solo WRITE setBand0Solo)
    Q_PROPERTY(bool band1Solo READ band1Solo WRITE setBand1Solo)
    Q_PROPERTY(bool band2Solo READ band2Solo WRITE setBand2Solo)
    Q_PROPERTY(bool band3Solo READ band3Solo WRITE setBand3Solo)
public:
    explicit MultiBandSplitter(QObject* parent = nullptr) : QObject(parent) {
        m_crossovers[0] = 250.0f;
        m_crossovers[1] = 2000.0f;
        m_crossovers[2] = 8000.0f;
        for (int i = 0; i < 4; ++i) m_bandGains[i] = 0.0f;
    }
    ~MultiBandSplitter() {}

    void setBandCount(int c) { m_bandCount = qBound(2, c, 4); }
    int bandCount() const { return m_bandCount; }

    void setCrossover0(float hz) { m_crossovers[0] = qBound(20.0f, hz, 20000.0f); }
    float crossover0() const { return m_crossovers[0]; }
    void setCrossover1(float hz) { m_crossovers[1] = qBound(20.0f, hz, 20000.0f); }
    float crossover1() const { return m_crossovers[1]; }
    void setCrossover2(float hz) { m_crossovers[2] = qBound(20.0f, hz, 20000.0f); }
    float crossover2() const { return m_crossovers[2]; }

    void setBand0Gain(float db) { m_bandGains[0] = qBound(-24.0f, db, 24.0f); }
    float band0Gain() const { return m_bandGains[0]; }
    void setBand1Gain(float db) { m_bandGains[1] = qBound(-24.0f, db, 24.0f); }
    float band1Gain() const { return m_bandGains[1]; }
    void setBand2Gain(float db) { m_bandGains[2] = qBound(-24.0f, db, 24.0f); }
    float band2Gain() const { return m_bandGains[2]; }
    void setBand3Gain(float db) { m_bandGains[3] = qBound(-24.0f, db, 24.0f); }
    float band3Gain() const { return m_bandGains[3]; }

    void setBand0Mute(bool m) { m_bandMutes[0] = m; }
    bool band0Mute() const { return m_bandMutes[0]; }
    void setBand1Mute(bool m) { m_bandMutes[1] = m; }
    bool band1Mute() const { return m_bandMutes[1]; }
    void setBand2Mute(bool m) { m_bandMutes[2] = m; }
    bool band2Mute() const { return m_bandMutes[2]; }
    void setBand3Mute(bool m) { m_bandMutes[3] = m; }
    bool band3Mute() const { return m_bandMutes[3]; }

    void setBand0Solo(bool s) { m_bandSolos[0] = s; }
    bool band0Solo() const { return m_bandSolos[0]; }
    void setBand1Solo(bool s) { m_bandSolos[1] = s; }
    bool band1Solo() const { return m_bandSolos[1]; }
    void setBand2Solo(bool s) { m_bandSolos[2] = s; }
    bool band2Solo() const { return m_bandSolos[2]; }
    void setBand3Solo(bool s) { m_bandSolos[3] = s; }
    bool band3Solo() const { return m_bandSolos[3]; }

    QVector<float> process(const QVector<float>& input, int sampleRate) {
        int n = input.size();
        QVector<float> output(n, 0.0f);
        bool anySolo = false;
        for (int i = 0; i < 4; ++i) if (m_bandSolos[i]) anySolo = true;

        QVector<QVector<float>> bands(m_bandCount, QVector<float>(n, 0.0f));

        for (int b = 0; b < m_bandCount; ++b) {
            float lo = (b == 0) ? 20.0f : m_crossovers[b - 1];
            float hi = (b == m_bandCount - 1) ? 20000.0f : m_crossovers[b];

            if (b == 0) {
                for (int i = 0; i < n; ++i)
                    bands[b][i] = lpf(input[i], sampleRate, hi, m_lpStates[0]);
            } else if (b == m_bandCount - 1) {
                for (int i = 0; i < n; ++i)
                    bands[b][i] = hpf(input[i], sampleRate, lo, m_hpStates[m_bandCount - 1]);
            } else {
                for (int i = 0; i < n; ++i) {
                    float lp = lpf(input[i], sampleRate, hi, m_lpStates[b]);
                    float hp = hpf(input[i], sampleRate, lo, m_hpStates[b]);
                    bands[b][i] = lp + hp;
                }
            }

            float gainLin = std::pow(10.0f, m_bandGains[b] / 20.0f);
            bool pass = !m_bandMutes[b] && (!anySolo || m_bandSolos[b]);

            for (int i = 0; i < n; ++i) {
                if (pass) output[i] += bands[b][i] * gainLin;
            }
        }

        return output;
    }

    void reset() {
        for (int i = 0; i < 4; ++i) {
            m_lpStates[i] = 0.0f;
            m_hpStates[i] = 0.0f;
        }
    }

private:
    float lpf(float x, int sr, float freq, float& state) {
        float rc = 1.0f / (2.0f * M_PI * freq);
        float dt = 1.0f / sr;
        float alpha = dt / (rc + dt);
        state = state + alpha * (x - state);
        return state;
    }

    float hpf(float x, int sr, float freq, float& state) {
        float rc = 1.0f / (2.0f * M_PI * freq);
        float dt = 1.0f / sr;
        float alpha = rc / (rc + dt);
        state = alpha * state + alpha * (x - state);
        return x - state;
    }

    int m_bandCount = 3;
    float m_crossovers[3] = {250.0f, 2000.0f, 8000.0f};
    float m_bandGains[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    bool m_bandMutes[4] = {false, false, false, false};
    bool m_bandSolos[4] = {false, false, false, false};
    float m_lpStates[4] = {0.0f};
    float m_hpStates[4] = {0.0f};
};

} // namespace audio
} // namespace ks
