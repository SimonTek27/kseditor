#include "AudioQMLBridge.h"
#include "WaveProcessor.h"
#include "WaveformEngine.h"
#include "FFTProcessor.h"
#include "AudioFormatConverter.h"
#include "AudioTimeStretch.h"
#include "AudioRecording.h"
#include "AudioCore.h"
#include "TextToSpeech.h"
#include <QDebug>
#include <QFileInfo>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QTimer>
#include <QDateTime>

AudioQMLBridge* AudioQMLBridge::s_instance = nullptr;

AudioQMLBridge::AudioQMLBridge(QObject *parent)
    : QObject(parent)
    , m_waveProcessor(new WaveProcessor(this))
    , m_waveformEngine(new WaveformEngine(this))
    , m_fft(new FFTProcessor(this))
    , m_noiseReducer(new NoiseReducer(this))
    , m_peakMeter(new PeakMeter(this))
    , m_selectionStartMs(0)
    , m_selectionEndMs(0)
    , m_modified(false)
    , m_recorder(new ks::audio::AudioRecorder(this))
    , m_studio(new ks::audio::Studio(this))
    , m_currentInputLevel(0.0f)
    , m_recordingStartTime(0)
{
    s_instance = this;

    m_peakMeter->setSampleRate(44100);
    m_peakMeter->setChannelCount(2);
    m_peakMeter->setPeakDecay(1000.0f);

    m_recordingFormat.setSampleRate(44100);
    m_recordingFormat.setChannelCount(2);
    m_recordingFormat.setSampleFormat(QAudioFormat::Int16);

    m_recorder->setFormat(m_recordingFormat);

    // Text-to-Speech
    m_tts = new ks::audio::TextToSpeech(this);
    connect(m_tts, &ks::audio::TextToSpeech::started, this, &AudioQMLBridge::ttsStateChanged);
    connect(m_tts, &ks::audio::TextToSpeech::finished, this, &AudioQMLBridge::ttsStateChanged);

    m_recordingTimer = new QTimer(this);
    m_recordingTimer->setInterval(100);
    connect(m_recordingTimer, &QTimer::timeout, this, [this]() {
        if (m_studio && m_studio->inputDevice()) {
            QByteArray data = m_studio->inputDevice()->readAll();
            if (!data.isEmpty()) {
                int sampleCount = data.size() / 2;
                QVector<float> samples(sampleCount);
                const qint16* rawData = reinterpret_cast<const qint16*>(data.constData());
                float peak = 0.0f;
                for (int i = 0; i < sampleCount; ++i) {
                    samples[i] = static_cast<float>(rawData[i]) / 32767.0f;
                    float absVal = std::abs(samples[i]);
                    if (absVal > peak) peak = absVal;
                }
                m_currentInputLevel = peak;
                m_recorder->appendData(samples);
                emit inputLevelChanged(peak);
            }
        }
        emit recordingDurationChanged(QDateTime::currentMSecsSinceEpoch() - m_recordingStartTime);
    });

    connect(m_recorder, &ks::audio::AudioRecorder::stateChanged, this, [this](ks::audio::AudioRecorder::State state) {
        emit recordingStateChanged(state == ks::audio::AudioRecorder::Recording);
    });
    connect(m_recorder, &ks::audio::AudioRecorder::levelChanged, this, [this](float level) {
        m_currentInputLevel = level;
        emit inputLevelChanged(level);
    });
    connect(m_recorder, &ks::audio::AudioRecorder::recordingComplete, this, [this](const QString& path) {
        if (m_waveProcessor) {
            m_waveProcessor->load(path);
            emit loadComplete();
        }
    });

    connect(m_waveformEngine, &WaveformEngine::playbackStarted, this, &AudioQMLBridge::playbackChanged);
    connect(m_waveformEngine, &WaveformEngine::playbackStopped, this, &AudioQMLBridge::playbackChanged);
    connect(m_waveformEngine, &WaveformEngine::playbackPaused, this, &AudioQMLBridge::playbackChanged);
    connect(m_waveformEngine, &WaveformEngine::playbackFinished, this, &AudioQMLBridge::playbackChanged);
    connect(m_waveformEngine, &WaveformEngine::positionChanged, this, &AudioQMLBridge::positionChanged);
    connect(m_waveformEngine, &WaveformEngine::loopToggled, this, &AudioQMLBridge::loopChanged);

    connect(m_peakMeter, &PeakMeter::levelsChanged, [this](float left, float right) {
        emit levelsUpdated(left, right, m_peakMeter->getLeftRMS(), m_peakMeter->getRightRMS());
    });

    qDebug() << "AudioQMLBridge: Initialized";
}

AudioQMLBridge::~AudioQMLBridge()
{
    s_instance = nullptr;
}

AudioQMLBridge* AudioQMLBridge::instance()
{
    if (!s_instance) {
        s_instance = new AudioQMLBridge();
    }
    return s_instance;
}

bool AudioQMLBridge::isPlaying() const
{
    return m_waveformEngine->isPlaying();
}

bool AudioQMLBridge::isPaused() const
{
    return m_waveformEngine->isPaused();
}

qint64 AudioQMLBridge::getPositionMs() const
{
    return m_waveformEngine->getPositionMs();
}

bool AudioQMLBridge::isLoopEnabled() const
{
    return m_waveformEngine->isLooping();
}

float AudioQMLBridge::getLeftPeak() const
{
    return m_peakMeter->getPeakLevel(0);
}

float AudioQMLBridge::getRightPeak() const
{
    return m_peakMeter->getPeakLevel(1);
}

float AudioQMLBridge::getLeftRMS() const
{
    return m_peakMeter->getRMSLevel(0);
}

float AudioQMLBridge::getRightRMS() const
{
    return m_peakMeter->getRMSLevel(1);
}

bool AudioQMLBridge::loadAudio(const QString &filePath)
{
    if (m_waveProcessor->loadWav(filePath)) {
        m_currentFilePath = filePath;
        m_modified = false;
        m_selectionStartMs = 0;
        m_selectionEndMs = m_waveProcessor->getDurationMs();

        m_waveformEngine->setSamples(
            m_waveProcessor->getSamples(),
            m_waveProcessor->getChannelCount(),
            m_waveProcessor->getSampleRate()
        );

        emit loadComplete();
        emit durationChanged(m_waveProcessor->getDurationMs());
        return true;
    }
    emit error("Failed to load audio file");
    return false;
}

bool AudioQMLBridge::saveAudio(const QString &filePath)
{
    if (m_waveProcessor->saveWav(filePath)) {
        m_currentFilePath = filePath;
        m_modified = false;
        emit saveComplete();
        return true;
    }
    emit error("Failed to save audio file");
    return false;
}

void AudioQMLBridge::newAudio(int channels, int sampleRate, int durationMs)
{
    int numSamples = (sampleRate * durationMs) / 1000 * channels;
    QVector<float> samples(numSamples, 0.0f);
    m_waveProcessor->setSamples(samples, channels, sampleRate);
    m_currentFilePath = "";
    m_modified = true;
    m_selectionStartMs = 0;
    m_selectionEndMs = 0;
    emit audioChanged();
    emit statusMessage("Created new audio: " + QString::number(channels) + "ch, " + QString::number(sampleRate) + "Hz, " + QString::number(durationMs) + "ms");
}

void AudioQMLBridge::play()
{
    m_waveformEngine->setSamples(
        m_waveProcessor->getSamples(),
        m_waveProcessor->getChannelCount(),
        m_waveProcessor->getSampleRate()
    );
    m_waveformEngine->play();
}

void AudioQMLBridge::stop()
{
    m_waveformEngine->stop();
}

void AudioQMLBridge::pause()
{
    m_waveformEngine->pause();
}

void AudioQMLBridge::setPositionMs(qint64 ms)
{
    m_waveformEngine->setPosition(ms);
}

void AudioQMLBridge::setLoopEnabled(bool enabled)
{
    m_waveformEngine->setLoopEnabled(enabled);
}

void AudioQMLBridge::setLoopRegion(qint64 startMs, qint64 endMs)
{
    m_waveformEngine->setLoopRegion(startMs, endMs);
}

void AudioQMLBridge::undo()
{
    m_waveProcessor->undo();
    m_modified = true;
}

void AudioQMLBridge::redo()
{
    m_waveProcessor->redo();
    m_modified = true;
}

bool AudioQMLBridge::canUndo() const
{
    return m_waveProcessor && m_waveProcessor->hasUndo();
}

bool AudioQMLBridge::canRedo() const
{
    return m_waveProcessor && m_waveProcessor->hasRedo();
}

void AudioQMLBridge::selectAll()
{
    m_selectionStartMs = 0;
    m_selectionEndMs = m_waveProcessor->getDurationMs();
    emit selectionChanged(m_selectionStartMs, m_selectionEndMs);
}

void AudioQMLBridge::selectNone()
{
    m_selectionStartMs = 0;
    m_selectionEndMs = 0;
    emit selectionChanged(m_selectionStartMs, m_selectionEndMs);
}

void AudioQMLBridge::selectRegion(int startMs, int endMs)
{
    m_selectionStartMs = startMs;
    m_selectionEndMs = endMs;
    emit selectionChanged(m_selectionStartMs, m_selectionEndMs);
}

int AudioQMLBridge::getSelectionStart() const
{
    return m_selectionStartMs;
}

int AudioQMLBridge::getSelectionEnd() const
{
    return m_selectionEndMs;
}

void AudioQMLBridge::cut()
{
    if (m_selectionStartMs >= m_selectionEndMs) return;
    m_waveProcessor->copyRegion(m_selectionStartMs, m_selectionEndMs);
    m_waveProcessor->deleteRegion(m_selectionStartMs, m_selectionEndMs);
    m_modified = true;
}

void AudioQMLBridge::copy()
{
    if (m_selectionStartMs >= m_selectionEndMs) return;
    m_waveProcessor->copyRegion(m_selectionStartMs, m_selectionEndMs);
}

void AudioQMLBridge::paste()
{
    m_waveProcessor->pasteRegion(m_selectionStartMs);
    m_modified = true;
}

void AudioQMLBridge::deleteSelection()
{
    if (m_selectionStartMs >= m_selectionEndMs) return;
    m_waveProcessor->deleteRegion(m_selectionStartMs, m_selectionEndMs);
    m_modified = true;
}

void AudioQMLBridge::reverse()
{
    m_waveProcessor->reverse();
    m_modified = true;
}

void AudioQMLBridge::fadeIn(int startMs, int durationMs)
{
    m_waveProcessor->fadeIn(startMs, durationMs);
    m_modified = true;
}

void AudioQMLBridge::fadeOut(int endMs, int durationMs)
{
    m_waveProcessor->fadeOut(endMs, durationMs);
    m_modified = true;
}

void AudioQMLBridge::normalize(float level)
{
    m_waveProcessor->normalize(level);
    m_modified = true;
}

void AudioQMLBridge::amplify(float factor)
{
    m_waveProcessor->amplify(factor);
    m_modified = true;
}

void AudioQMLBridge::invert()
{
    m_waveProcessor->invert();
    m_modified = true;
}

void AudioQMLBridge::silence(int startMs, int endMs)
{
    m_waveProcessor->silence(startMs, endMs);
    m_modified = true;
}

void AudioQMLBridge::insertSilence(int positionMs, int durationMs)
{
    m_waveProcessor->insertSilence(positionMs, durationMs);
    m_modified = true;
}

void AudioQMLBridge::deleteRegion(int startMs, int endMs)
{
    m_waveProcessor->deleteRegion(startMs, endMs);
    m_modified = true;
}

void AudioQMLBridge::applyLowPassFilter(float cutoff, float resonance)
{
    m_waveProcessor->applyLowPassFilter(cutoff, resonance);
    m_modified = true;
    emit statusMessage("Low-pass filter applied (cutoff: " + QString::number(cutoff) + "Hz, resonance: " + QString::number(resonance) + ")");
}

void AudioQMLBridge::applyHighPassFilter(float cutoff, float resonance)
{
    m_waveProcessor->applyHighPassFilter(cutoff, resonance);
    m_modified = true;
    emit statusMessage("High-pass filter applied (cutoff: " + QString::number(cutoff) + "Hz, resonance: " + QString::number(resonance) + ")");
}

void AudioQMLBridge::applyBandPassFilter(float low, float high)
{
    m_waveProcessor->applyBandPassFilter(low, high);
    m_modified = true;
}

void AudioQMLBridge::applyNotchFilter(float freq, float bandwidth)
{
    m_waveProcessor->applyNotchFilter(freq, bandwidth);
    m_modified = true;
}

void AudioQMLBridge::applyDelay(float delayMs, float feedback, float mix)
{
    m_waveProcessor->applyDelay(delayMs, feedback, mix);
    m_modified = true;
}

void AudioQMLBridge::applyReverb(float roomSize, float damping, float wetDry)
{
    m_waveProcessor->applyReverb(roomSize, damping, wetDry);
    m_modified = true;
}

void AudioQMLBridge::applyEcho(float delayMs, float feedback, float mix)
{
    m_waveProcessor->applyEcho(delayMs, feedback, mix);
    m_modified = true;
}

void AudioQMLBridge::applyChorus(float depth, float rate, float mix)
{
    m_waveProcessor->applyChorus(depth, rate, mix);
    m_modified = true;
}

void AudioQMLBridge::applyFlanger(float depth, float rate, float mix)
{
    m_waveProcessor->applyFlanger(depth, rate, mix);
    m_modified = true;
}

void AudioQMLBridge::applyCompressor(float threshold, float ratio, float attack, float release, float makeupGain)
{
    m_waveProcessor->applyCompressor(threshold, ratio, attack, release, makeupGain);
    m_modified = true;
}

void AudioQMLBridge::applyLimiter(float threshold, float release)
{
    m_waveProcessor->applyLimiter(threshold, release);
    m_modified = true;
}

void AudioQMLBridge::captureNoiseProfile()
{
    QVector<float> region;
    if (m_selectionStartMs < m_selectionEndMs) {
        region = m_waveProcessor->getRegion(m_selectionStartMs, m_selectionEndMs);
    } else {
        region = m_waveProcessor->getSamples();
    }
    m_noiseReducer->captureNoiseProfile(region);
}

void AudioQMLBridge::applyNoiseReduction(float amount)
{
    if (!m_noiseReducer) return;

    QVector<float> samples = m_waveProcessor->getSamples();
    QVector<float> reduced = m_noiseReducer->reduceNoise(samples);
    m_waveProcessor->setSamples(reduced);
    m_modified = true;
}

bool AudioQMLBridge::hasNoiseProfile() const
{
    return m_noiseReducer && m_noiseReducer->hasProfile();
}

QVariantList AudioQMLBridge::getWaveformData(int width)
{
    QVariantList result;
    const QVector<float> &samples = m_waveProcessor->getSamples();
    if (samples.isEmpty()) return result;

    int channels = m_waveProcessor->getChannelCount();
    int samplesPerPixel = samples.size() / channels / width;
    if (samplesPerPixel < 1) samplesPerPixel = 1;

    for (int i = 0; i < width; ++i) {
        float minVal = 0.0f, maxVal = 0.0f;
        int startIdx = i * samplesPerPixel * channels;
        int endIdx = qMin(startIdx + samplesPerPixel * channels, samples.size());

        for (int j = startIdx; j < endIdx; j += channels) {
            float sample = samples[j];
            minVal = qMin(minVal, sample);
            maxVal = qMax(maxVal, sample);
        }

        result.append(minVal);
        result.append(maxVal);
    }

    return result;
}

QVariantList AudioQMLBridge::getSpectrumData()
{
    const QVector<float> &samples = m_waveProcessor->getSamples();
    QVector<float> spectrum = m_fft->computeLogMagnitudes(samples);

    QVariantList result;
    for (float val : spectrum) {
        result.append(val);
    }
    return result;
}

QVariantList AudioQMLBridge::getFrequencyBands(int bandCount)
{
    const QVector<float> &samples = m_waveProcessor->getSamples();
    QVector<float> bands = m_fft->getFrequencyBands(samples, bandCount);

    QVariantList result;
    for (float val : bands) {
        result.append(val);
    }
    return result;
}

QVariantList AudioQMLBridge::getMelSpectrum(int bands)
{
    const QVector<float> &samples = m_waveProcessor->getSamples();
    QVector<float> melSpec = m_fft->getMelSpectrum(samples, bands);

    QVariantList result;
    for (float val : melSpec) {
        result.append(val);
    }
    return result;
}

int AudioQMLBridge::getSampleCount() const
{
    return m_waveProcessor->getSampleCount();
}

int AudioQMLBridge::getChannelCount() const
{
    return m_waveProcessor->getChannelCount();
}

int AudioQMLBridge::getSampleRate() const
{
    return m_waveProcessor->getSampleRate();
}

int AudioQMLBridge::getBitDepth() const
{
    return m_waveProcessor->getBitDepth();
}

qint64 AudioQMLBridge::getDurationMs() const
{
    return m_waveProcessor->getDurationMs();
}

QString AudioQMLBridge::getFileName() const
{
    return QFileInfo(m_currentFilePath).fileName();
}

bool AudioQMLBridge::isModified() const
{
    return m_modified;
}

void AudioQMLBridge::timeStretch(float ratio)
{
    AudioTimeStretch stretch;
    QVector<float> samples = m_waveProcessor->getSamples();
    int channels = m_waveProcessor->getChannelCount();
    int sampleRate = m_waveProcessor->getSampleRate();

    QVector<float> stretched = stretch.stretch(samples, channels, sampleRate, ratio);
    m_waveProcessor->setSamples(stretched);
    m_modified = true;
}

void AudioQMLBridge::pitchShift(float semitones)
{
    float ratio = qPow(2.0f, semitones / 12.0f);
    AudioTimeStretch stretch;
    stretch.setPitchShift(semitones);

    QVector<float> samples = m_waveProcessor->getSamples();
    int channels = m_waveProcessor->getChannelCount();
    int sampleRate = m_waveProcessor->getSampleRate();

    QVector<float> shifted = stretch.stretch(samples, channels, sampleRate, ratio);
    m_waveProcessor->setSamples(shifted);
    m_modified = true;
}

void AudioQMLBridge::changeTempo(float percent)
{
    float ratio = 100.0f / percent;
    timeStretch(ratio);
}

void AudioQMLBridge::changePitch(float semitones)
{
    pitchShift(semitones);
}

bool AudioQMLBridge::convertFormat(const QString &inputPath, const QString &outputPath, int quality)
{
    AudioFormatConverter converter;
    AudioFormatConverter::ConversionQuality convQuality =
        static_cast<AudioFormatConverter::ConversionQuality>(quality);
    return converter.convert(inputPath, outputPath, convQuality);
}

QStringList AudioQMLBridge::getSupportedFormats()
{
    return AudioFormatConverter::supportedExtensions();
}

bool AudioQMLBridge::startRecording(const QString &outputPath, int sampleRate, int channels)
{
    if (isRecording()) {
        stopRecording();
    }

    m_recordingOutputPath = outputPath;
    m_recordingFormat.setSampleRate(sampleRate);
    m_recordingFormat.setChannelCount(channels);
    m_recordingFormat.setSampleFormat(QAudioFormat::Int16);

    m_recorder->setFormat(m_recordingFormat);
    m_recorder->setOutputPath(outputPath);

    if (!m_studio->openInput(m_recordingFormat)) {
        emit error("Failed to open audio input device");
        return false;
    }

    m_recorder->start();
    m_recordingStartTime = QDateTime::currentMSecsSinceEpoch();
    m_recordingTimer->start();

    emit recordingPathChanged(outputPath);
    return true;
}

void AudioQMLBridge::stopRecording()
{
    if (!isRecording()) return;

    m_recordingTimer->stop();
    m_recorder->stop();
    m_studio->closeInput();
}

void AudioQMLBridge::pauseRecording()
{
    if (!isRecording()) return;
    m_recordingTimer->stop();
    m_recorder->pause();
}

void AudioQMLBridge::resumeRecording()
{
    if (m_recorder->state() != ks::audio::AudioRecorder::Paused) return;
    m_recorder->resume();
    m_recordingTimer->start();
}

bool AudioQMLBridge::isRecording() const
{
    return m_recorder && m_recorder->state() == ks::audio::AudioRecorder::Recording;
}

float AudioQMLBridge::getInputLevel(int channel) const
{
    if (channel < 0 || channel >= m_inputLevels.size()) return 0.0f;
    return m_inputLevels[channel];
}

QString AudioQMLBridge::recordingOutputPath() const
{
    return m_recordingOutputPath;
}

qint64 AudioQMLBridge::recordingDuration() const
{
    if (!isRecording()) return m_recorder->recordedDuration();
    return QDateTime::currentMSecsSinceEpoch() - m_recordingStartTime;
}

QStringList AudioQMLBridge::getAvailableInputDevices()
{
    QStringList devices;
    QList<QAudioDevice> audioDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice& dev : audioDevices) {
        devices.append(dev.description());
    }
    if (devices.isEmpty()) {
        devices.append("Default");
    }
    return devices;
}

void AudioQMLBridge::setInputDevice(const QString& deviceName)
{
    m_inputDeviceName = deviceName;
}

QString AudioQMLBridge::getCurrentInputDevice() const
{
    return m_inputDeviceName.isEmpty() ? "Default" : m_inputDeviceName;
}

// ── Text-to-Speech ──────────────────────────────────────────────────────────

void AudioQMLBridge::ttsSpeak(const QString& text)
{
    if (m_tts) m_tts->speak(text);
}

void AudioQMLBridge::ttsStop()
{
    if (m_tts) m_tts->stop();
}

void AudioQMLBridge::ttsPause()
{
    if (m_tts) m_tts->pause();
}

void AudioQMLBridge::ttsResume()
{
    if (m_tts) m_tts->resume();
}

bool AudioQMLBridge::ttsSpeaking() const
{
    return m_tts ? m_tts->isSpeaking() : false;
}

QStringList AudioQMLBridge::ttsVoices() const
{
    return m_tts ? m_tts->availableVoices() : QStringList();
}

QString AudioQMLBridge::ttsCurrentVoice() const
{
    return m_tts ? m_tts->currentVoice() : QString();
}

void AudioQMLBridge::setTtsCurrentVoice(const QString& name)
{
    if (m_tts) {
        m_tts->setVoice(name);
        emit ttsCurrentVoiceChanged();
    }
}

int AudioQMLBridge::ttsVolume() const
{
    return m_tts ? m_tts->volume() : 0;
}

void AudioQMLBridge::setTtsVolume(int percent)
{
    if (m_tts) {
        m_tts->setVolume(percent);
        emit ttsVolumeChanged();
    }
}

int AudioQMLBridge::ttsRate() const
{
    return m_tts ? m_tts->rate() : 0;
}

void AudioQMLBridge::setTtsRate(int rate)
{
    if (m_tts) {
        m_tts->setRate(rate);
        emit ttsRateChanged();
    }
}

bool AudioQMLBridge::ttsSaveToWav(const QString& text, const QString& filePath)
{
    return m_tts ? m_tts->saveToWav(text, filePath) : false;
}