#ifndef FFT_PROCESSOR_H
#define FFT_PROCESSOR_H

#include <QObject>
#include <QVector>
#include <complex>
#include <QtMath>

class FFTProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FFTProcessor(QObject *parent = nullptr);
    ~FFTProcessor();

    void setFFTSize(int size);
    int getFFTSize() const { return m_fftSize; }

    void setWindowType(int type);
    int getWindowType() const { return m_windowType; }

    QVector<float> computeSpectrum(const QVector<float> &samples);
    QVector<float> computeMagnitudes(const QVector<float> &samples);
    QVector<float> computeLogMagnitudes(const QVector<float> &samples);
    QVector<float> computePhase(const QVector<float> &samples);

    QVector<float> applyWindow(const QVector<float> &samples);

    QVector<float> getFrequencyBands(const QVector<float> &samples, int bandCount);
    QVector<float> getMelSpectrum(const QVector<float> &samples, int melBands);

    QVector<float> generateNoiseProfile(const QVector<float> &noiseSamples);
    QVector<float> spectralSubtraction(const QVector<float> &samples, const QVector<float> &noiseProfile);

    static float hzToMel(float hz);
    static float melToHz(float mel);

    void fft(QVector<std::complex<float>> &data);
    void ifft(QVector<std::complex<float>> &data);
    void fftReal(const QVector<float> &input, QVector<std::complex<float>> &output);

signals:
    void spectrumComputed(const QVector<float> &magnitudes);
    void analysisComplete();

private:
    void initFFT();
    void destroyFFT();

    QVector<float> m_hannWindow;
    QVector<float> m_hammingWindow;
    QVector<float> m_blackmanWindow;

    int m_fftSize;
    int m_windowType;

    QVector<std::complex<float>> m_fftBuffer;

    float m_noiseFloor;
    float m_spectralFloor;
    int m_sampleRate = 44100;
};

class NoiseReducer : public QObject
{
    Q_OBJECT

public:
    explicit NoiseReducer(QObject *parent = nullptr);
    ~NoiseReducer();

    void setNoiseProfile(const QVector<float> &profile);
    void setReductionAmount(float db);
    void setSmoothing(float factor);
    void setFFTProcessor(FFTProcessor *fft) { m_fft = fft; }

    QVector<float> reduceNoise(const QVector<float> &samples);
    void captureNoiseProfile(const QVector<float> &noiseSamples);

    float getNoiseFloor() const { return m_noiseFloorDb; }
    bool hasProfile() const { return !m_noiseProfile.isEmpty(); }

signals:
    void processingProgress(int percent);
    void noiseProfileCaptured();
    void noiseReductionComplete();

private:
    FFTProcessor *m_fft;
    QVector<float> m_noiseProfile;
    float m_reductionDb;
    float m_smoothingFactor;
    float m_noiseFloorDb;

    float dbToLinear(float db) const { return qPow(10.0f, db / 20.0f); }
    float linearToDb(float linear) const { return 20.0f * std::log(linear) / std::log(10.0f); }
};

class PeakMeter : public QObject
{
    Q_OBJECT

public:
    explicit PeakMeter(QObject *parent = nullptr);
    ~PeakMeter();

    void setSampleRate(int rate) { m_sampleRate = rate; }
    void setChannelCount(int channels) { m_channels = channels; }
    void setPeakDecay(float decayMs);

    void processSamples(const QVector<float> &samples);
    void reset();

    float getPeakLevel(int channel = 0) const;
    float getRMSLevel(int channel = 0) const;
    float getPeakLevelDb(int channel = 0) const;
    float getRMSLevelDb(int channel = 0) const;
    float getLeftPeak() const { return m_leftPeak; }
    float getRightPeak() const { return m_rightPeak; }
    float getLeftRMS() const { return m_leftRMS; }
    float getRightRMS() const { return m_rightRMS; }

    bool isClipping(int channel = 0) const;

signals:
    void levelsChanged(float leftPeak, float rightPeak);
    void clippingDetected(int channel);

private:
    void updatePeaks(const QVector<float> &samples);

    int m_sampleRate;
    int m_channels;

    float m_leftPeak;
    float m_rightPeak;
    float m_leftRMS;
    float m_rightRMS;
    float m_decayFactor;

    float m_holdPeakLeft;
    float m_holdPeakRight;
    int m_holdSamples;
};

#endif // FFT_PROCESSOR_H