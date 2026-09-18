#include "ConsolePanel.h"
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>

namespace ks {

ConsolePanel* ConsolePanel::s_instance = nullptr;

ConsolePanel* ConsolePanel::instance()
{
    if (!s_instance) s_instance = new ConsolePanel();
    return s_instance;
}

ConsolePanel::ConsolePanel(QObject* parent) : QObject(parent) {}
ConsolePanel::~ConsolePanel() { s_instance = nullptr; }

void ConsolePanel::setMaxLines(int max) { m_maxLines = qMax(10, max); }
void ConsolePanel::setAutoScroll(bool v) { m_autoScroll = v; }
void ConsolePanel::setTimestampVisible(bool v) { m_timestampVisible = v; }
void ConsolePanel::setFontFamily(const QString& f) { m_fontFamily = f; }
void ConsolePanel::setFontSize(int s) { m_fontSize = qBound(6, s, 48); }
void ConsolePanel::setWordWrap(bool enabled) { m_wordWrap = enabled; }

void ConsolePanel::saveToFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    for (const auto& msg : m_messages) {
        QString prefix;
        switch (msg.type) {
            case MessageType::Info:    prefix = "[INFO] ";    break;
            case MessageType::Warning: prefix = "[WARN] ";    break;
            case MessageType::Error:   prefix = "[ERROR] ";   break;
            case MessageType::Command: prefix = "[CMD] ";     break;
            case MessageType::Output:  prefix = "";           break;
            case MessageType::Debug:   prefix = "[DEBUG] ";   break;
        }
        if (m_timestampVisible)
            out << "[" << msg.timestamp.toString(Qt::ISODate) << "] ";
        out << prefix << msg.text << "\n";
    }
    file.close();
}

void ConsolePanel::copyToClipboard()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text;
    for (const auto& msg : m_messages) {
        if (!text.isEmpty()) text += "\n";
        text += msg.text;
    }
    clipboard->setText(text);
}

void ConsolePanel::find(const QString& text, bool caseSensitive, bool regex)
{
    m_searchText = text;
    m_searchCaseSensitive = caseSensitive;
    m_searchRegex = regex;
    m_searchCurrentIndex = 0;
    findNext();
}

void ConsolePanel::findNext()
{
    if (m_searchText.isEmpty()) return;
    Qt::CaseSensitivity cs = m_searchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (int i = m_searchCurrentIndex; i < m_messages.size(); ++i) {
        bool match = false;
        if (m_searchRegex) {
            match = m_messages[i].text.contains(QRegularExpression(m_searchText));
        } else {
            match = m_messages[i].text.contains(m_searchText, cs);
        }
        if (match) {
            m_searchCurrentIndex = i + 1;
            emit messageAdded(m_messages[i]);
            return;
        }
    }
    // Wrap around
    m_searchCurrentIndex = 0;
}

void ConsolePanel::findPrevious()
{
    if (m_searchText.isEmpty()) return;
    Qt::CaseSensitivity cs = m_searchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (int i = qMin(m_searchCurrentIndex - 1, m_messages.size() - 1); i >= 0; --i) {
        bool match = false;
        if (m_searchRegex) {
            match = m_messages[i].text.contains(QRegularExpression(m_searchText));
        } else {
            match = m_messages[i].text.contains(m_searchText, cs);
        }
        if (match) {
            m_searchCurrentIndex = i;
            emit messageAdded(m_messages[i]);
            return;
        }
    }
    m_searchCurrentIndex = m_messages.size() - 1;
}

void ConsolePanel::clear()
{
    m_messages.clear();
    emit cleared();
}

void ConsolePanel::print(const QString& text, MessageType type)
{
    ConsoleMessage msg;
    msg.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    msg.text      = text;
    msg.type      = type;
    msg.timestamp = QDateTime::currentDateTime();

    if (m_messages.size() >= m_maxLines)
        m_messages.removeFirst();

    m_messages.append(msg);
    emit messagePrinted(msg);
}

void ConsolePanel::printInfo(const QString& t)    { print(t, MessageType::Info); }
void ConsolePanel::printWarning(const QString& t) { print(t, MessageType::Warning); }
void ConsolePanel::printError(const QString& t)   { print(t, MessageType::Error); }
void ConsolePanel::printCommand(const QString& t) { print(t, MessageType::Command); }
void ConsolePanel::printOutput(const QString& t)  { print(t, MessageType::Output); }
void ConsolePanel::printDebug(const QString& t)   { print(t, MessageType::Debug); }

void ConsolePanel::setTypeFilter(const QMap<MessageType, bool>& filter)
{
    m_filter = filter;
}

QVector<ConsolePanel::ConsoleMessage> ConsolePanel::getMessages(MessageType type) const
{
    if (type == MessageType::Info && m_filter.isEmpty()) return m_messages;
    QVector<ConsoleMessage> result;
    for (const auto& m : m_messages)
        if (m_filter.value(m.type, true)) result << m;
    return result;
}

// ─── ConsoleInput ────────────────────────────────────────────────────────────

static ConsoleInput* g_consoleInputInstance = nullptr;

ConsoleInput* ConsoleInput::instance()
{
    if (!g_consoleInputInstance)
        g_consoleInputInstance = new ConsoleInput();
    return g_consoleInputInstance;
}

ConsoleInput::ConsoleInput(QObject* parent)
    : QObject(parent) {}

ConsoleInput::~ConsoleInput()
{
    g_consoleInputInstance = nullptr;
}

void ConsoleInput::setConsole(ConsolePanel* console)
{
    m_console = console;
}

void ConsoleInput::setHistoryEnabled(bool enabled)
{
    m_historyEnabled = enabled;
}

void ConsoleInput::setMaxHistory(int max)
{
    m_maxHistory = qMax(1, max);
}

void ConsoleInput::setMultilineEnabled(bool enabled)
{
    m_multiline = enabled;
}

void ConsoleInput::execute(const QString& command)
{
    if (command.isEmpty()) return;
    emit commandExecuted(command);
    if (m_historyEnabled) {
        m_history.prepend(command);
        if (m_history.size() > m_maxHistory)
            m_history.removeLast();
    }
    m_historyIndex = -1;
}

void ConsoleInput::clearHistory()
{
    m_history.clear();
    m_historyIndex = -1;
    emit historyChanged();
}

void ConsoleInput::setAutoCompleteEnabled(bool enabled)
{
    m_autoComplete = enabled;
}

void ConsoleInput::onHistoryUp()
{
    if (m_history.isEmpty()) return;
    if (m_historyIndex < m_history.size() - 1) {
        ++m_historyIndex;
    }
}

void ConsoleInput::onHistoryDown()
{
    if (m_historyIndex >= 0) {
        --m_historyIndex;
    }
}

// ─── OutputPanel ─────────────────────────────────────────────────────────────

static OutputPanel* g_outputPanelInstance = nullptr;

OutputPanel* OutputPanel::instance()
{
    if (!g_outputPanelInstance)
        g_outputPanelInstance = new OutputPanel();
    return g_outputPanelInstance;
}

OutputPanel::OutputPanel(QObject* parent)
    : QObject(parent) {}

OutputPanel::~OutputPanel()
{
    g_outputPanelInstance = nullptr;
}

void OutputPanel::registerCategory(const QString& id, const QString& name, const QString& color)
{
    OutputCategory cat;
    cat.id = id;
    cat.name = name;
    cat.color = color;
    m_categories.insert(id, cat);
}

void OutputPanel::unregisterCategory(const QString& categoryId)
{
    m_categories.remove(categoryId);
    m_outputs.remove(categoryId);
}

void OutputPanel::print(const QString& text, const QString& categoryId)
{
    QString cat = categoryId.isEmpty() ? "default" : categoryId;
    m_outputs[cat].append(text);
    while (m_outputs[cat].size() > m_maxLinesPerCategory)
        m_outputs[cat].removeFirst();
    emit outputPrinted(text, categoryId);
}

void OutputPanel::clear(const QString& categoryId)
{
    if (categoryId.isEmpty())
        m_outputs.clear();
    else
        m_outputs.remove(categoryId);
}

void OutputPanel::setMaxLinesPerCategory(int max)
{
    m_maxLinesPerCategory = qMax(10, max);
}

void OutputPanel::setCategoryExpanded(const QString& categoryId, bool expanded)
{
    m_expanded[categoryId] = expanded;
}

bool OutputPanel::isCategoryExpanded(const QString& categoryId) const
{
    return m_expanded.value(categoryId, true);
}

void OutputPanel::setCategoryFiltered(const QString& categoryId, bool filtered)
{
    m_filtered[categoryId] = filtered;
}

bool OutputPanel::isCategoryFiltered(const QString& categoryId) const
{
    return m_filtered.value(categoryId, false);
}

void OutputPanel::setCategoryAutoScroll(const QString& categoryId, bool autoScroll)
{
    m_autoScroll[categoryId] = autoScroll;
}

bool OutputPanel::isCategoryAutoScroll(const QString& categoryId) const
{
    return m_autoScroll.value(categoryId, true);
}

} // namespace ks
