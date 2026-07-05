#ifndef WAVEFORM_ENGINE_H
#define WAVEFORM_ENGINE_H

#include <QObject>
#include <QVector>
#include <QAudioSink>
#include <QAudioFormat>
#include <QTimer>
#include <QIODevice>

class WaveformEngine : public QObject
{
    Q_OBJECT

public:
    static WaveformEngine* instance();

    explicit WaveformEngine(QObject *parent = nullptr);
    ~WaveformEngine();

    void setSamples(const QVector<float> &samples, int channels, int sampleRate);
    void clear();

    qint64 getPositionMs() const { return m_positionMs; }
    qint64 getDurationMs() const { return m_durationMs; }
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    bool isLooping() const { return m_loopEnabled; }
    qint64 getLoopStart() const { return m_loopStartMs; }
    qint64 getLoopEnd() const { return m_loopEndMs; }

public slots:
    void play();
    void stop();
    void pause();
    void setPosition(qint64 ms);
    void setLoopEnabled(bool enabled);
    void setLoopRegion(qint64 startMs, qint64 endMs);

signals:
    void positionChanged(qint64 ms);
    void playbackStarted();
    void playbackStopped();
    void playbackPaused();
    void playbackFinished();
    void loopToggled(bool enabled);

private slots:
    void updatePosition();

private:
    static WaveformEngine* s_instance;

    QAudioSink *m_audioOutput;
    QIODevice *m_outputDevice;
    QTimer *m_positionTimer;

    QVector<float> m_samples;
    int m_channels;
    int m_sampleRate;
    QAudioFormat m_format;

    qint64 m_positionMs;
    qint64 m_durationMs;
    bool m_isPlaying;
    bool m_isPaused;
    bool m_loopEnabled;
    qint64 m_loopStartMs;
    qint64 m_loopEndMs;

    void audioCallback();
    void seekToPosition(qint64 ms);
};

class WaveformIODevice : public QIODevice
{
    Q_OBJECT

public:
    explicit WaveformIODevice(QObject *parent = nullptr);
    ~WaveformIODevice();

    void setData(const QVector<float> &samples, int channels, int sampleRate);
    void clear();

    qint64 getDataSize() const { return m_dataSize; }
    void setReadPosition(qint64 ms);
    qint64 getReadPosition() const { return m_readPosition; }

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QVector<float> m_samples;
    int m_channels;
    int m_sampleRate;
    qint64 m_readPosition;
    qint64 m_dataSize;
    qint64 m_startPosition;
    qint64 m_endPosition;
    bool m_loopEnabled;

    friend class WaveformEngine;
};

#endif // WAVEFORM_ENGINE_H