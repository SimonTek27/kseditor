#pragma once

#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QStringList>
#include <QDir>

class TerminalOutput : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit TerminalOutput(QWidget* parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent* event) override;
};

class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

    void executeCommand(const QString& command);
    void setWorkingDirectory(const QString& path);
    QString workingDirectory() const { return m_workingDir; }

signals:
    void commandStarted();
    void commandFinished(int exitCode);
    void currentDirectoryChanged(const QString& path);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onInputSubmitted();

private:
    void setupUI();
    void startShell();

    QProcess* m_process;
    TerminalOutput* m_output;
    QLineEdit* m_input;
    QString m_workingDir;
    QStringList m_history;
    int m_historyIndex;
};
