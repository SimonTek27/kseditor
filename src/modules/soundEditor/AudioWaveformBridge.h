#pragma once

#include <QObject>
#include <QVector>
#include <QQuickItem>

namespace ks {

class AudioWaveformBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int sampleRate READ sampleRate NOTIFY dataChanged)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY dataChanged)
    Q_PROPERTY(int totalSamples READ totalSamples NOTIFY dataChanged)
    Q_PROPERTY(double duration READ duration NOTIFY dataChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY dataChanged)

public:
    explicit AudioWaveformBridge(QObject* parent = nullptr);
    ~AudioWaveformBridge() override = default;

    // Load audio file and prepare waveform data
    bool loadFile(const QString& path);

    // Get waveform data for rendering (downsampled for display)
    // Returns QVector<QPointF> with x = time (normalized 0-1), y = amplitude (-1 to 1)
    Q_INVOKABLE QVector<QPointF> getWaveformPoints(int width, int channel = 0) const;

    // Get raw sample at time
    Q_INVOKABLE float getSampleAtTime(double time, int channel = 0) const;

    // Convert sample index to time (seconds)
    Q_INVOKABLE double sampleToTime(int sample) const;

    // Convert time to sample index
    Q_INVOKABLE int timeToSample(double time) const;

    int sampleRate() const { return m_sampleRate; }
    int channelCount() const { return m_channels; }
    int totalSamples() const { return m_audioData.size() / m_channels; }
    double duration() const { return m_channels > 0 ? double(totalSamples()) / m_sampleRate : 0.0; }
    bool hasData() const { return !m_audioData.isEmpty(); }

    // Peak amplitude
    float peakAmplitude() const { return m_peakAmplitude; }
    float rmsAmplitude() const { return m_rmsAmplitude; }

    // Clear data
    void clear();

signals:
    void dataChanged();
    void loadComplete(bool success);
    void error(const QString& msg);

private:
    QVector<float> m_audioData;
    int m_sampleRate = 44100;
    int m_channels = 2;
    float m_peakAmplitude = 0.0f;
    float m_rmsAmplitude = 0.0f;

    void analyzeAudio();
};

} // namespace ks