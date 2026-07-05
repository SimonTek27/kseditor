#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <complex>
#include <cmath>

namespace ks {

class AudioFFT : public QObject
{
    Q_OBJECT
public:
    enum WindowType {
        Hann,
        Hamming,
        Blackman
    };
    Q_ENUM(WindowType)

    explicit AudioFFT(QObject* parent = nullptr) : QObject(parent) {}
    ~AudioFFT() {}

    void setFFTSize(int size) { m_fftSize = size; m_bins.resize(size / 2 + 1); }
    int getFFTSize() const { return m_fftSize; }

    void setWindowType(WindowType type) { m_windowType = type; }
    WindowType getWindowType() const { return m_windowType; }

    void setSampleRate(int rate) { m_sampleRate = rate; }
    int sampleRate() const { return m_sampleRate; }

    QVector<float> computeFFT(const QVector<float>& input) {
        int n = qMin(m_fftSize, input.size());
        if (n < 2) return QVector<float>();

        QVector<std::complex<float>> buffer(m_fftSize, {0.0f, 0.0f});
        for (int i = 0; i < n; ++i) {
            float w = applyWindow(i);
            buffer[i] = {input[i] * w, 0.0f};
        }

        int log2n = 0;
        int tmp = m_fftSize;
        while (tmp > 1) { tmp >>= 1; log2n++; }

        for (int s = 1; s <= log2n; ++s) {
            int m = 1 << s;
            float angle = -2.0f * M_PI / m;
            std::complex<float> wm(cosf(angle), sinf(angle));
            for (int k = 0; k < m_fftSize; k += m) {
                std::complex<float> w(1.0f, 0.0f);
                for (int j = 0; j < m / 2; ++j) {
                    auto t = w * buffer[k + j + m / 2];
                    auto u = buffer[k + j];
                    buffer[k + j] = u + t;
                    buffer[k + j + m / 2] = u - t;
                    w *= wm;
                }
            }
        }

        QVector<float> magnitude(m_fftSize / 2 + 1);
        for (int i = 0; i <= m_fftSize / 2; ++i) {
            magnitude[i] = sqrtf(buffer[i].real() * buffer[i].real() +
                                  buffer[i].imag() * buffer[i].imag()) / m_fftSize;
        }
        return magnitude;
    }

    QVector<float> getFrequencyBins() const {
        QVector<float> bins(m_fftSize / 2 + 1);
        float binWidth = static_cast<float>(m_sampleRate) / m_fftSize;
        for (int i = 0; i <= m_fftSize / 2; ++i) {
            bins[i] = i * binWidth;
        }
        return bins;
    }

    void reset() {
        m_fftSize = 1024;
        m_windowType = Hann;
    }

signals:
    void computed(const QVector<float>& magnitude);

private:
    float applyWindow(int index) const {
        switch (m_windowType) {
        case Hann:
            return 0.5f * (1.0f - cosf(2.0f * M_PI * index / m_fftSize));
        case Hamming:
            return 0.54f - 0.46f * cosf(2.0f * M_PI * index / m_fftSize);
        case Blackman:
            return 0.42f - 0.5f * cosf(2.0f * M_PI * index / m_fftSize)
                         + 0.08f * cosf(4.0f * M_PI * index / m_fftSize);
        }
        return 1.0f;
    }

    int m_fftSize = 1024;
    WindowType m_windowType = Hann;
    int m_sampleRate = 44100;
    QVector<float> m_bins;
};

} // namespace ks
