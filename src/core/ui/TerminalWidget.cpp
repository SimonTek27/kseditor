#include "TerminalWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QApplication>
#include <QStyle>
#include <QTextCursor>
#include <QTimer>
#include <QLabel>

// ── TerminalOutput ──

TerminalOutput::TerminalOutput(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setStyleSheet(
        "QPlainTextEdit { background: #1e1e1e; color: #d4d4d4; font-family: 'Consolas', 'Courier New', monospace; "
        "font-size: 12px; border: none; padding: 4px; }"
    );
    setMaximumBlockCount(10000);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    document()->setDefaultFont(QFont("Consolas", 10));
}

void TerminalOutput::keyPressEvent(QKeyEvent* event)
{
    // Forward Ctrl+C to allow interrupting, otherwise ignore input
    if (event == QKeySequence::Copy) {
        copy();
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

// ── TerminalWidget ──

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_historyIndex(-1)
{
    setupUI();
    startShell();
}

TerminalWidget::~TerminalWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void TerminalWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_output = new TerminalOutput(this);
    layout->addWidget(m_output, 1);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(4, 2, 4, 2);

    QLabel* promptLabel = new QLabel(">", this);
    promptLabel->setStyleSheet("QLabel { color: #6a9955; font-family: 'Consolas', monospace; font-size: 12px; padding: 0px 4px; }");
    inputLayout->addWidget(promptLabel);

    m_input = new QLineEdit(this);
    m_input->setStyleSheet(
        "QLineEdit { background: #2d2d2d; color: #d4d4d4; font-family: 'Consolas', 'Courier New', monospace; "
        "font-size: 12px; border: 1px solid #3c3c3c; padding: 4px 6px; }"
        "QLineEdit:focus { border-color: #264f78; }"
    );
    m_input->setPlaceholderText("Type a command...");
    inputLayout->addWidget(m_input, 1);

    layout->addLayout(inputLayout);

    connect(m_input, &QLineEdit::returnPressed, this, &TerminalWidget::onInputSubmitted);
}

void TerminalWidget::startShell()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    m_workingDir = QDir::currentPath();

    connect(m_process, &QProcess::started, this, &TerminalWidget::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyReadStderr);

#ifdef Q_OS_WIN
    m_process->start("cmd.exe", {"/q"});
#else
    m_process->start("bash", {"--noediting"});
#endif
}

void TerminalWidget::onReadyReadStdout()
{
    QString data = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(data);
    m_output->ensureCursorVisible();
}

void TerminalWidget::onReadyReadStderr()
{
    QString data = QString::fromLocal8Bit(m_process->readAllStandardError());
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(data);
    m_output->ensureCursorVisible();
}

void TerminalWidget::onProcessStarted()
{
    m_output->clear();
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    QString msg = QString("\n[Process exited with code %1]\n").arg(exitCode);
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(msg);
    m_output->ensureCursorVisible();

    // Restart after a short delay
    QTimer::singleShot(1000, this, &TerminalWidget::startShell);
}

void TerminalWidget::onInputSubmitted()
{
    QString cmd = m_input->text().trimmed();
    m_input->clear();

    if (cmd.isEmpty()) return;

    // Add to history
    if (m_history.isEmpty() || m_history.last() != cmd) {
        m_history.append(cmd);
    }
    m_historyIndex = m_history.size();

    // Handle built-in commands
    if (cmd.toLower() == "clear" || cmd.toLower() == "cls") {
        m_output->clear();
        return;
    }

    if (cmd.toLower().startsWith("cd ")) {
        QString newDir = cmd.mid(3).trimmed();
        newDir.remove('"');
        QDir dir(m_workingDir);
        if (dir.cd(newDir)) {
            m_workingDir = dir.absolutePath();
            emit currentDirectoryChanged(m_workingDir);
        } else {
            m_output->moveCursor(QTextCursor::End);
            m_output->insertPlainText("cd: " + newDir + ": No such directory\n");
            m_output->ensureCursorVisible();
        }
        return;
    }

    // Forward to shell process
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write((cmd + "\n").toLocal8Bit());
    }

    m_input->setFocus();
}

void TerminalWidget::executeCommand(const QString& command)
{
    m_input->setText(command);
    onInputSubmitted();
}

void TerminalWidget::setWorkingDirectory(const QString& path)
{
    m_workingDir = path;
    emit currentDirectoryChanged(m_workingDir);
}
