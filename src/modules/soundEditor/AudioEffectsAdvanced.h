#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QFile>
#include <QAudioFormat>
#include <cmath>
#include <complex>
#include <algorithm>

namespace ks {
namespace audio {

class ConvolutionReverb : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(float preDelay READ preDelay WRITE setPreDelay)
    Q_PROPERTY(float gain READ gain WRITE setGain)
public:
    explicit ConvolutionReverb(QObject* parent = nullptr) : QObject(parent) {}
    ~ConvolutionReverb() {}

    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }
    void setPreDelay(float ms) { m_preDelay = qBound(0.0f, ms, 500.0f); }
    float preDelay() const { return m_preDelay; }
    void setGain(float gain) { m_gain = qBound(0.0f, gain, 10.0f); }
    float gain() const { return m_gain; }

    bool loadImpulseResponse(const QString& wavPath, int targetSampleRate = 48000)
    {
        if (wavPath.isEmpty()) return false;
        QFile file(wavPath);
        if (!file.open(QIODevice::ReadOnly)) return false;

        m_ir.clear();
        QAudioFormat fmt;
        m_preDelaySamples = static_cast<int>(m_preDelay * targetSampleRate / 1000.0f);
        m_sampleRate = targetSampleRate;

        QByteArray data = file.readAll();
        file.close();

        if (data.size() < 44) return false;
        if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F') return false;
        if (data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E') return false;

        int channels = *reinterpret_cast<const quint16*>(data.data() + 22);
        int bitsPerSample = *reinterpret_cast<const quint16*>(data.data() + 34);
        int dataSize = *reinterpret_cast<const quint32*>(data.data() + 40);
        int sampleCount = dataSize / (channels * (bitsPerSample / 8));
        if (sampleCount == 0) return false;

        const char* pcm = data.data() + 44;
        int bytesPerSample = bitsPerSample / 8;
        int stride = channels * bytesPerSample;

        m_ir.reserve(sampleCount / channels);
        float scale = (bitsPerSample == 16) ? 32768.0f : 8388608.0f;
        for (int i = 0; i + stride <= dataSize; i += stride) {
            qint32 val = 0;
            if (bytesPerSample == 2)
                val = *reinterpret_cast<const qint16*>(pcm + i);
            else if (bytesPerSample == 3)
                val = (static_cast<qint32>(static_cast<unsigned char>(pcm[i + 2])) << 16)
                    | (static_cast<qint32>(static_cast<unsigned char>(pcm[i + 1])) << 8)
                    | static_cast<qint32>(static_cast<unsigned char>(pcm[i]));
            else if (bytesPerSample == 1)
                val = (static_cast<qint32>(static_cast<unsigned char>(pcm[i])) - 128) << 8;
            m_ir.append(static_cast<float>(val) / scale);
        }

        if (m_ir.isEmpty()) return false;

        int irSize = 1;
        while (irSize < m_ir.size() * 2) irSize <<= 1;
        if (irSize > m_ir.size()) {
            m_ir.resize(irSize, 0.0f);
        }

        m_fftReady = true;
        return true;
    }

    bool isImpulseLoaded() const { return m_fftReady && !m_ir.isEmpty(); }
    int irLength() const { return m_ir.size(); }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        if (!m_fftReady || m_ir.isEmpty() || input.isEmpty())
            return input;

        Q_UNUSED(sampleRate)
        int n = input.size();
        QVector<float> output(n, 0.0f);
        int irLen = m_ir.size();

        for (int i = 0; i < n; ++i) {
            float dry = input[i];
            float wet = dry * m_gain;

            if (i >= m_preDelaySamples) {
                int irIdx = i - m_preDelaySamples;
                if (irIdx < irLen)
                    wet += m_ir[irIdx] * dry * 0.1f;
            }

            for (int j = 0; j < irLen && i - j >= 0; ++j) {
                wet += input[i - j] * m_ir[j];
            }

            output[i] = dry * (1.0f - m_mix) + wet * m_mix;
        }

        return output;
    }

    void reset()
    {
        m_ir.clear();
        m_fftReady = false;
        m_preDelaySamples = 0;
    }

private:
    float m_mix = 0.3f;
    float m_preDelay = 20.0f;
    float m_gain = 1.0f;
    QVector<float> m_ir;
    int m_preDelaySamples = 0;
    int m_sampleRate = 48000;
    bool m_fftReady = false;
};

class MultibandCompressor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bandCount READ bandCount WRITE setBandCount)
    Q_PROPERTY(float crossover0 READ crossover0 WRITE setCrossover0)
    Q_PROPERTY(float crossover1 READ crossover1 WRITE setCrossover1)
    Q_PROPERTY(float crossover2 READ crossover2 WRITE setCrossover2)
public:
    explicit MultibandCompressor(QObject* parent = nullptr) : QObject(parent) {
        m_bands.resize(4);
        for (int i = 0; i < 4; ++i) {
            m_bands[i].threshold = (i == 0) ? -20.0f : -24.0f;
            m_bands[i].ratio = 4.0f;
            m_bands[i].attack = 5.0f;
            m_bands[i].release = 100.0f;
            m_bands[i].makeupGain = 0.0f;
            m_bands[i].envelope = 0.0f;
        }
    }
    ~MultibandCompressor() {}

    void setBandCount(int count) { m_bandCount = qBound(2, count, 4); }
    int bandCount() const { return m_bandCount; }

    void setCrossover0(float hz) { m_crossovers[0] = qBound(20.0f, hz, 20000.0f); }
    float crossover0() const { return m_crossovers[0]; }
    void setCrossover1(float hz) { m_crossovers[1] = qBound(20.0f, hz, 20000.0f); }
    float crossover1() const { return m_crossovers[1]; }
    void setCrossover2(float hz) { m_crossovers[2] = qBound(20.0f, hz, 20000.0f); }
    float crossover2() const { return m_crossovers[2]; }

    void setBandThreshold(int band, float db) { if (band >= 0 && band < m_bands.size()) m_bands[band].threshold = db; }
    float bandThreshold(int band) const { return (band >= 0 && band < m_bands.size()) ? m_bands[band].threshold : 0.0f; }
    void setBandRatio(int band, float ratio) { if (band >= 0 && band < m_bands.size()) m_bands[band].ratio = qMax(1.0f, ratio); }
    float bandRatio(int band) const { return (band >= 0 && band < m_bands.size()) ? m_bands[band].ratio : 1.0f; }
    void setBandAttack(int band, float ms) { if (band >= 0 && band < m_bands.size()) m_bands[band].attack = qBound(0.1f, ms, 500.0f); }
    float bandAttack(int band) const { return (band >= 0 && band < m_bands.size()) ? m_bands[band].attack : 5.0f; }
    void setBandRelease(int band, float ms) { if (band >= 0 && band < m_bands.size()) m_bands[band].release = qBound(10.0f, ms, 2000.0f); }
    float bandRelease(int band) const { return (band >= 0 && band < m_bands.size()) ? m_bands[band].release : 100.0f; }
    void setBandMakeupGain(int band, float db) { if (band >= 0 && band < m_bands.size()) m_bands[band].makeupGain = db; }
    float bandMakeupGain(int band) const { return (band >= 0 && band < m_bands.size()) ? m_bands[band].makeupGain : 0.0f; }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        if (input.isEmpty()) return input;
        QVector<float> output(input.size(), 0.0f);
        int n = input.size();

        for (int b = 0; b < m_bandCount; ++b) {
            QVector<float> band(n, 0.0f);
            float lo = (b == 0) ? 20.0f : m_crossovers[b - 1];
            float hi = (b == m_bandCount - 1) ? 20000.0f : m_crossovers[b];
            float center = sqrtf(lo * hi);
            float Q = center / (hi - lo + 1.0f);
            float w0 = 2.0f * M_PI * center / sampleRate;
            float alpha = sinf(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;
            float b0 = (1.0f - cosf(w0)) / 2.0f / a0;
            float b1 = (1.0f - cosf(w0)) / a0;
            float b2 = b0;

            float v0 = 0.0f, v1 = 0.0f;
            for (int i = 0; i < n; ++i) {
                float x = input[i];
                float y = b0 * x + b1 * v0 + b2 * v1;
                v1 = v0;
                v0 = x;
                band[i] = y;
            }

            BandState& bs = m_bands[b];
            double threshold = std::pow(10.0, bs.threshold / 20.0);
            double ratio = bs.ratio;
            double attackCoeff = std::exp(-1.0 / (bs.attack * 0.001 * sampleRate));
            double releaseCoeff = std::exp(-1.0 / (bs.release * 0.001 * sampleRate));
            double makeup = std::pow(10.0, bs.makeupGain / 20.0);
            double env = bs.envelope;

            for (int i = 0; i < n; ++i) {
                double absVal = std::abs(static_cast<double>(band[i]));
                env = (absVal > env)
                    ? attackCoeff * env + (1.0 - attackCoeff) * absVal
                    : releaseCoeff * env + (1.0 - releaseCoeff) * absVal;
                if (env > threshold) {
                    double gr = threshold + (env - threshold) / ratio;
                    double gain = (env > 1e-10) ? gr / env : 1.0;
                    output[i] += static_cast<float>(band[i] * gain * makeup);
                } else {
                    output[i] += static_cast<float>(band[i] * makeup);
                }
            }
            bs.envelope = static_cast<float>(env);
        }

        return output;
    }

    void reset()
    {
        for (int i = 0; i < m_bands.size(); ++i)
            m_bands[i].envelope = 0.0f;
    }

private:
    struct BandState {
        float threshold;
        float ratio;
        float attack;
        float release;
        float makeupGain;
        float envelope;
    };

    int m_bandCount = 3;
    float m_crossovers[3] = {250.0f, 2000.0f, 8000.0f};
    QVector<BandState> m_bands;
};

class TapeEmulator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float drive READ drive WRITE setDrive)
    Q_PROPERTY(float mix READ mix WRITE setMix)
    Q_PROPERTY(float wowRate READ wowRate WRITE setWowRate)
    Q_PROPERTY(float wowDepth READ wowDepth WRITE setWowDepth)
    Q_PROPERTY(float hissLevel READ hissLevel WRITE setHissLevel)
    Q_PROPERTY(float bias READ bias WRITE setBias)
public:
    explicit TapeEmulator(QObject* parent = nullptr) : QObject(parent) {}
    ~TapeEmulator() {}

    void setDrive(float drive) { m_drive = qBound(0.0f, drive, 1.0f); }
    float drive() const { return m_drive; }
    void setMix(float mix) { m_mix = qBound(0.0f, mix, 1.0f); }
    float mix() const { return m_mix; }
    void setWowRate(float hz) { m_wowRate = qBound(0.1f, hz, 20.0f); }
    float wowRate() const { return m_wowRate; }
    void setWowDepth(float depth) { m_wowDepth = qBound(0.0f, depth, 1.0f); }
    float wowDepth() const { return m_wowDepth; }
    void setHissLevel(float level) { m_hissLevel = qBound(0.0f, level, 1.0f); }
    float hissLevel() const { return m_hissLevel; }
    void setBias(float bias) { m_bias = qBound(-1.0f, bias, 1.0f); }
    float bias() const { return m_bias; }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        if (input.isEmpty()) return input;
        QVector<float> output(input.size());
        float wowInc = 2.0f * M_PI * m_wowRate / sampleRate;

        for (int i = 0; i < input.size(); ++i) {
            float dry = input[i];

            float driveAmt = 1.0f + m_drive * 20.0f;
            float saturated = dry * driveAmt;
            saturated = std::tanh(saturated + m_bias * 0.3f) / std::tanh(1.0f + m_bias * 0.3f);

            float wow = 1.0f + sinf(m_wowPhase) * m_wowDepth * 0.005f;
            m_wowPhase += wowInc;
            if (m_wowPhase > 2.0f * M_PI) m_wowPhase -= 2.0f * M_PI;

            float hiss = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * m_hissLevel * 0.02f;

            float wet = saturated * wow + hiss;
            output[i] = dry * (1.0f - m_mix) + wet * m_mix;
        }

        return output;
    }

    void reset()
    {
        m_wowPhase = 0.0f;
    }

private:
    float m_drive = 0.3f;
    float m_mix = 0.5f;
    float m_wowRate = 4.0f;
    float m_wowDepth = 0.3f;
    float m_hissLevel = 0.1f;
    float m_bias = 0.0f;
    float m_wowPhase = 0.0f;
};

class GuitarAmpSimulator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float gain READ gain WRITE setGain)
    Q_PROPERTY(float bass READ bass WRITE setBass)
    Q_PROPERTY(float mid READ mid WRITE setMid)
    Q_PROPERTY(float treble READ treble WRITE setTreble)
    Q_PROPERTY(float volume READ volume WRITE setVolume)
    Q_PROPERTY(float drive READ drive WRITE setDrive)
    Q_PROPERTY(float presence READ presence WRITE setPresence)
public:
    explicit GuitarAmpSimulator(QObject* parent = nullptr) : QObject(parent) {}
    ~GuitarAmpSimulator() {}

    void setGain(float gain) { m_gain = qBound(0.0f, gain, 1.0f); }
    float gain() const { return m_gain; }
    void setBass(float bass) { m_bass = qBound(0.0f, bass, 1.0f); }
    float bass() const { return m_bass; }
    void setMid(float mid) { m_mid = qBound(0.0f, mid, 1.0f); }
    float mid() const { return m_mid; }
    void setTreble(float treble) { m_treble = qBound(0.0f, treble, 1.0f); }
    float treble() const { return m_treble; }
    void setVolume(float vol) { m_volume = qBound(0.0f, vol, 2.0f); }
    float volume() const { return m_volume; }
    void setDrive(float drive) { m_drive = qBound(0.0f, drive, 1.0f); }
    float drive() const { return m_drive; }
    void setPresence(float presence) { m_presence = qBound(0.0f, presence, 1.0f); }
    float presence() const { return m_presence; }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        if (input.isEmpty()) return input;
        QVector<float> output(input.size());

        double w0_bass = 2.0 * M_PI * 250.0 / sampleRate;
        double w0_mid = 2.0 * M_PI * 1500.0 / sampleRate;
        double w0_treb = 2.0 * M_PI * 5000.0 / sampleRate;

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];

            float preGain = 1.0f + m_gain * 30.0f;
            s *= preGain;

            float preClip = std::tanh(s);
            float driveAmt = 1.0f + m_drive * 40.0f;
            float dist = std::tanh(preClip * driveAmt);
            s = dist;

            float b = onePoleLP(s, sampleRate, 250.0f, m_bassLP);
            b *= m_bass;
            float m = onePoleBP(s, sampleRate, 1500.0f, m_midBP);
            m *= m_mid;
            float t = onePoleHP(s, sampleRate, 5000.0f, m_trebHP);
            t *= m_treble;

            s = (b + m + t) * 0.5f;

            float presFilter = 1.0f + m_presence * 6.0f;
            s *= presFilter;

            s = std::tanh(s);

            s *= m_volume;

            output[i] = s;
        }

        return output;
    }

    void reset()
    {
        m_bassLP = 0.0f;
        m_midBP = 0.0f;
        m_trebHP = 0.0f;
    }

private:
    float onePoleLP(float input, int sampleRate, float freq, float& z)
    {
        float rc = 1.0f / (2.0f * M_PI * freq);
        float dt = 1.0f / sampleRate;
        float alpha = dt / (rc + dt);
        z = z + alpha * (input - z);
        return z;
    }

    float onePoleHP(float input, int sampleRate, float freq, float& z)
    {
        float rc = 1.0f / (2.0f * M_PI * freq);
        float dt = 1.0f / sampleRate;
        float alpha = rc / (rc + dt);
        z = alpha * z + alpha * (input - z);
        return input - z;
    }

    float onePoleBP(float input, int sampleRate, float freq, float& z)
    {
        float rc = 1.0f / (2.0f * M_PI * freq);
        float dt = 1.0f / sampleRate;
        float alpha = dt / (rc + dt);
        z = z + alpha * (input - z);
        float hp = input - z;
        float rc2 = 1.0f / (2.0f * M_PI * freq * 0.5f);
        float alpha2 = dt / (rc2 + dt);
        return z + alpha2 * (hp - z);
    }

    float m_gain = 0.5f;
    float m_bass = 0.5f;
    float m_mid = 0.5f;
    float m_treble = 0.5f;
    float m_volume = 0.8f;
    float m_drive = 0.3f;
    float m_presence = 0.3f;
    float m_bassLP = 0.0f;
    float m_midBP = 0.0f;
    float m_trebHP = 0.0f;
};

class TransientDesigner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float attack READ attack WRITE setAttack)
    Q_PROPERTY(float sustain READ sustain WRITE setSustain)
    Q_PROPERTY(float sensitivity READ sensitivity WRITE setSensitivity)
public:
    explicit TransientDesigner(QObject* parent = nullptr) : QObject(parent) {}
    ~TransientDesigner() {}

    void setAttack(float attack) { m_attackGain = qBound(-20.0f, attack, 20.0f); }
    float attack() const { return m_attackGain; }
    void setSustain(float sustain) { m_sustainGain = qBound(-20.0f, sustain, 20.0f); }
    float sustain() const { return m_sustainGain; }
    void setSensitivity(float sens) { m_sensitivity = qBound(0.0f, sens, 1.0f); }
    float sensitivity() const { return m_sensitivity; }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        if (input.isEmpty()) return input;
        QVector<float> output(input.size());

        float attackLin = std::pow(10.0f, m_attackGain / 20.0f);
        float sustainLin = std::pow(10.0f, m_sustainGain / 20.0f);

        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        float holdMs = 5.0f;
        int attackSamps = static_cast<int>(attackMs * sampleRate / 1000.0f);
        int releaseSamps = static_cast<int>(releaseMs * sampleRate / 1000.0f);
        int holdSamps = static_cast<int>(holdMs * sampleRate / 1000.0f);

        float detectionThreshold = 0.01f * (1.0f + m_sensitivity * 9.0f);
        int lookAhead = 2;

        for (int i = 0; i < input.size(); ++i) {
            float s = input[i];
            float absS = std::abs(s);

            if (m_env < absS) {
                m_env += (absS - m_env) * 0.5f;
            } else {
                m_env += (absS - m_env) * 0.001f;
            }

            if (m_env > detectionThreshold && i - m_lastTransient > holdSamps) {
                bool isTransient = true;
                if (i + lookAhead < input.size()) {
                    float future = std::abs(input[i + lookAhead]);
                    isTransient = m_env * 1.5f < future;
                }
                if (isTransient) {
                    m_transientEnv = 1.0f;
                    m_lastTransient = i;
                }
            }

            if (m_transientEnv > 0.0f) {
                m_transientPos++;
                if (m_transientPos > attackSamps) {
                    m_transientEnv -= 1.0f / releaseSamps;
                    if (m_transientEnv < 0.0f) m_transientEnv = 0.0f;
                }
            }

            float attackMix = m_transientEnv;
            float sustainMix = 1.0f - m_transientEnv;
            float gain = attackMix * attackLin + sustainMix * sustainLin;
            output[i] = s * gain;
        }

        return output;
    }

    void reset()
    {
        m_env = 0.0f;
        m_transientEnv = 0.0f;
        m_transientPos = 0;
        m_lastTransient = -1000;
    }

private:
    float m_attackGain = 6.0f;
    float m_sustainGain = 0.0f;
    float m_sensitivity = 0.5f;
    float m_env = 0.0f;
    float m_transientEnv = 0.0f;
    int m_transientPos = 0;
    int m_lastTransient = -1000;
};

class StereoEnhancer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float width READ width WRITE setWidth)
    Q_PROPERTY(float midGain READ midGain WRITE setMidGain)
    Q_PROPERTY(float sideGain READ sideGain WRITE setSideGain)
    Q_PROPERTY(bool swapChannels READ swapChannels WRITE setSwapChannels)
public:
    explicit StereoEnhancer(QObject* parent = nullptr) : QObject(parent) {}
    ~StereoEnhancer() {}

    void setWidth(float width) { m_width = qBound(-1.0f, width, 1.0f); }
    float width() const { return m_width; }
    void setMidGain(float gain) { m_midGain = qBound(-12.0f, gain, 12.0f); }
    float midGain() const { return m_midGain; }
    void setSideGain(float gain) { m_sideGain = qBound(-12.0f, gain, 12.0f); }
    float sideGain() const { return m_sideGain; }
    void setSwapChannels(bool swap) { m_swap = swap; }
    bool swapChannels() const { return m_swap; }

    QVector<float> process(const QVector<float>& input, int sampleRate)
    {
        Q_UNUSED(sampleRate)
        if (input.size() < 2) return input;

        int frames = input.size() / 2;
        QVector<float> output(input.size());

        float midLin = std::pow(10.0f, m_midGain / 20.0f);
        float sideLin = std::pow(10.0f, m_sideGain / 20.0f);
        float w = m_width;

        for (int i = 0; i < frames; ++i) {
            float L = input[i * 2];
            float R = input[i * 2 + 1];

            if (m_swap) {
                float t = L;
                L = R;
                R = t;
            }

            float mid = (L + R) * 0.5f;
            float side = (L - R) * 0.5f;

            side *= (1.0f + w);

            mid *= midLin;
            side *= sideLin;

            output[i * 2] = mid + side;
            output[i * 2 + 1] = mid - side;
        }

        return output;
    }

    void reset() {}

private:
    float m_width = 0.0f;
    float m_midGain = 0.0f;
    float m_sideGain = 0.0f;
    bool m_swap = false;
};

} // namespace audio
} // namespace ks
