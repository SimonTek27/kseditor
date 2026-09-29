#pragma once

#include <QObject>
#include <QVector>

class PeakMeter : public QObject
{
    Q_OBJECT
public:
    explicit PeakMeter(QObject *parent = nullptr);
    ~PeakMeter() override = default;

    void setSampleRate(int sampleRate);
    void setChannelCount(int channels);
    void setPeakDecay(float decayMs);

    void process(const float *samples, int frameCount);
    void process(const QVector<float> &samples);

    float getPeakLevel(int channel) const;
    float getRMSLevel(int channel) const;
    float getLeftRMS() const;
    float getRightRMS() const;

signals:
    void levelsChanged(float left, float right);

private:
    int m_sampleRate = 44100;
    int m_channels = 2;
    float m_peakDecayMs = 1000.0f;
    QVector<float> m_peak;
    QVector<float> m_rms;
};
