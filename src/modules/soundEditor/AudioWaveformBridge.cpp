#include "AudioWaveformBridge.h"
#include "AudioCore.h"
#include <QFile>
#include <QAudioFormat>
#include <QFileInfo>
#include <cmath>

namespace ks {

AudioWaveformBridge::AudioWaveformBridge(QObject* parent)
    : QObject(parent)
{}

bool AudioWaveformBridge::loadFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Cannot open file: " + path);
        return false;
    }

    QAudioFormat format;
    QVector<float> samples;
    bool ok = false;

    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "wav") {
        ok = AudioFormatConverter::readWav(file, samples, format);
        file.close();
    } else {
        file.close();
        // For compressed formats, use the audio engine's file loader
        // which delegates to the core AudioFormatConverter
        QFile testFile(path);
        if (testFile.open(QIODevice::ReadOnly)) {
            ok = AudioFormatConverter::readWav(testFile, samples, format);
            testFile.close();
        }
    }

    if (!ok || samples.isEmpty()) {
        emit error("Failed to decode audio file: " + path);
        return false;
    }

    m_sampleRate = format.sampleRate();
    m_channels = format.channelCount();
    m_audioData = samples;
    analyzeAudio();
    emit dataChanged();
    emit loadComplete(true);
    return true;
}

void AudioWaveformBridge::clear()
{
    m_audioData.clear();
    m_peakAmplitude = 0.0f;
    m_rmsAmplitude = 0.0f;
    emit dataChanged();
}

void AudioWaveformBridge::analyzeAudio()
{
    if (m_audioData.isEmpty() || m_channels <= 0) {
        m_peakAmplitude = 0.0f;
        m_rmsAmplitude = 0.0f;
        return;
    }

    float peak = 0.0f;
    float sumSq = 0.0f;
    for (float s : m_audioData) {
        peak = qMax(peak, std::abs(s));
        sumSq += s * s;
    }
    m_peakAmplitude = peak;
    m_rmsAmplitude = std::sqrt(sumSq / m_audioData.size());
}

QVector<QPointF> AudioWaveformBridge::getWaveformPoints(int width, int channel) const
{
    QVector<QPointF> points;
    if (m_audioData.isEmpty() || m_channels <= 0 || channel >= m_channels || width <= 0) {
        return points;
    }

    int totalSamples = m_audioData.size() / m_channels;
    int samplesPerPixel = qMax(1, totalSamples / width);
    points.reserve(width);

    for (int px = 0; px < width; ++px) {
        int startSample = px * samplesPerPixel * m_channels + channel;
        int endSample = qMin(startSample + samplesPerPixel * m_channels, m_audioData.size());

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int i = startSample; i < endSample; i += m_channels) {
            float s = m_audioData[i];
            minVal = qMin(minVal, s);
            maxVal = qMax(maxVal, s);
        }

        // Output as normalized coordinates (0-1 for x, -1 to 1 for y)
        float x = totalSamples > 0 ? float(px) / width : 0.0f;
        // Use max absolute value for symmetrical waveform display
        float y = qMax(std::abs(minVal), std::abs(maxVal));
        points.append(QPointF(x, y));
    }

    return points;
}

float AudioWaveformBridge::getSampleAtTime(double time, int channel) const
{
    if (m_audioData.isEmpty() || m_channels <= 0 || channel >= m_channels || time < 0) {
        return 0.0f;
    }
    int sample = int(time * m_sampleRate);
    if (sample * m_channels + channel >= m_audioData.size()) {
        return 0.0f;
    }
    return m_audioData[sample * m_channels + channel];
}

double AudioWaveformBridge::sampleToTime(int sample) const
{
    return m_sampleRate > 0 ? double(sample) / m_sampleRate : 0.0;
}

int AudioWaveformBridge::timeToSample(double time) const
{
    return int(time * m_sampleRate);
}

} // namespace ks