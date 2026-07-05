#include "FFTProcessor.h"
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <complex>

using QComplex = std::complex<float>;

FFTProcessor::FFTProcessor(QObject *parent)
    : QObject(parent)
    , m_fftSize(2048)
    , m_windowType(0)
    , m_noiseFloor(-90.0f)
    , m_spectralFloor(-90.0f)
{
    initFFT();
}

FFTProcessor::~FFTProcessor()
{
    destroyFFT();
}

void FFTProcessor::initFFT()
{
    m_hannWindow.resize(m_fftSize);
    m_hammingWindow.resize(m_fftSize);
    m_blackmanWindow.resize(m_fftSize);

    for (int i = 0; i < m_fftSize; ++i) {
        m_hannWindow[i] = 0.5f * (1.0f - qCos(2.0f * M_PI * i / (m_fftSize - 1)));
        m_hammingWindow[i] = 0.54f - 0.46f * qCos(2.0f * M_PI * i / (m_fftSize - 1));
        m_blackmanWindow[i] = 0.42f - 0.5f * qCos(2.0f * M_PI * i / (m_fftSize - 1))
                           + 0.08f * qCos(4.0f * M_PI * i / (m_fftSize - 1));
    }

    m_fftBuffer.resize(m_fftSize);
}

void FFTProcessor::destroyFFT()
{
    m_hannWindow.clear();
    m_hammingWindow.clear();
    m_blackmanWindow.clear();
    m_fftBuffer.clear();
}

void FFTProcessor::setFFTSize(int size)
{
    if (size < 64 || size > 16384) return;

    m_fftSize = size;
    initFFT();
}

void FFTProcessor::setWindowType(int type)
{
    m_windowType = type;
}

QVector<float> FFTProcessor::applyWindow(const QVector<float> &samples)
{
    const QVector<float> &window = (m_windowType == 0) ? m_hannWindow
                                   : (m_windowType == 1) ? m_hammingWindow
                                   : m_blackmanWindow;

    QVector<float> result(m_fftSize);
    int windowSize = qMin(samples.size(), m_fftSize);

    for (int i = 0; i < windowSize; ++i) {
        result[i] = samples[i] * window[i];
    }
    for (int i = windowSize; i < m_fftSize; ++i) {
        result[i] = 0.0f;
    }

    return result;
}

void FFTProcessor::fft(QVector<QComplex> &data)
{
    int n = data.size();
    if (n <= 1) return;

    QVector<QComplex> result(n);
    int bits = static_cast<int>(std::floor(std::log(static_cast<double>(n)) / std::log(2.0) + 0.5));
    int j = 0;

    for (int i = 0; i < n - 1; ++i) {
        if (i < j) {
            qSwap(data[i], data[j]);
        }
        int k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    for (int len = 2; len <= n; len *= 2) {
        float angle = -2.0f * M_PI / len;
        QComplex wn(cos(angle), sin(angle));

        for (int i = 0; i < n; i += len) {
            QComplex w(1.0f, 0.0f);
            for (int k = 0; k < len / 2; ++k) {
                QComplex u = data[i + k];
                QComplex t = w * data[i + k + len / 2];
                data[i + k] = u + t;
                data[i + k + len / 2] = u - t;
                w *= wn;
            }
        }
    }
}

void FFTProcessor::ifft(QVector<QComplex> &data)
{
    int n = data.size();
    for (int i = 0; i < n; ++i) {
        data[i] = QComplex(data[i].real(), -data[i].imag());
    }

    fft(data);

    for (int i = 0; i < n; ++i) {
        data[i] = QComplex(data[i].real() / n, data[i].imag() / n);
    }
}

void FFTProcessor::fftReal(const QVector<float> &input, QVector<QComplex> &output)
{
    output.resize(m_fftSize);

    for (int i = 0; i < m_fftSize; ++i) {
        if (i < input.size()) {
            output[i] = QComplex(input[i], 0.0f);
        } else {
            output[i] = QComplex(0.0f, 0.0f);
        }
    }

    fft(output);
}

QVector<float> FFTProcessor::computeSpectrum(const QVector<float> &samples)
{
    QVector<float> windowed = applyWindow(samples);
    QVector<QComplex> fftData;
    fftReal(windowed, fftData);

    QVector<float> magnitudes(m_fftSize / 2);

    for (int i = 0; i < m_fftSize / 2; ++i) {
        magnitudes[i] = sqrt(fftData[i].real() * fftData[i].real()
                           + fftData[i].imag() * fftData[i].imag()) / m_fftSize;
    }

    return magnitudes;
}

QVector<float> FFTProcessor::computeMagnitudes(const QVector<float> &samples)
{
    return computeSpectrum(samples);
}

QVector<float> FFTProcessor::computeLogMagnitudes(const QVector<float> &samples)
{
    QVector<float> linear = computeSpectrum(samples);
    QVector<float> logMags(m_fftSize / 2);

    for (int i = 0; i < linear.size(); ++i) {
        float val = linear[i];
        if (val < 1e-10f) val = 1e-10f;
        logMags[i] = 20.0f * std::log(val) / qLn(10.0f);

        if (logMags[i] < m_spectralFloor) {
            logMags[i] = m_spectralFloor;
        }
    }

    return logMags;
}

QVector<float> FFTProcessor::computePhase(const QVector<float> &samples)
{
    QVector<float> windowed = applyWindow(samples);
    QVector<QComplex> fftData;
    fftReal(windowed, fftData);

    QVector<float> phases(m_fftSize / 2);

    for (int i = 0; i < m_fftSize / 2; ++i) {
        phases[i] = qAtan2(fftData[i].imag(), fftData[i].real());
    }

    return phases;
}

QVector<float> FFTProcessor::getFrequencyBands(const QVector<float> &samples, int bandCount)
{
    QVector<float> spectrum = computeLogMagnitudes(samples);
    QVector<float> bands(bandCount);

    int binsPerBand = (m_fftSize / 2) / bandCount;
    if (binsPerBand == 0) binsPerBand = 1;

    for (int b = 0; b < bandCount; ++b) {
        float sum = 0.0f;
        int startBin = b * binsPerBand;
        int endBin = qMin(startBin + binsPerBand, spectrum.size());

        for (int i = startBin; i < endBin; ++i) {
            sum += spectrum[i];
        }
        bands[b] = sum / (endBin - startBin);
    }

    return bands;
}

float FFTProcessor::hzToMel(float hz)
{
    return 2595.0f * std::log(1.0f + hz / 700.0f) / qLn(10.0f);
}

float FFTProcessor::melToHz(float mel)
{
    return 700.0f * (qPow(10.0f, mel / 2595.0f) - 1.0f);
}

QVector<float> FFTProcessor::getMelSpectrum(const QVector<float> &samples, int melBands)
{
    QVector<float> spectrum = computeLogMagnitudes(samples);
    QVector<float> melBandsResult(melBands);

    float minMel = hzToMel(0.0f);
    float maxMel = hzToMel(m_sampleRate / 2);
    float binWidth = (m_fftSize / 2) / (m_sampleRate / 2.0f);

    for (int m = 0; m < melBands; ++m) {
        float melLow = minMel + m * (maxMel - minMel) / (melBands + 1);
        float melHigh = minMel + (m + 2) * (maxMel - minMel) / (melBands + 1);

        float fLow = melToHz(melLow);
        float fHigh = melToHz(melHigh);

        int binLow = qMax(0, int(fLow * binWidth));
        int binHigh = qMin(spectrum.size() - 1, int(fHigh * binWidth));

        float sum = 0.0f;
        int count = binHigh - binLow + 1;
        for (int i = binLow; i <= binHigh; ++i) {
            sum += spectrum[i];
        }

        melBandsResult[m] = count > 0 ? sum / count : 0.0f;
    }

    return melBandsResult;
}

QVector<float> FFTProcessor::generateNoiseProfile(const QVector<float> &noiseSamples)
{
    QVector<QComplex> fftData;
    fftReal(noiseSamples, fftData);

    QVector<float> profile(m_fftSize / 2);

    for (int i = 0; i < m_fftSize / 2; ++i) {
        float mag = sqrt(fftData[i].real() * fftData[i].real()
                       + fftData[i].imag() * fftData[i].imag()) / m_fftSize;
        profile[i] = mag * mag;
    }

    return profile;
}

QVector<float> FFTProcessor::spectralSubtraction(const QVector<float> &samples,
                                                  const QVector<float> &noiseProfile)
{
    QVector<float> windowed = applyWindow(samples);
    QVector<QComplex> fftData;
    fftReal(windowed, fftData);

    QVector<float> result(samples.size());

    for (int i = 0; i < m_fftSize / 2; ++i) {
        float mag = sqrt(fftData[i].real() * fftData[i].real()
                        + fftData[i].imag() * fftData[i].imag());

        float noiseMag = 0.0f;
        if (i < noiseProfile.size()) {
            noiseMag = sqrt(noiseProfile[i]) * 2.0f;
        }

        float subtracted = mag - noiseMag;
        if (subtracted < 0) subtracted = 0;

        float phase = qAtan2(fftData[i].imag(), fftData[i].real());
        fftData[i] = QComplex(subtracted * qCos(phase), subtracted * qSin(phase));

        if (i > 0) {
            fftData[m_fftSize - i] = QComplex(subtracted * qCos(-phase),
                                              subtracted * qSin(-phase));
        }
    }

    ifft(fftData);

    for (int i = 0; i < result.size() && i < m_fftSize; ++i) {
        result[i] = fftData[i].real();
    }

    return result;
}

NoiseReducer::NoiseReducer(QObject *parent)
    : QObject(parent)
    , m_fft(new FFTProcessor(this))
    , m_reductionDb(-15.0f)
    , m_smoothingFactor(0.3f)
    , m_noiseFloorDb(-80.0f)
{
    m_fft->setFFTSize(2048);
}

NoiseReducer::~NoiseReducer() = default;

void NoiseReducer::setNoiseProfile(const QVector<float> &profile)
{
    m_noiseProfile = profile;
}

void NoiseReducer::setReductionAmount(float db)
{
    m_reductionDb = db;
}

void NoiseReducer::setSmoothing(float factor)
{
    m_smoothingFactor = qBound(0.0f, factor, 1.0f);
}

void NoiseReducer::captureNoiseProfile(const QVector<float> &noiseSamples)
{
    m_noiseProfile = m_fft->generateNoiseProfile(noiseSamples);
    emit noiseProfileCaptured();
    qDebug() << "NoiseReducer: Captured noise profile with" << m_noiseProfile.size() << "bins";
}

QVector<float> NoiseReducer::reduceNoise(const QVector<float> &samples)
{
    if (m_noiseProfile.isEmpty()) {
        qWarning() << "NoiseReducer: No noise profile available";
        return samples;
    }

    return m_fft->spectralSubtraction(samples, m_noiseProfile);
}

PeakMeter::PeakMeter(QObject *parent)
    : QObject(parent)
    , m_sampleRate(44100)
    , m_channels(2)
    , m_leftPeak(0.0f)
    , m_rightPeak(0.0f)
    , m_leftRMS(0.0f)
    , m_rightRMS(0.0f)
    , m_decayFactor(0.995f)
    , m_holdPeakLeft(0.0f)
    , m_holdPeakRight(0.0f)
    , m_holdSamples(0)
{
}

PeakMeter::~PeakMeter() = default;

void PeakMeter::setPeakDecay(float decayMs)
{
    if (decayMs <= 0) decayMs = 1;
    m_decayFactor = exp(-1.0f / (m_sampleRate * decayMs / 1000.0f));
}

void PeakMeter::processSamples(const QVector<float> &samples)
{
    int samplesPerChannel = samples.size() / m_channels;

    float sumL = 0.0f, sumR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;

    for (int i = 0; i < samples.size(); i += m_channels) {
        float leftSample = samples[i];
        float rightSample = (m_channels > 1 && i + 1 < samples.size()) ? samples[i + 1] : leftSample;

        float absL = qAbs(leftSample);
        float absR = qAbs(rightSample);

        peakL = qMax(peakL, absL);
        peakR = qMax(peakR, absR);

        sumL += leftSample * leftSample;
        sumR += rightSample * rightSample;
    }

    m_leftRMS = sqrt(sumL / samplesPerChannel);
    m_rightRMS = sqrt(sumR / samplesPerChannel);

    m_leftPeak = m_leftPeak * m_decayFactor + peakL * (1.0f - m_decayFactor);
    m_rightPeak = m_rightPeak * m_decayFactor + peakR * (1.0f - m_decayFactor);

    if (peakL > m_holdPeakLeft) {
        m_holdPeakLeft = peakL;
        m_holdSamples = 0;
    } else {
        ++m_holdSamples;
        if (m_holdSamples > m_sampleRate / 10) {
            m_holdPeakLeft *= 0.99f;
        }
    }

    if (peakR > m_holdPeakRight) {
        m_holdPeakRight = peakR;
        m_holdSamples = 0;
    } else {
        ++m_holdSamples;
        if (m_holdSamples > m_sampleRate / 10) {
            m_holdPeakRight *= 0.99f;
        }
    }

    emit levelsChanged(m_leftPeak, m_rightPeak);

    if (m_leftPeak > 0.99f) emit clippingDetected(0);
    if (m_rightPeak > 0.99f) emit clippingDetected(1);
}

void PeakMeter::reset()
{
    m_leftPeak = 0.0f;
    m_rightPeak = 0.0f;
    m_leftRMS = 0.0f;
    m_rightRMS = 0.0f;
    m_holdPeakLeft = 0.0f;
    m_holdPeakRight = 0.0f;
}

float PeakMeter::getPeakLevel(int channel) const
{
    return (channel == 0) ? m_leftPeak : m_rightPeak;
}

float PeakMeter::getRMSLevel(int channel) const
{
    return (channel == 0) ? m_leftRMS : m_rightRMS;
}

float PeakMeter::getPeakLevelDb(int channel) const
{
    float peak = getPeakLevel(channel);
    if (peak < 1e-10f) return -120.0f;
    return 20.0f * std::log(peak) / qLn(10.0f);
}

float PeakMeter::getRMSLevelDb(int channel) const
{
    float rms = getRMSLevel(channel);
    if (rms < 1e-10f) return -120.0f;
    return 20.0f * std::log(rms) / qLn(10.0f);
}

bool PeakMeter::isClipping(int channel) const
{
    return getPeakLevel(channel) > 0.99f;
}