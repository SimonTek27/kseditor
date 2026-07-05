#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariantMap>
#include <QMap>

namespace ks {

class AudioEngineQML : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY initializedChanged)
    Q_PROPERTY(QStringList loadedBanks READ loadedBanks NOTIFY banksChanged)
    Q_PROPERTY(float rpm READ rpm WRITE setRpm NOTIFY rpmChanged)
    Q_PROPERTY(float throttle READ throttle WRITE setThrottle NOTIFY throttleChanged)
    Q_PROPERTY(float turboBoost READ turboBoost WRITE setTurboBoost NOTIFY turboChanged)
    Q_PROPERTY(int limiterRPM READ limiterRPM WRITE setLimiterRPM NOTIFY limiterChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackChanged)

public:
    static AudioEngineQML* instance();

    explicit AudioEngineQML(QObject* parent = nullptr);
    ~AudioEngineQML();

    bool isInitialized() const { return m_initialized; }
    QStringList loadedBanks() const { return m_loadedBanks; }

    float rpm() const { return m_rpm; }
    float throttle() const { return m_throttle; }
    float turboBoost() const { return m_turboBoost; }
    int limiterRPM() const { return m_limiterRPM; }
    bool isPlaying() const { return m_isPlaying; }

    Q_INVOKABLE bool initialize(const QString& simRootPath = QString());
    Q_INVOKABLE void shutdown();

    Q_INVOKABLE bool loadBank(const QString& bankPath);
    Q_INVOKABLE void unloadBank(const QString& bankName);
    Q_INVOKABLE void unloadAllBanks();

    Q_INVOKABLE QStringList getEvents(const QString& bankName) const;
    Q_INVOKABLE QVariantMap getEventInfo(const QString& eventPath) const;

    Q_INVOKABLE void playEvent(const QString& eventPath);
    Q_INVOKABLE void stopEvent(const QString& eventPath);
    Q_INVOKABLE void stopAllEvents();

    Q_INVOKABLE void setRpm(float value);
    Q_INVOKABLE void setThrottle(float value);
    Q_INVOKABLE void setTurboBoost(float value);
    Q_INVOKABLE void setLimiterRPM(int value);

    Q_INVOKABLE void set3DListenerPosition(float x, float y, float z, float forwardX, float forwardY, float forwardZ);

    Q_INVOKABLE QStringList getBuses() const;
    Q_INVOKABLE float getBusVolume(const QString& busPath) const;
    Q_INVOKABLE void setBusVolume(const QString& busPath, float volume);
    Q_INVOKABLE void setBusMute(const QString& busPath, bool mute);

    Q_INVOKABLE QStringList getVCAs() const;
    Q_INVOKABLE float getVCAVolume(const QString& vcaPath) const;
    Q_INVOKABLE void setVCAVolume(const QString& vcaPath, float volume);

    Q_INVOKABLE bool loadSoundbank(const QString& carDirectory, const QString& carId);
    Q_INVOKABLE void setCameraExternal(bool external);
    Q_INVOKABLE void setTurboValue(float boost);
    Q_INVOKABLE void setLimiterValue(float decay);
    Q_INVOKABLE void setDoorState(bool open);
    Q_INVOKABLE void setHornState(bool active);

    Q_INVOKABLE QString getLastError() const { return m_lastError; }

public slots:
    void update();

signals:
    void initializedChanged(bool initialized);
    void banksChanged();
    void bankLoaded(const QString& bankName);
    void bankUnloaded(const QString& bankName);
    void eventStarted(const QString& eventPath);
    void eventStopped(const QString& eventPath);
    void playbackChanged();
    void rpmChanged(float rpm);
    void throttleChanged(float throttle);
    void turboChanged(float boost);
    void limiterChanged(int rpm);
    void busVolumeChanged(const QString& busPath, float volume);
    void busMuteChanged(const QString& busPath, bool muted);
    void vcaVolumeChanged(const QString& vcaPath, float volume);
    void error(const QString& message);

private:
    bool m_initialized = false;
    QString m_lastError;
    QString m_simRootPath;

    QStringList m_loadedBanks;
    QMap<QString, QStringList> m_bankEvents;

    float m_rpm = 0.0f;
    float m_throttle = 0.0f;
    float m_turboBoost = 0.0f;
    int m_limiterRPM = 9000;
    bool m_isPlaying = false;

    QString m_currentEvent;
};

}