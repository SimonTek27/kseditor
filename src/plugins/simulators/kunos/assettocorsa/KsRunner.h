#pragma once
#include <QObject>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFileInfo>

namespace ks {

enum class KsRunnerStatus {
    Ready,
    Running,
    Error,
    NotFound
};

class KsRunner : public QObject {
    Q_OBJECT

public:
    static KsRunner* instance();

    bool setKsPath(const QString& path);
    QString getKsPath() const { return m_ksPath; }
    
    bool testConnection();
    bool launchKs(const QString& track = QString(), const QString& car = QString());
    bool launchTimeTrial(const QString& track);
    bool launchPractice(const QString& track);
    bool stopKs();
    
    KsRunnerStatus status() const { return m_status; }
    QString lastError() const { return m_lastError; }
    QString lastOutput() const { return m_lastOutput; }
    bool isRunning() const;
    
    QString findCspExecutable() const;
    
    QString getContentPath() const;
    QString getTracksPath() const;
    QString getCarsPath() const;

signals:
    void launched();
    void launchFailed(const QString& error);
    void statusChanged(KsRunnerStatus status);
    void processExited(int exitCode);

private:
    explicit KsRunner(QObject* parent = nullptr);
    static KsRunner* s_instance;
    
    QString m_ksPath;
    KsRunnerStatus m_status = KsRunnerStatus::NotFound;
    QString m_lastError;
    QString m_lastOutput;
    QProcess* m_process = nullptr;
    
    bool validateKsPath();
    QString findKsExecutable() const;
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
};

}