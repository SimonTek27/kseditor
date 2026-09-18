#pragma once
#include <QObject>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFileInfo>

namespace ks {

enum class KsAssettoCorsaRunnerStatus {
    Ready,
    Running,
    Error,
    NotFound
};

class KsAssettoCorsaRunner : public QObject {
    Q_OBJECT

public:
    static KsAssettoCorsaRunner* instance();

    bool setKsPath(const QString& path);
    QString getKsPath() const { return m_ksPath; }
    
    bool testConnection();
    bool launchKs(const QString& track = QString(), const QString& car = QString());
    bool launchTimeTrial(const QString& track);
    bool launchPractice(const QString& track);
    bool stopKs();
    
    KsAssettoCorsaRunnerStatus status() const { return m_status; }
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
    void statusChanged(KsAssettoCorsaRunnerStatus status);
    void processExited(int exitCode);

private:
    explicit KsAssettoCorsaRunner(QObject* parent = nullptr);
    static KsAssettoCorsaRunner* s_instance;
    
    QString m_ksPath;
    KsAssettoCorsaRunnerStatus m_status = KsAssettoCorsaRunnerStatus::NotFound;
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