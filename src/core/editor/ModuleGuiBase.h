#pragma once

#include "core/editor/EditorModule.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QGroupBox>

class ModuleGuiBase : public ks::EditorModule {
    Q_OBJECT
public:
    explicit ModuleGuiBase(QWidget* parent = nullptr);
    ~ModuleGuiBase() override = default;

    bool initialize() override;
    void shutdown() override;

    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    QString getModuleIcon() const override { return QString(); }
    int getModulePriority() const override { return 50; }

    void importFile(const QString& filePath) override { Q_UNUSED(filePath); }
    void exportFile(const QString& filePath) override { Q_UNUSED(filePath); }

    void newProject(const QString& name, const QString& path) override;
    void openProject(const QString& projectPath) override;
    void saveProject(const QString& path = QString()) override;
    void saveProjectAs(const QString& path) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    virtual void buildUI() = 0;
    virtual void onActivation() override;
    virtual void onDeactivation() override;

    void setupDockWidget(const QString& title, Qt::DockWidgetAreas areas = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QToolBar* createToolBar(const QString& name);
    QAction* addToolAction(QToolBar* toolbar, const QString& text, const QString& tooltip = QString(), const QKeySequence& shortcut = QKeySequence());
    QSplitter* createSplitter(Qt::Orientation orientation = Qt::Horizontal);
    QTreeWidget* createTreeWidget(const QStringList& headers);
    QListWidget* createListWidget();
    QComboBox* createComboBox(const QStringList& items = QStringList());
    QSpinBox* createSpinBox(int min = 0, int max = 100, int value = 0, const QString& suffix = QString());
    QDoubleSpinBox* createDoubleSpinBox(double min = 0, double max = 100, double value = 0, int decimals = 2, const QString& suffix = QString());
    QCheckBox* createCheckBox(const QString& text, bool checked = false);
    QPushButton* createButton(const QString& text, const QString& style = QString());
    QTextEdit* createLogOutput(int maxHeight = 120);
    QGroupBox* createGroupBox(const QString& title);
    QLabel* createLabel(const QString& text, const QString& style = QString());

    void log(const QString& msg, const QString& level = "info");
    void logError(const QString& msg) { log(msg, "error"); }
    void logWarning(const QString& msg) { log(msg, "warning"); }
    void logSuccess(const QString& msg) { log(msg, "success"); }

    QString selectFile(const QString& caption, const QString& filter, const QString& dir = QString());
    QString selectDirectory(const QString& caption, const QString& dir = QString());
    QStringList selectFiles(const QString& caption, const QString& filter, const QString& dir = QString());
    bool confirmAction(const QString& title, const QString& text);

    void saveSettings(const QString& key, const QVariant& value);
    QVariant loadSetting(const QString& key, const QVariant& defaultValue = QVariant());

    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QToolBar* m_mainToolbar = nullptr;
    QTextEdit* m_logOutput = nullptr;
    QDockWidget* m_dockWidget = nullptr;
    bool m_uiBuilt = false;
    QString m_currentProjectPath;
};