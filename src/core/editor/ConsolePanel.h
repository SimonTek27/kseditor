#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include <QTextCursor>

namespace ks {

class ConsolePanel : public QObject
{
    Q_OBJECT

public:
    static ConsolePanel* instance();

    enum class MessageType {
        Info,
        Warning,
        Error,
        Command,
        Output,
        Debug
    };

    struct ConsoleMessage {
        QString id;
        QString text;
        MessageType type;
        QDateTime timestamp;
    };

    void setMaxLines(int max);
    int getMaxLines() const { return m_maxLines; }

    void clear();

    void print(const QString& text, MessageType type = MessageType::Info);
    void printInfo(const QString& text);
    void printWarning(const QString& text);
    void printError(const QString& text);
    void printCommand(const QString& text);
    void printOutput(const QString& text);
    void printDebug(const QString& text);

    QVector<ConsoleMessage> getMessages() const { return m_messages; }
    QVector<ConsoleMessage> getMessages(MessageType type) const;

    void setAutoScroll(bool autoScroll);
    bool isAutoScrollEnabled() const { return m_autoScroll; }

    void setTimestampVisible(bool visible);
    bool isTimestampVisible() const { return m_timestampVisible; }

    void setTypeFilter(const QMap<MessageType, bool>& filter);
    QMap<MessageType, bool> getTypeFilter() const { return m_typeFilter; }

    void setFontFamily(const QString& family);
    QString getFontFamily() const { return m_fontFamily; }

    void setFontSize(int size);
    int getFontSize() const { return m_fontSize; }

    void setWordWrap(bool enabled);
    bool isWordWrapEnabled() const { return m_wordWrap; }

    void saveToFile(const QString& path);
    void copyToClipboard();

    void find(const QString& text, bool caseSensitive = false, bool regex = false);
    void findNext();
    void findPrevious();

signals:
    void messageAdded(const ConsoleMessage& message);
    void messagePrinted(const ConsoleMessage& message);
    void messageCleared();
    void cleared();

private:
    ConsolePanel(QObject* parent = nullptr);
    ~ConsolePanel();
    Q_DISABLE_COPY(ConsolePanel)

    static ConsolePanel* s_instance;

    int m_maxLines = 1000;
    bool m_autoScroll = true;
    bool m_timestampVisible = false;
    QString m_fontFamily = "monospace";
    int m_fontSize = 11;
    bool m_wordWrap = true;

    QVector<ConsoleMessage> m_messages;
    QString m_nextId;

    QMap<MessageType, bool> m_typeFilter;
    QMap<MessageType, bool> m_filter;

    // Search state
    QString m_searchText;
    bool m_searchCaseSensitive = false;
    bool m_searchRegex = false;
    int m_searchCurrentIndex = 0;
};

class ConsoleInput : public QObject
{
    Q_OBJECT

public:
    static ConsoleInput* instance();

    void setConsole(ConsolePanel* console);

    void setHistoryEnabled(bool enabled);
    bool isHistoryEnabled() const { return m_historyEnabled; }

    void setMaxHistory(int max);
    int getMaxHistory() const { return m_maxHistory; }

    void setMultilineEnabled(bool enabled);
    bool isMultilineEnabled() const { return m_multiline; }

    void execute(const QString& command);

    QStringList getHistory() const { return m_history; }
    void clearHistory();

    void setAutoCompleteEnabled(bool enabled);
    bool isAutoCompleteEnabled() const { return m_autoComplete; }

signals:
    void commandExecuted(const QString& command);
    void historyChanged();

private slots:
    void onHistoryUp();
    void onHistoryDown();

private:
    ConsoleInput(QObject* parent = nullptr);
    ~ConsoleInput();
    Q_DISABLE_COPY(ConsoleInput)

    static ConsoleInput* s_instance;

    ConsolePanel* m_console = nullptr;
    bool m_historyEnabled = true;
    int m_maxHistory = 50;
    bool m_multiline = false;
    bool m_autoComplete = true;

    QStringList m_history;
    int m_historyIndex = -1;
    QString m_currentInput;
};

class OutputPanel : public QObject
{
    Q_OBJECT

public:
    static OutputPanel* instance();

    struct OutputCategory {
        QString id;
        QString name;
        QString color;
    };

    void registerCategory(const QString& id, const QString& name, const QString& color);
    void unregisterCategory(const QString& categoryId);

    void print(const QString& text, const QString& categoryId = QString());
    void clear(const QString& categoryId = QString());

    void setMaxLinesPerCategory(int max);
    int getMaxLinesPerCategory() const { return m_maxLinesPerCategory; }

    QVector<OutputCategory> getCategories() const { return m_categories.values(); }

    void setCategoryExpanded(const QString& categoryId, bool expanded);
    bool isCategoryExpanded(const QString& categoryId) const;

    void setCategoryFiltered(const QString& categoryId, bool filtered);
    bool isCategoryFiltered(const QString& categoryId) const;

    void setCategoryAutoScroll(const QString& categoryId, bool autoScroll);
    bool isCategoryAutoScroll(const QString& categoryId) const;

signals:
    void outputPrinted(const QString& text, const QString& categoryId);
    void categoryChanged(const QString& categoryId);

private:
    OutputPanel(QObject* parent = nullptr);
    ~OutputPanel();
    Q_DISABLE_COPY(OutputPanel)

    static OutputPanel* s_instance;

    int m_maxLinesPerCategory = 100;

    QMap<QString, OutputCategory> m_categories;
    QMap<QString, QVector<QString>> m_outputs;
    QMap<QString, bool> m_expanded;
    QMap<QString, bool> m_filtered;
    QMap<QString, bool> m_autoScroll;
};

} // namespace ks