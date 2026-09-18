#include "LuaScriptEditorModule.h"
#include "../../sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QShortcut>
#include <QApplication>
#include <QRegularExpression>
#include <QJsonArray>

namespace ks {

LuaScriptEditorModule::LuaScriptEditorModule(QWidget* parent) : EditorModule(parent) {}

bool LuaScriptEditorModule::initialize() { LOG_INFO("LuaScriptEditorModule", "Initialized"); return true; }

void LuaScriptEditorModule::shutdown()
{
    if (m_luaProcess) {
        m_luaProcess->kill();
        m_luaProcess->waitForFinished(3000);
        m_luaProcess->deleteLater();
        m_luaProcess = nullptr;
    }
    qDeleteAll(m_tabs);
    m_tabs.clear();
}

QDockWidget* LuaScriptEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget("Lua Script Editor", mainWindow);
    m_dockWidget->setObjectName("LuaScriptEditorDock");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(2, 2, 2, 2);

    auto* topLayout = new QHBoxLayout();
    m_fileTypeCombo = new QComboBox();
    m_fileTypeCombo->addItems({"CSP App", "CSP Lua", "WeatherFX", "Server Script", "General Lua", "INI Config", "JSON Config"});
    topLayout->addWidget(new QLabel("Type:"));
    topLayout->addWidget(m_fileTypeCombo);

    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText("No file loaded");
    m_filePathEdit->setReadOnly(true);
    topLayout->addWidget(m_filePathEdit, 1);
    mainLayout->addLayout(topLayout);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    mainLayout->addWidget(m_tabWidget);

    auto* toolLayout = new QHBoxLayout();
    m_newBtn = new QPushButton("New");
    m_loadBtn = new QPushButton("Open");
    m_saveBtn = new QPushButton("Save");
    m_saveAsBtn = new QPushButton("Save As");
    m_findBtn = new QPushButton("Find");
    m_executeBtn = new QPushButton("Execute");
    m_executeBtn->setStyleSheet("QPushButton { background-color: #2d7d46; color: white; font-weight: bold; }");
    toolLayout->addWidget(m_newBtn);
    toolLayout->addWidget(m_loadBtn);
    toolLayout->addWidget(m_saveBtn);
    toolLayout->addWidget(m_saveAsBtn);
    toolLayout->addWidget(m_findBtn);
    toolLayout->addWidget(m_executeBtn);
    toolLayout->addStretch();
    m_lineCountLabel = new QLabel("Lines: 0");
    toolLayout->addWidget(m_lineCountLabel);
    mainLayout->addLayout(toolLayout);

    m_statusLabel = new QLabel("Ready");
    mainLayout->addWidget(m_statusLabel);

    connect(m_newBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onNewFile);
    connect(m_loadBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onLoadFile);
    connect(m_saveBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onSaveFile);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onSaveAsFile);
    connect(m_findBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onFindReplace);
    connect(m_executeBtn, &QPushButton::clicked, this, &LuaScriptEditorModule::onExecuteScript);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &LuaScriptEditorModule::onCloseTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &LuaScriptEditorModule::onTabChanged);

    auto* findShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(findShortcut, &QShortcut::activated, this, &LuaScriptEditorModule::onFindReplace);

    auto* saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveShortcut, &QShortcut::activated, this, &LuaScriptEditorModule::onSaveFile);

    m_luaCompletions = {
        "function", "end", "if", "then", "else", "elseif", "for", "while", "do",
        "repeat", "until", "return", "local", "nil", "true", "false", "and", "or",
        "not", "in", "break", "require", "print", "math.", "string.", "table.",
        "scene_find_by_name", "scene_find_by_type", "scene_object_count",
        "scene_create_object", "scene_delete_object", "scene_set_translation",
        "scene_get_translation", "scene_set_material", "scene_update_transforms"
    };

    onNewFile();

    m_dockWidget->setWidget(centralWidget);
    return m_dockWidget;
}

LuaScriptTab* LuaScriptEditorModule::createTab(const QString& title, const QString& filePath)
{
    auto* editor = new CodeEditor();
    editor->setFont(QFont("Consolas", 10));
    editor->setTabStopDistance(20);
    editor->addCompletions(m_luaCompletions);
    auto* highlighter = new SyntaxHighlighter(editor->document());
    highlighter->setLanguage(SyntaxHighlighter::Lua);

    connect(editor, &CodeEditor::textChanged, this, &LuaScriptEditorModule::onTextChanged);

    auto* tab = new LuaScriptTab();
    tab->editor = editor;
    tab->highlighter = highlighter;
    tab->filePath = filePath;

    int idx = m_tabWidget->addTab(editor, title);
    tab->tabIndex = idx;
    m_tabs.append(tab);
    m_tabWidget->setCurrentIndex(idx);
    return tab;
}

LuaScriptTab* LuaScriptEditorModule::currentTab() const
{
    int idx = m_tabWidget->currentIndex();
    if (idx < 0) return nullptr;
    for (auto* t : m_tabs)
        if (t->tabIndex == idx) return t;
    return nullptr;
}

void LuaScriptEditorModule::updateTabTitle(LuaScriptTab* tab)
{
    if (!tab) return;
    QString title = tab->filePath.isEmpty() ? "untitled.lua" : QFileInfo(tab->filePath).fileName();
    if (tab->modified) title.prepend("* ");
    m_tabWidget->setTabText(tab->tabIndex, title);
}

void LuaScriptEditorModule::onNewFile()
{
    createTab("untitled.lua");
    updateLineCount();
    m_statusLabel->setText("New file created");
}

void LuaScriptEditorModule::onLoadFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Open Lua Script", QString(),
        "Lua Files (*.lua);;All Files (*)");
    for (const QString& p : files) {
        auto* tab = createTab(QFileInfo(p).fileName(), p);
        QFile file(p);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            tab->editor->setPlainText(file.readAll());
            m_statusLabel->setText("Loaded: " + p);
            tab->modified = false;
            updateTabTitle(tab);
        }
        // Detect file type from path
        QString lower = p.toLower();
        if (lower.contains("weather") || lower.contains("fx"))
            m_fileTypeCombo->setCurrentText("WeatherFX");
        else if (lower.contains("server"))
            m_fileTypeCombo->setCurrentText("Server Script");
        else if (lower.contains("apps") || lower.contains("lua"))
            m_fileTypeCombo->setCurrentText("CSP App");
    }
    updateLineCount();
}

void LuaScriptEditorModule::onSaveFile()
{
    auto* tab = currentTab();
    if (!tab) return;
    if (tab->filePath.isEmpty()) { onSaveAsFile(); return; }
    QFile file(tab->filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(tab->editor->toPlainText().toUtf8());
        file.close();
        tab->modified = false;
        updateTabTitle(tab);
        m_statusLabel->setText("Saved: " + tab->filePath);
    }
}

void LuaScriptEditorModule::onSaveAsFile()
{
    auto* tab = currentTab();
    if (!tab) return;
    QString p = QFileDialog::getSaveFileName(this, "Save Lua Script", QString(), "Lua Files (*.lua);;All Files (*)");
    if (!p.isEmpty()) {
        tab->filePath = p;
        updateTabTitle(tab);
        onSaveFile();
    }
}

void LuaScriptEditorModule::onCloseTab(int index)
{
    LuaScriptTab* tab = nullptr;
    for (auto* t : m_tabs) {
        if (t->tabIndex == index) { tab = t; break; }
    }
    if (!tab) return;
    if (tab->modified) {
        auto ret = QMessageBox::warning(this, "Unsaved Changes",
            QString("'%1' has unsaved changes. Save before closing?")
                .arg(tab->filePath.isEmpty() ? "untitled.lua" : tab->filePath),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            int oldIdx = m_tabWidget->currentIndex();
            m_tabWidget->setCurrentIndex(index);
            onSaveFile();
            if (tab->modified) return;
            m_tabWidget->setCurrentIndex(oldIdx);
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    m_tabWidget->removeTab(index);
    m_tabs.removeOne(tab);
    delete tab;
    m_statusLabel->setText("Tab closed");
    if (m_tabs.isEmpty()) onNewFile();
}

void LuaScriptEditorModule::onTabChanged(int)
{
    updateLineCount();
    auto* tab = currentTab();
    if (tab) {
        m_filePathEdit->setText(tab->filePath.isEmpty() ? "(new)" : tab->filePath);
    }
}

void LuaScriptEditorModule::onTextChanged()
{
    auto* tab = currentTab();
    if (!tab) return;
    tab->modified = true;
    updateTabTitle(tab);
    updateLineCount();
}

void LuaScriptEditorModule::updateLineCount()
{
    auto* tab = currentTab();
    int lines = tab ? tab->editor->document()->blockCount() : 0;
    m_lineCountLabel->setText(QString("Lines: %1").arg(lines));
}

void LuaScriptEditorModule::onFindReplace()
{
    auto* tab = currentTab();
    if (!tab) return;
    if (!m_findReplaceDialog) {
        m_findReplaceDialog = new FindReplaceDialog(m_dockWidget);
    }
    m_findReplaceDialog->setEditor(tab->editor);
    m_findReplaceDialog->show();
    m_findReplaceDialog->raise();
    m_findReplaceDialog->activateWindow();
}

void LuaScriptEditorModule::importFile(const QString& f)
{
    auto* tab = createTab(QFileInfo(f).fileName(), f);
    QFile file(f);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        tab->editor->setPlainText(file.readAll());
        m_statusLabel->setText("Loaded: " + f);
        tab->modified = false;
        updateTabTitle(tab);
    }
}

void LuaScriptEditorModule::exportFile(const QString& f)
{
    auto* tab = currentTab();
    if (!tab) return;
    tab->filePath = f;
    onSaveFile();
}

void LuaScriptEditorModule::onActivation()
{
    if (m_statusLabel) m_statusLabel->setText("Active");
}

void LuaScriptEditorModule::onDeactivation()
{
    if (m_statusLabel) m_statusLabel->setText("Inactive");
}

void LuaScriptEditorModule::onExecuteScript()
{
    auto* tab = currentTab();
    if (!tab) return;
    QString code = tab->editor->toPlainText();
    if (code.trimmed().isEmpty()) {
        m_statusLabel->setText("No script to execute.");
        return;
    }

    m_statusLabel->setText("Executing script...");

    if (m_luaProcess) {
        m_luaProcess->kill();
        m_luaProcess->deleteLater();
    }
    m_luaProcess = new QProcess(this);
    connect(m_luaProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &LuaScriptEditorModule::onLuaProcessFinished);

    m_luaProcess->setProcessChannelMode(QProcess::SeparateChannels);
    QStringList luaExes = {"luajit", "lua5.4", "lua5.3", "lua5.2", "lua5.1", "lua"};
    QString luaPath;
    for (const auto& exe : luaExes) {
        QProcess which;
        which.start("where", QStringList() << exe);
        which.waitForFinished(2000);
        if (which.exitCode() == 0) { luaPath = exe; break; }
    }

    if (luaPath.isEmpty()) {
        m_statusLabel->setText("Lua runtime not found in PATH. Install Lua or luajit.");
        return;
    }

    m_luaProcess->start(luaPath, QStringList() << "-e" << code);
    if (!m_luaProcess->waitForStarted(3000)) {
        m_statusLabel->setText("Failed to start Lua process.");
    }
}

void LuaScriptEditorModule::onLuaProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    QString output = QString::fromUtf8(m_luaProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(m_luaProcess->readAllStandardError());
    if (!output.trimmed().isEmpty())
        m_statusLabel->setText(QString("Output: %1").arg(output.left(120)));
    if (!error.trimmed().isEmpty()) {
        m_statusLabel->setText(QString("Error (exit %1): %2").arg(exitCode).arg(error.left(120)));
        // Try to extract error line number
        QRegularExpression re(":(\\d+):");
        auto match = re.match(error);
        if (match.hasMatch()) {
            int errLine = match.captured(1).toInt();
            auto* tab = currentTab();
            if (tab && errLine > 0 && errLine <= tab->editor->document()->blockCount()) {
                QTextCursor cursor(tab->editor->document()->findBlockByNumber(errLine - 1));
                tab->editor->setTextCursor(cursor);
                tab->editor->setFocus();
            }
        }
    } else if (exitCode == 0) {
        m_statusLabel->setText("Script executed successfully.");
    }
}

QJsonObject LuaScriptEditorModule::serializeProject() const
{
    QJsonObject data;
    QJsonArray filesArr;
    for (auto* tab : m_tabs) {
        QJsonObject tabObj;
        tabObj["filePath"] = tab->filePath;
        tabObj["content"] = tab->editor->toPlainText();
        tabObj["fileType"] = tab->fileType;
        filesArr.append(tabObj);
    }
    data["files"] = filesArr;
    data["fileType"] = m_fileTypeCombo->currentText();
    return data;
}

void LuaScriptEditorModule::deserializeProject(const QJsonObject& data)
{
    QJsonArray filesArr = data["files"].toArray();
    for (const auto& fileVal : filesArr) {
        QJsonObject tabObj = fileVal.toObject();
        QString fp = tabObj["filePath"].toString();
        auto* tab = createTab(fp.isEmpty() ? "untitled.lua" : QFileInfo(fp).fileName(), fp);
        tab->editor->setPlainText(tabObj["content"].toString());
        tab->fileType = tabObj["fileType"].toString();
        tab->modified = false;
        updateTabTitle(tab);
    }
    m_fileTypeCombo->setCurrentText(data["fileType"].toString("General Lua"));
}

void LuaScriptEditorModule::setupUi() { if (m_statusLabel) m_statusLabel->setText("UI Ready"); }

bool LuaScriptEditorModule::maybeSaveModified()
{
    for (auto* tab : m_tabs) {
        if (tab->modified) return false;
    }
    return true;
}

} // namespace ks