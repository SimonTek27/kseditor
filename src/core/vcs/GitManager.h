#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

struct GitFileStatus {
    QString path;
    QString stagedStatus;  // 'M', 'A', 'D', 'R', 'C', '?' for unstaged
    QString unstagedStatus;
    bool isStaged() const { return !stagedStatus.isEmpty() && stagedStatus != "?"; }
    bool isUnstaged() const { return !unstagedStatus.isEmpty() && unstagedStatus != " "; }
    bool isUntracked() const { return unstagedStatus == "?"; }
    bool isNew() const { return stagedStatus == "A" || unstagedStatus == "A"; }
    bool isDeleted() const { return stagedStatus == "D" || unstagedStatus == "D"; }
    bool isModified() const { return stagedStatus == "M" || unstagedStatus == "M"; }
};

struct GitCommit {
    QString hash;
    QString author;
    QString date;
    QString message;
};

class GitManager : public QObject {
    Q_OBJECT
public:
    explicit GitManager(QObject* parent = nullptr);

    void setRepoPath(const QString& path);
    QString repoPath() const { return m_repoPath; }
    bool isValidRepo() const;

    // Async operations — results via signals
    void fetchStatus();
    void fetchDiff(const QString& filePath);
    void fetchLog(int count = 50);
    void fetchBranches();
    void stageFile(const QString& filePath);
    void unstageFile(const QString& filePath);
    void commit(const QString& message);
    void fetchBlame(const QString& filePath);

signals:
    void statusReady(const QList<GitFileStatus>& files);
    void diffReady(const QString& filePath, const QString& diff);
    void logReady(const QList<GitCommit>& commits);
    void branchesReady(const QStringList& branches, const QString& current);
    void blameReady(const QString& filePath, const QList<QPair<int, QString>>& blameLines);
    void commitDone(bool success, const QString& output);
    void operationFailed(const QString& error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void runGit(const QStringList& args, const QString& tag = QString());
    QString findGitPath() const;

    QProcess* m_process;
    QString m_repoPath;
    QString m_currentTag;
    QString m_pendingFilePath;
    QString m_gitPath;
};

class GitRunner : public QObject {
    Q_OBJECT
public:
    static QString runGitSync(const QString& repoPath, const QStringList& args);
};
