#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFileInfo>

namespace ks {

enum class ACERunnerStatus {
    Ready,
    Running,
    Error,
    NotFound
};

class ACERunner : public QObject {
    Q_OBJECT

public:
    static ACERunner* instance();

    bool setAcePath(const QString& path);
    QString getAcePath() const { return m_acePath; }

    bool testConnection();
    bool launchAce(const QString& track = QString(), const QString& car = QString());
    bool launchTimeTrial(const QString& track);
    bool launchPractice(const QString& track);
    bool launchModded(const QString& modPath);
    bool stopAce();

    ACERunnerStatus status() const { return m_status; }
    QString lastError() const { return m_lastError; }
    QString lastOutput() const { return m_lastOutput; }
    bool isRunning() const;

    QString getContentPath() const;
    QString getTracksPath() const;
    QString getCarsPath() const;
    QString getModsPath() const;

signals:
    void launched();
    void launchFailed(const QString& error);
    void statusChanged(ACERunnerStatus status);
    void processExited(int exitCode);

private:
    explicit ACERunner(QObject* parent = nullptr);
    static ACERunner* s_instance;

    QString m_acePath;
    ACERunnerStatus m_status = ACERunnerStatus::NotFound;
    QString m_lastError;
    QString m_lastOutput;
    QProcess* m_process = nullptr;

    bool validateAcePath();
    QString findAceExecutable() const;
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
};

} // namespace ks
