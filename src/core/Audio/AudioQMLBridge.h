#ifndef AUDIO_QML_BRIDGE_H
#define AUDIO_QML_BRIDGE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariantList>
#include <QTimer>

class WaveProcessor;
class WaveformEngine;
class FFTProcessor;
class NoiseReducer;
class PeakMeter;

#include <QAudioFormat>
namespace ks { class AudioRecorder; }
namespace ks { namespace audio { class Studio; } }
namespace ks { namespace audio { class TextToSpeech; } }

class AudioQMLBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY playbackChanged)
    Q_PROPERTY(qint64 position READ getPositionMs WRITE setPositionMs NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ getDurationMs NOTIFY durationChanged)
    Q_PROPERTY(bool isLoopEnabled READ isLoopEnabled NOTIFY loopChanged)
    Q_PROPERTY(float leftPeak READ getLeftPeak NOTIFY levelsUpdated)
    Q_PROPERTY(float rightPeak READ getRightPeak NOTIFY levelsUpdated)
    Q_PROPERTY(float leftRMS READ getLeftRMS NOTIFY levelsUpdated)
    Q_PROPERTY(float rightRMS READ getRightRMS NOTIFY levelsUpdated)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(float inputLevel READ getInputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(QString recordingOutputPath READ recordingOutputPath NOTIFY recordingPathChanged)

    // Text-to-Speech properties
    Q_PROPERTY(bool ttsSpeaking READ ttsSpeaking NOTIFY ttsStateChanged)
    Q_PROPERTY(int ttsVolume READ ttsVolume WRITE setTtsVolume NOTIFY ttsVolumeChanged)
    Q_PROPERTY(int ttsRate READ ttsRate WRITE setTtsRate NOTIFY ttsRateChanged)
    Q_PROPERTY(QStringList ttsVoices READ ttsVoices NOTIFY ttsVoicesChanged)
    Q_PROPERTY(QString ttsCurrentVoice READ ttsCurrentVoice WRITE setTtsCurrentVoice NOTIFY ttsCurrentVoiceChanged)

public:
    static AudioQMLBridge* instance();

    explicit AudioQMLBridge(QObject *parent = nullptr);
    ~AudioQMLBridge();

    bool isPlaying() const;
    bool isPaused() const;
    qint64 getPositionMs() const;
    qint64 getDurationMs() const;
    bool isLoopEnabled() const;
    float getLeftPeak() const;
    float getRightPeak() const;
    float getLeftRMS() const;
    float getRightRMS() const;

    Q_INVOKABLE bool loadAudio(const QString &filePath);
    Q_INVOKABLE bool saveAudio(const QString &filePath);
    Q_INVOKABLE void newAudio(int channels, int sampleRate, int durationMs);

    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void setPositionMs(qint64 ms);
    Q_INVOKABLE void setLoopEnabled(bool enabled);
    Q_INVOKABLE void setLoopRegion(qint64 startMs, qint64 endMs);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool canUndo() const;
    Q_INVOKABLE bool canRedo() const;

    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectNone();
    Q_INVOKABLE void selectRegion(int startMs, int endMs);
    Q_INVOKABLE int getSelectionStart() const;
    Q_INVOKABLE int getSelectionEnd() const;

    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void deleteSelection();

    Q_INVOKABLE void reverse();
    Q_INVOKABLE void fadeIn(int startMs, int durationMs);
    Q_INVOKABLE void fadeOut(int endMs, int durationMs);
    Q_INVOKABLE void normalize(float level = 1.0f);
    Q_INVOKABLE void amplify(float factor);
    Q_INVOKABLE void invert();
    Q_INVOKABLE void silence(int startMs, int endMs);
    Q_INVOKABLE void insertSilence(int positionMs, int durationMs);
    Q_INVOKABLE void deleteRegion(int startMs, int endMs);

    Q_INVOKABLE void applyLowPassFilter(float cutoff, float resonance = 0.7f);
    Q_INVOKABLE void applyHighPassFilter(float cutoff, float resonance = 0.7f);
    Q_INVOKABLE void applyBandPassFilter(float low, float high);
    Q_INVOKABLE void applyNotchFilter(float freq, float bandwidth);

    Q_INVOKABLE void applyDelay(float delayMs, float feedback = 0.3f, float mix = 0.5f);
    Q_INVOKABLE void applyReverb(float roomSize = 0.5f, float damping = 0.5f, float wetDry = 0.3f);
    Q_INVOKABLE void applyEcho(float delayMs, float feedback = 0.5f, float mix = 0.5f);
    Q_INVOKABLE void applyChorus(float depth = 1.0f, float rate = 0.5f, float mix = 0.3f);
    Q_INVOKABLE void applyFlanger(float depth = 1.0f, float rate = 0.5f, float mix = 0.5f);
    Q_INVOKABLE void applyCompressor(float threshold = -20.0f, float ratio = 4.0f,
                                      float attack = 10.0f, float release = 100.0f, float makeupGain = 1.0f);
    Q_INVOKABLE void applyLimiter(float threshold = -0.1f, float release = 50.0f);

    Q_INVOKABLE void timeStretch(float ratio);
    Q_INVOKABLE void pitchShift(float semitones);
    Q_INVOKABLE void changeTempo(float percent);
    Q_INVOKABLE void changePitch(float semitones);

    Q_INVOKABLE bool convertFormat(const QString &inputPath, const QString &outputPath, int quality = 2);
    Q_INVOKABLE QStringList getSupportedFormats();

    // Recording
    Q_INVOKABLE bool startRecording(const QString &outputPath, int sampleRate = 44100, int channels = 2);
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void pauseRecording();
    Q_INVOKABLE void resumeRecording();
    Q_INVOKABLE bool isRecording() const;
    Q_INVOKABLE float getInputLevel(int channel = 0) const;
    Q_INVOKABLE QString recordingOutputPath() const;
    Q_INVOKABLE qint64 recordingDuration() const;
    Q_INVOKABLE QStringList getAvailableInputDevices();
    Q_INVOKABLE void setInputDevice(const QString& deviceName);
    Q_INVOKABLE QString getCurrentInputDevice() const;

    Q_INVOKABLE void captureNoiseProfile();
    Q_INVOKABLE void applyNoiseReduction(float amount = -15.0f);
    Q_INVOKABLE bool hasNoiseProfile() const;

    Q_INVOKABLE QVariantList getWaveformData(int width);
    Q_INVOKABLE QVariantList getSpectrumData();
    QVariantList getFrequencyBands(int bandCount);
    Q_INVOKABLE QVariantList getMelSpectrum(int bands = 64);

    Q_INVOKABLE int getSampleCount() const;
    Q_INVOKABLE int getChannelCount() const;
    Q_INVOKABLE int getSampleRate() const;
    Q_INVOKABLE int getBitDepth() const;
    Q_INVOKABLE QString getFileName() const;
    Q_INVOKABLE bool isModified() const;

    // Text-to-Speech
    Q_INVOKABLE void ttsSpeak(const QString& text);
    Q_INVOKABLE void ttsStop();
    Q_INVOKABLE void ttsPause();
    Q_INVOKABLE void ttsResume();
    Q_INVOKABLE bool ttsSpeaking() const;
    Q_INVOKABLE QStringList ttsVoices() const;
    Q_INVOKABLE QString ttsCurrentVoice() const;
    Q_INVOKABLE void setTtsCurrentVoice(const QString& name);
    Q_INVOKABLE int ttsVolume() const;
    Q_INVOKABLE void setTtsVolume(int percent);
    Q_INVOKABLE int ttsRate() const;
    Q_INVOKABLE void setTtsRate(int rate);
    Q_INVOKABLE bool ttsSaveToWav(const QString& text, const QString& filePath);

signals:
    void playbackChanged();
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void loopChanged(bool enabled);
    void levelsUpdated(float left, float right, float leftRMS, float rightRMS);
    void selectionChanged(int startMs, int endMs);
    void loadComplete();
    void saveComplete();
    void error(const QString &message);
    void recordingStateChanged(bool recording);
    void inputLevelChanged(float level);
    void recordingPathChanged(const QString& path);
    void recordingDurationChanged(qint64 duration);
    void audioChanged();
    void statusMessage(const QString& msg);

    // Text-to-Speech signals
    void ttsStateChanged();
    void ttsVolumeChanged();
    void ttsRateChanged();
    void ttsVoicesChanged();
    void ttsCurrentVoiceChanged();

private:
    static AudioQMLBridge* s_instance;

    WaveProcessor *m_waveProcessor;
    WaveformEngine *m_waveformEngine;
    FFTProcessor *m_fft;
    NoiseReducer *m_noiseReducer;
    PeakMeter *m_peakMeter;

    int m_selectionStartMs;
    int m_selectionEndMs;
    QString m_currentFilePath;
    bool m_modified;

    // Recording members
    ks::AudioRecorder* m_recorder;
    ks::audio::Studio* m_studio;
    QAudioFormat m_recordingFormat;
    QString m_recordingOutputPath;
    float m_currentInputLevel;
    QVector<float> m_inputLevels;
    QString m_inputDeviceName;
    QTimer* m_recordingTimer;
    qint64 m_recordingStartTime;

    // Text-to-Speech
    ks::audio::TextToSpeech* m_tts;
};

#endif // AUDIO_QML_BRIDGE_H