#include "AudioCore.h"
#include <QMutexLocker>
#include <QMediaDevices>
#include <QFile>
#include <QAudioFormat>

namespace ks {
namespace audio {

Studio::Studio(QObject* parent)
    : QObject(parent)
{
}

Studio::~Studio() {
    closeOutput();
    closeInput();
}

bool Studio::openOutput(const QAudioFormat& fmt) {
    QMutexLocker locker(&m_mutex);
    if (m_audioOut) {
        closeOutput();
    }
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    // No parent: we manage lifetime manually in closeOutput()
    m_audioOut = new QAudioSink(device, fmt);
    if (m_audioOut) {
        m_outputDevice = m_audioOut->start();
        return true;
    }
    return false;
}

void Studio::closeOutput() {
    QMutexLocker locker(&m_mutex);
    if (m_audioOut) {
        m_audioOut->stop();
        delete m_audioOut;
        m_audioOut = nullptr;
        m_outputDevice = nullptr;
    }
}

bool Studio::openInput(const QAudioFormat& fmt) {
    QMutexLocker locker(&m_mutex);
    if (m_audioIn) {
        closeInput();
    }
    QAudioDevice device = QMediaDevices::defaultAudioInput();
    // No parent: we manage lifetime manually in closeInput()
    m_audioIn = new QAudioSource(device, fmt);
    if (m_audioIn) {
        m_inputDevice = m_audioIn->start();
        return true;
    }
    return false;
}

void Studio::closeInput() {
    QMutexLocker locker(&m_mutex);
    if (m_audioIn) {
        m_audioIn->stop();
        delete m_audioIn;
        m_audioIn = nullptr;
        m_inputDevice = nullptr;
    }
}

bool Studio::previewEvent(const QString& eventPath, const QString& audioFilePath,
                          float volume, float pitch, bool loop)
{
    if (m_previewing) {
        stopPreview();
    }

    // Load the audio file for preview
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit previewError("Cannot open audio file for preview: " + audioFilePath);
        return false;
    }

    // Read WAV header and samples (simplified - assumes WAV format)
    // In production, use AudioFormatConverter
    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 44) {
        emit previewError("Audio file too small: " + audioFilePath);
        return false;
    }

    // Parse WAV header
    int channels = static_cast<int>(data[22]) | (static_cast<int>(data[23]) << 8);
    int sampleRate = static_cast<int>(data[24]) | (static_cast<int>(data[25]) << 8)
                   | (static_cast<int>(data[26]) << 16) | (static_cast<int>(data[27]) << 24);
    int bitsPerSample = static_cast<int>(data[34]) | (static_cast<int>(data[35]) << 8);
    int dataSize = static_cast<int>(data[40]) | (static_cast<int>(data[41]) << 8)
                 | (static_cast<int>(data[42]) << 16) | (static_cast<int>(data[43]) << 24);

    if (channels <= 0 || sampleRate <= 0 || bitsPerSample <= 0) {
        emit previewError("Invalid WAV header: " + audioFilePath);
        return false;
    }

    // Convert to float samples
    int bytesPerSample = bitsPerSample / 8;
    int sampleCount = dataSize / bytesPerSample;
    m_previewSamples.resize(sampleCount);

    const char* rawData = data.constData() + 44;
    for (int i = 0; i < sampleCount; ++i) {
        if (bitsPerSample == 16) {
            qint16 sample = static_cast<qint16>(static_cast<int>(static_cast<unsigned char>(rawData[i * 2]))
                           | (static_cast<int>(static_cast<unsigned char>(rawData[i * 2 + 1])) << 8));
            m_previewSamples[i] = static_cast<float>(sample) / 32768.0f;
        } else if (bitsPerSample == 32) {
            m_previewSamples[i] = *reinterpret_cast<const float*>(rawData + i * sizeof(float));
        } else {
            m_previewSamples[i] = 0.0f;
        }
    }

    m_previewSampleRate = sampleRate;
    m_previewChannels = channels;
    m_previewPosition = 0;
    m_previewVolume = volume;
    m_previewPitch = pitch;
    m_previewLoop = loop;
    m_previewEventPath = eventPath;

    // Ensure output device is open
    if (!m_audioOut) {
        QAudioFormat fmt;
        fmt.setSampleRate(sampleRate);
        fmt.setChannelCount(channels);
        fmt.setSampleFormat(QAudioFormat::Float);
        if (!openOutput(fmt)) {
            emit previewError("Cannot open audio output for preview");
            return false;
        }
    }

    m_previewing = true;
    emit previewStarted(eventPath);
    return true;
}

void Studio::stopPreview() {
    QMutexLocker locker(&m_mutex);
    m_previewing = false;
    m_previewPosition = 0;
    m_previewSamples.clear();
    m_previewEventPath.clear();
    emit previewStopped();
}

void Studio::setPreviewVolume(float volume) {
    QMutexLocker locker(&m_mutex);
    m_previewVolume = volume;
}

void Studio::setPreviewPitch(float pitch) {
    QMutexLocker locker(&m_mutex);
    m_previewPitch = pitch;
}

void Studio::processPreviewOutput() {
    if (!m_previewing || !m_outputDevice) return;

    QMutexLocker locker(&m_mutex);

    int framesAvailable = m_audioOut->bytesFree() / (m_previewChannels * static_cast<int>(sizeof(float)));
    int totalFrames = m_previewSamples.size() / m_previewChannels;
    int framesToWrite = qMin(framesAvailable, totalFrames - m_previewPosition);

    if (framesToWrite <= 0) {
        if (m_previewLoop) {
            m_previewPosition = 0;
            framesToWrite = qMin(m_audioOut->bytesFree() / (m_previewChannels * static_cast<int>(sizeof(float))),
                                 totalFrames);
        } else {
            m_previewing = false;
            emit previewStopped();
            return;
        }
    }

    QByteArray buffer(framesToWrite * m_previewChannels * sizeof(float), 0);
    float* out = reinterpret_cast<float*>(buffer.data());

    for (int f = 0; f < framesToWrite; ++f) {
        for (int c = 0; c < m_previewChannels; ++c) {
            int idx = (m_previewPosition + f) * m_previewChannels + c;
            float sample = (idx < m_previewSamples.size()) ? m_previewSamples[idx] : 0.0f;
            out[f * m_previewChannels + c] = sample * m_previewVolume;
        }
    }

    m_previewPosition += framesToWrite;
    m_outputDevice->write(buffer);
}

} // namespace audio
} // namespace ks