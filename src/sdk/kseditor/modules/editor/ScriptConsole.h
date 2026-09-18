#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QJSEngine>
#include <QJSValue>

namespace ks {

class ConsolePanel;

class ScriptConsole : public QObject
{
    Q_OBJECT

public:
    static ScriptConsole* instance();

    void setConsoleOutput(ConsolePanel* console);

    void evaluate(const QString& script);
    QJsonValue evaluateScript(const QString& script);

    void clear();
    void reset();

    void setGlobalObject(const QString& name, const QJsonValue& value);
    QJsonValue getGlobalObject(const QString& name) const;

    void registerFunction(const QString& name, const QJSValue& function);
    void unregisterFunction(const QString& name);

    void setAutoCompleteEnabled(bool enabled);
    bool isAutoCompleteEnabled() const { return m_autoComplete; }

    QString getLastError() const { return m_lastError; }

    Q_INVOKABLE void scriptPrint(const QString& msg);
    QStringList autoComplete(const QString& prefix) const;
    QStringList getHistory() const;

signals:
    void scriptEvaluated(const QJsonValue& result);
    void scriptError(const QString& error);
    void printOutput(const QString& output);

private:
    ScriptConsole(QObject* parent = nullptr);
    ~ScriptConsole();
    Q_DISABLE_COPY(ScriptConsole)

    static ScriptConsole* s_instance;

    ConsolePanel* m_console = nullptr;
    QJSEngine* m_engine = nullptr;
    QString m_lastError;
    bool m_autoComplete = true;
    QStringList m_history;
    QMap<QString, QJsonValue> m_globals;
    QMap<QString, QJSValue> m_functions;
};

class ScriptConsoleEditor : public QObject
{
    Q_OBJECT

public:
    static ScriptConsoleEditor* instance();

    void setScriptConsole(ScriptConsole* console);

    void setScript(const QString& script);
    QString getScript() const { return m_script; }

    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return m_readOnly; }

    void insertText(const QString& text);
    void removeText(int start, int end);

    void setSelection(int start, int end);
    void getSelection(int& start, int& end) const;

    void goToPosition(int position);

    void undo();
    void redo();

    bool canUndo() const { return m_canUndo; }
    bool canRedo() const { return m_canRedo; }

    void runScript();

    struct EditorState {
        QString script;
        int cursorPosition;
        int selectionStart;
        int selectionEnd;
        int scrollPosition;
    };

    EditorState getState() const;
    void setState(const EditorState& state);

    void setFontFamily(const QString& family);
    void setFontSize(int size);
    void setTabWidth(int width);

    void setSyntaxHighlighting(bool enabled);
    bool isSyntaxHighlightingEnabled() const { return m_syntaxHighlighting; }

signals:
    void scriptChanged(const QString& script);
    void cursorPositionChanged(int position);
    void selectionChanged(int start, int end);
    void readOnlyChanged(bool readOnly);

private:
    ScriptConsoleEditor(QObject* parent = nullptr);
    ~ScriptConsoleEditor();
    Q_DISABLE_COPY(ScriptConsoleEditor)

    static ScriptConsoleEditor* s_instance;

    ScriptConsole* m_console = nullptr;
    QString m_script;
    bool m_readOnly = false;
    bool m_canUndo = false;
    bool m_canRedo = false;
    bool m_syntaxHighlighting = true;
    int m_cursorPos = 0;
    int m_selStart = 0;
    int m_selEnd = 0;

    QString m_fontFamily = "monospace";
    int m_fontSize = 12;
    int m_tabWidth = 4;

    // Undo/redo
    QVector<EditorState> m_undoHistory;
    int m_undoIndex = 0;

    void pushUndoState();
};

} // namespace ks
