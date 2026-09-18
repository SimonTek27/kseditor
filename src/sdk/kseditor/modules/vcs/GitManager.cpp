#include "GitManager.h"
#include <QRegularExpression>
#include <QDebug>

// ── GitRunner (sync helper) ──

QString GitRunner::runGitSync(const QString& repoPath, const QStringList& args)
{
    QProcess proc;
    proc.setWorkingDirectory(repoPath);
    proc.start("git", args);
    if (!proc.waitForFinished(30000))
        return QString();
    return QString::fromUtf8(proc.readAllStandardOutput());
}

// ── GitManager ──

GitManager::GitManager(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_gitPath("git")
{
}

void GitManager::setRepoPath(const QString& path)
{
    m_repoPath = QDir(path).absolutePath();
}

bool GitManager::isValidRepo() const
{
    if (m_repoPath.isEmpty()) return false;
    QString output = GitRunner::runGitSync(m_repoPath,
        {"rev-parse", "--git-dir"});
    return !output.trimmed().isEmpty();
}

void GitManager::runGit(const QStringList& args, const QString& tag)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }

    m_currentTag = tag;
    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitManager::onProcessFinished);

    m_process->setWorkingDirectory(m_repoPath);
    m_process->start("git", args);
}

void GitManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QString output;
    if (status == QProcess::NormalExit && exitCode == 0) {
        output = QString::fromUtf8(m_process->readAllStandardOutput());
    } else if (exitCode != 0) {
        QString err = QString::fromUtf8(m_process->readAllStandardError());
        emit operationFailed(err);
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }

    QString tag = m_currentTag;

    if (tag == "status") {
        QList<GitFileStatus> files;
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.length() < 3) continue;
            // "XY path"
            GitFileStatus s;
            s.stagedStatus = line[0];
            s.unstagedStatus = line[1];
            QString path = line.mid(3).trimmed();
            // Handle "R  old -> new"
            if (s.stagedStatus == "R" || s.unstagedStatus == "R") {
                path = path.section(" -> ", 1, 1).trimmed();
            }
            s.path = path;
            files.append(s);
        }
        emit statusReady(files);
    } else if (tag == "diff") {
        emit diffReady(m_pendingFilePath, output);
    } else if (tag == "log") {
        QList<GitCommit> commits;
        // Parse --format="%H|%an|%ai|%s"
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QStringList parts = line.split('|');
            if (parts.size() >= 4) {
                GitCommit c;
                c.hash = parts[0].left(8);
                c.author = parts[1];
                c.date = parts[2].left(10);
                c.message = parts[3];
                commits.append(c);
            }
        }
        emit logReady(commits);
    } else if (tag == "branches") {
        QStringList branches;
        QString current;
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.startsWith("* ")) {
                current = line.mid(2);
                branches << current;
            } else if (line.startsWith("  ")) {
                branches << line.trimmed();
            }
        }
        emit branchesReady(branches, current);
    } else if (tag == "blame") {
        QList<QPair<int, QString>> blameLines;
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        QRegularExpression re(R"(^(\w+)\s.+\((.+?)\s+\d{4})");
        for (int i = 0; i < lines.size(); ++i) {
            QRegularExpressionMatch m = re.match(lines[i]);
            if (m.hasMatch()) {
                blameLines.append({i + 1, m.captured(1).left(7) + " " + m.captured(2)});
            }
        }
        emit blameReady(m_pendingFilePath, blameLines);
    } else if (tag == "stage" || tag == "unstage" || tag == "commit") {
        emit commitDone(true, output);
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void GitManager::fetchStatus()
{
    runGit({"status", "--porcelain"}, "status");
}

void GitManager::fetchDiff(const QString& filePath)
{
    m_pendingFilePath = filePath;
    runGit({"diff", "--", filePath}, "diff");
}

void GitManager::fetchLog(int count)
{
    runGit({"log", "--oneline", QString("--max-count=%1").arg(count),
            QString("--format=%H|%an|%ai|%s")}, "log");
}

void GitManager::fetchBranches()
{
    runGit({"branch"}, "branches");
}

void GitManager::stageFile(const QString& filePath)
{
    m_pendingFilePath = filePath;
    runGit({"add", filePath}, "stage");
}

void GitManager::unstageFile(const QString& filePath)
{
    m_pendingFilePath = filePath;
    runGit({"reset", "HEAD", "--", filePath}, "unstage");
}

void GitManager::commit(const QString& message)
{
    runGit({"commit", "-m", message}, "commit");
}

void GitManager::fetchBlame(const QString& filePath)
{
    m_pendingFilePath = filePath;
    runGit({"blame", "--line-porcelain", filePath}, "blame");
}
