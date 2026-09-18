#pragma once

#include "core/editor/EditorModule.h"
#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QStringListModel>

class QVBoxLayout;

namespace ks {

class FormatToolsEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit FormatToolsEditorModule(QWidget* parent = nullptr);
    ~FormatToolsEditorModule() override = default;

    QString moduleName() const override { return "Format Converter"; }
    QString moduleId() const override { return "formatToolsEditor"; }
    QString getModuleIcon() const override { return ":/icons/format.png"; }
    int getModulePriority() const override { return 25; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    bool initialize() override;
    void shutdown() override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onConvertClicked();
    void onBatchConvertClicked();
    void onSelectInputFiles();
    void onSelectOutputDir();
    void onAddFilesClicked();
    void onRemoveFilesClicked();
    void onClearFilesClicked();
    void onClearLog();
    void onToolChanged(int index);
    void onExtractKn5Clicked();
    void onSelectKn5File();
    void onValidateClicked();
    void onSelectValidateFile();
    void onDdsToPngClicked();
    void onPngToDdsClicked();
    void onSelectTextureFile();

private:
    void setupUI();
    void setupToolBar(QVBoxLayout* mainLayout);
    void setupSingleConvTab(QWidget* parent);
    void setupBatchConvTab(QWidget* parent);
    void setupKn5Tab(QWidget* parent);
    void setupValidateTab(QWidget* parent);
    void setupTextureTab(QWidget* parent);
    void log(const QString& msg);
    void logError(const QString& msg);
    QStringList collectFiles(const QStringList& extensions);

    // Batch conversion helpers
    bool convertKn5ToFbxBatch(const QString& inputPath, const QString& outputPath);
    bool convertFbxToKn5Batch(const QString& inputPath, const QString& outputPath);

    QDockWidget* m_dockWidget = nullptr;

    QTabWidget* m_tabWidget;
    QTextEdit* m_logOutput;
    QProgressBar* m_progressBar;

    // Single conversion
    QPushButton* m_selectInputBtn;
    QLabel* m_inputFilePath;
    QPushButton* m_selectOutputBtn;
    QLabel* m_outputFilePath;
    QComboBox* m_formatCombo;
    QPushButton* m_convertBtn;

    // Batch conversion
    QListWidget* m_fileList;
    QPushButton* m_addFilesBtn;
    QPushButton* m_removeFilesBtn;
    QPushButton* m_clearFilesBtn;
    QPushButton* m_selectOutputDirBtn;
    QLabel* m_batchOutputDir;
    QComboBox* m_batchFormatCombo;
    QPushButton* m_batchConvertBtn;

    // KN5 extraction
    QPushButton* m_selectKn5Btn;
    QLabel* m_kn5FilePath;
    QPushButton* m_selectKn5OutputBtn;
    QLabel* m_kn5OutputPath;
    QCheckBox* m_extractTextures;
    QCheckBox* m_extractModels;
    QPushButton* m_extractKn5Btn;

    // Validation
    QPushButton* m_selectValidateBtn;
    QLabel* m_validateFilePath;
    QComboBox* m_validateTypeCombo;
    QPushButton* m_validateBtn;

    // Texture conversion
    QPushButton* m_selectTextureBtn;
    QLabel* m_textureFilePath;
    QPushButton* m_selectTextureOutputBtn;
    QLabel* m_textureOutputPath;
    QPushButton* m_ddsToPngBtn;
    QPushButton* m_pngToDdsBtn;
};

} // namespace ks
