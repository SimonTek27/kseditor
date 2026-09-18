#include "ScriptConsole.h"
#include "ConsolePanel.h"

#include <QJSEngine>
#include <QJSValue>
#include <QDebug>
#include <QUuid>
#include <QJsonArray>

namespace ks {

// ─── ScriptConsole ─────────────────────────────────────────────────────────────

ScriptConsole* ScriptConsole::s_instance = nullptr;

ScriptConsole* ScriptConsole::instance()
{
    if (!s_instance) s_instance = new ScriptConsole();
    return s_instance;
}

ScriptConsole::ScriptConsole(QObject* parent)
    : QObject(parent)
    , m_engine(new QJSEngine(this))
{
    m_engine->installExtensions(QJSEngine::ConsoleExtension);

    // Expose a print() function to scripts
    QJSValue printFn = m_engine->newQObject(this);
    m_engine->globalObject().setProperty("__console__", printFn);
    evaluate(R"(
        function print(msg) { __console__.scriptPrint(String(msg)); }
        function log(msg)   { __console__.scriptPrint(String(msg)); }
    )");
}

ScriptConsole::~ScriptConsole() { s_instance = nullptr; }

void ScriptConsole::setConsoleOutput(ConsolePanel* console) { m_console = console; }

QJsonValue ScriptConsole::evaluateScript(const QString& script)
{
    if (script.trimmed().isEmpty()) return QJsonValue();
    m_history.append(script);
    if (m_history.size() > 200) m_history.removeFirst();

    QJSValue result = m_engine->evaluate(script);
    m_lastError.clear();
    if (result.isError()) {
        m_lastError = result.property("message").toString();
        emit scriptError(m_lastError);
        return QJsonValue();
    }
    if (result.isUndefined()) return QJsonValue();
    if (result.isBool()) return QJsonValue(result.toBool());
    if (result.isNumber()) return QJsonValue(result.toNumber());
    if (result.isString()) return QJsonValue(result.toString());
    // For objects/arrays, attempt JSON conversion
    QString jsonStr = result.toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isObject()) return QJsonValue(doc.object());
    if (doc.isArray()) return QJsonValue(doc.array());
    return QJsonValue(jsonStr);
}

QJsonValue ScriptConsole::getGlobalObject(const QString& name) const
{
    return m_globals.value(name);
}

// Called from QML/JS via Q_INVOKABLE
void ScriptConsole::scriptPrint(const QString& msg)
{
    emit printOutput(msg);
    if (m_console) m_console->printOutput(msg);
}

void ScriptConsole::evaluate(const QString& script)
{
    if (script.trimmed().isEmpty()) return;

    m_history.append(script);
    if (m_history.size() > 200) m_history.removeFirst();

    QJSValue result = m_engine->evaluate(script);
    m_lastError.clear();

    if (result.isError()) {
        m_lastError = QString("[%1:%2] %3")
            .arg(result.property("fileName").toString())
            .arg(result.property("lineNumber").toInt())
            .arg(result.property("message").toString());
        emit scriptError(m_lastError);
        if (m_console) m_console->printError(m_lastError);
        qWarning() << "[ScriptConsole]" << m_lastError;
    } else {
        QString resultStr = result.isUndefined() ? QString() : result.toString();
        QJsonValue jv = result.isObject()
            ? QJsonDocument::fromJson(m_engine->toScriptValue(result).toString().toUtf8()).toVariant().toJsonValue()
            : QJsonValue(resultStr);
        emit scriptEvaluated(jv);
        if (!resultStr.isEmpty() && m_console) m_console->printOutput("→ " + resultStr);
    }
}

void ScriptConsole::clear()
{
    m_history.clear();
    if (m_console) m_console->clear();
}

void ScriptConsole::reset()
{
    m_globals.clear();
    m_functions.clear();
    // Re-create the engine to flush all globals
    delete m_engine;
    m_engine = new QJSEngine(this);
    m_engine->installExtensions(QJSEngine::ConsoleExtension);
    m_lastError.clear();
}

void ScriptConsole::setGlobalObject(const QString& name, const QJsonValue& value)
{
    m_globals.insert(name, value);
    QString json = QJsonDocument(QJsonDocument::fromVariant(value.toVariant()))
                       .toJson(QJsonDocument::Compact);
    evaluate(name + " = " + json + ";");
}

void ScriptConsole::registerFunction(const QString& name, const QJSValue& fn)
{
    m_functions.insert(name, fn);
    m_engine->globalObject().setProperty(name, fn);
}

void ScriptConsole::unregisterFunction(const QString& name)
{
    m_functions.remove(name);
    m_engine->globalObject().deleteProperty(name);
}

void ScriptConsole::setAutoCompleteEnabled(bool enabled) { m_autoComplete = enabled; }

QStringList ScriptConsole::autoComplete(const QString& prefix) const
{
    if (!m_autoComplete) return {};
    QStringList results;
    QJSValue globals = m_engine->globalObject();
    // QJSValue doesn't expose property enumeration directly; use known globals
    for (const auto& name : m_globals.keys())
        if (name.startsWith(prefix)) results << name;
    for (const auto& name : m_functions.keys())
        if (name.startsWith(prefix)) results << name;
    // Built-ins
    static const QStringList builtins = {
        "print","log","Math","JSON","parseInt","parseFloat","isNaN","isFinite",
        "String","Number","Boolean","Array","Object","Date","RegExp",
        "undefined","null","true","false"
    };
    for (const auto& b : builtins)
        if (b.startsWith(prefix)) results << b;
    results.sort();
    return results;
}

QStringList ScriptConsole::getHistory() const { return m_history; }

// ─── ScriptConsoleEditor ───────────────────────────────────────────────────────

static ScriptConsoleEditor* s_editorInstance = nullptr;

ScriptConsoleEditor* ScriptConsoleEditor::instance()
{
    if (!s_editorInstance) s_editorInstance = new ScriptConsoleEditor();
    return s_editorInstance;
}

ScriptConsoleEditor::ScriptConsoleEditor(QObject* parent)
    : QObject(parent)
{
    EditorState initialState;
    initialState.script = m_script;
    initialState.cursorPosition = 0;
    initialState.selectionStart = 0;
    initialState.selectionEnd = 0;
    initialState.scrollPosition = 0;
    m_undoHistory.append(initialState);
}
ScriptConsoleEditor::~ScriptConsoleEditor() { s_editorInstance = nullptr; }

void ScriptConsoleEditor::undo()
{
    if (m_undoIndex > 0) {
        if (m_undoIndex == m_undoHistory.size() - 1) {
            // Save current state for redo
            m_undoHistory.append(getState());
        }
        --m_undoIndex;
        setState(m_undoHistory[m_undoIndex]);
        m_canUndo = m_undoIndex > 0;
        m_canRedo = true;
    }
}

void ScriptConsoleEditor::redo()
{
    if (m_canRedo && m_undoIndex + 1 < m_undoHistory.size()) {
        ++m_undoIndex;
        setState(m_undoHistory[m_undoIndex]);
        m_canUndo = true;
        m_canRedo = m_undoIndex + 1 < m_undoHistory.size();
    }
}

void ScriptConsoleEditor::setFontFamily(const QString& family)
{
    m_fontFamily = family;
}

void ScriptConsoleEditor::setFontSize(int size)
{
    m_fontSize = qBound(6, size, 48);
}

void ScriptConsoleEditor::setTabWidth(int width)
{
    m_tabWidth = qMax(1, width);
}

void ScriptConsoleEditor::setSyntaxHighlighting(bool enabled)
{
    m_syntaxHighlighting = enabled;
}

// Track changes for undo
void ScriptConsoleEditor::pushUndoState()
{
    EditorState st = getState();
    if (m_undoIndex < m_undoHistory.size() - 1) {
        m_undoHistory = m_undoHistory.mid(0, m_undoIndex + 1);
    }
    m_undoHistory.append(st);
    if (m_undoHistory.size() > 100) m_undoHistory.removeFirst();
    m_undoIndex = m_undoHistory.size() - 1;
    m_canUndo = m_undoIndex > 0;
    m_canRedo = false;
}

void ScriptConsoleEditor::setScriptConsole(ScriptConsole* console) { m_console = console; }

void ScriptConsoleEditor::setScript(const QString& script)
{
    if (m_script != script) {
        pushUndoState();
        m_script = script;
        emit scriptChanged(script);
    }
}

void ScriptConsoleEditor::setReadOnly(bool ro)
{
    m_readOnly = ro;
    emit readOnlyChanged(ro);
}

void ScriptConsoleEditor::insertText(const QString& text)
{
    if (m_readOnly) return;
    pushUndoState();
    m_script.insert(m_cursorPos, text);
    m_cursorPos += text.length();
    emit scriptChanged(m_script);
}

void ScriptConsoleEditor::removeText(int start, int end)
{
    if (m_readOnly || start < 0 || end > m_script.length()) return;
    pushUndoState();
    m_script.remove(start, end - start);
    m_cursorPos = qBound(0, m_cursorPos, m_script.length());
    emit scriptChanged(m_script);
}

void ScriptConsoleEditor::setSelection(int start, int end)
{
    m_selStart = start;
    m_selEnd   = end;
    emit selectionChanged(start, end);
}

void ScriptConsoleEditor::getSelection(int& start, int& end) const
{
    start = m_selStart;
    end   = m_selEnd;
}

void ScriptConsoleEditor::goToPosition(int pos)
{
    m_cursorPos = qBound(0, pos, m_script.length());
    emit cursorPositionChanged(m_cursorPos);
}

void ScriptConsoleEditor::runScript()
{
    if (m_console && !m_script.trimmed().isEmpty())
        m_console->evaluate(m_script);
}

ScriptConsoleEditor::EditorState ScriptConsoleEditor::getState() const
{
    EditorState s;
    s.script         = m_script;
    s.cursorPosition = m_cursorPos;
    return s;
}

void ScriptConsoleEditor::setState(const EditorState& state)
{
    m_script    = state.script;
    m_cursorPos = state.cursorPosition;
    emit scriptChanged(m_script);
}

} // namespace ks
