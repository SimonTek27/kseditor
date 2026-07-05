#pragma once

#include "../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QColorDialog>

namespace ks {

struct GUISkinColors {
    QColor windowBg = QColor(30, 30, 30);
    QColor panelBg = QColor(40, 40, 40);
    QColor textColor = QColor(200, 200, 200);
    QColor accentColor = QColor(0, 120, 215);
    QColor highlightColor = QColor(0, 120, 215);
    QColor borderColor = QColor(60, 60, 60);
    QColor errorColor = QColor(200, 50, 50);
    QColor successColor = QColor(50, 200, 50);
    QColor warningColor = QColor(200, 150, 50);
};

class GUISkinEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit GUISkinEditorModule(QWidget* parent = nullptr);
    ~GUISkinEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "GUI Skin Editor"; }
    QString moduleId() const override { return "guiSkinEditor"; }
    int getModulePriority() const override { return 43; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onWindowBgClicked();
    void onPanelBgClicked();
    void onTextColorClicked();
    void onAccentColorClicked();
    void onHighlightColorClicked();
    void onBorderColorClicked();
    void onErrorColorClicked();
    void onSuccessColorClicked();
    void onWarningColorClicked();
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();
    void updateButtonColor(QPushButton* btn, const QColor& c);

    QDockWidget* m_dockWidget = nullptr;
    QPushButton* m_windowBgBtn = nullptr;
    QPushButton* m_panelBgBtn = nullptr;
    QPushButton* m_textColorBtn = nullptr;
    QPushButton* m_accentColorBtn = nullptr;
    QPushButton* m_highlightColorBtn = nullptr;
    QPushButton* m_borderColorBtn = nullptr;
    QPushButton* m_errorColorBtn = nullptr;
    QPushButton* m_successColorBtn = nullptr;
    QPushButton* m_warningColorBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    GUISkinColors m_colors;
    QString m_filePath;
};

} // namespace ks
