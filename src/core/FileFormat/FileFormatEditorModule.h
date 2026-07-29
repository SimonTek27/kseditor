#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QProgressBar>

namespace ks {

class FileFormatEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit FileFormatEditorModule(QWidget* parent = nullptr);
    ~FileFormatEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "File Format Manager"; }
    QString moduleId() const override { return "fileFormat"; }
    int getModulePriority() const override { return 20; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onFormatSelected(QTreeWidgetItem* item, int column);
    void onConvertFormat();
    void onValidateFile();
    void onDetectFormat();
    void onBatchConvert();
    void onConversionOptionsChanged();

private:
    void setupConverterTab();
    void setupValidatorTab();
    void setupBatchTab();
    void setupFormatBrowserTab();
    void populateFormatTree();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_converterTab = nullptr;
    QComboBox* m_sourceFormatCombo = nullptr;
    QComboBox* m_targetFormatCombo = nullptr;
    QPushButton* m_convertBtn = nullptr;
    QPushButton* m_detectBtn = nullptr;
    QLineEdit* m_sourcePathEdit = nullptr;
    QLineEdit* m_targetPathEdit = nullptr;
    QCheckBox* m_preserveStructureCheck = nullptr;
    QCheckBox* m_generateLODCheck = nullptr;
    QCheckBox* m_validateOutputCheck = nullptr;
    QProgressBar* m_conversionProgress = nullptr;
    QLabel* m_conversionStatusLabel = nullptr;

    QWidget* m_validatorTab = nullptr;
    QPushButton* m_validateBtn = nullptr;
    QTextEdit* m_validationOutput = nullptr;
    QLabel* m_validationResultLabel = nullptr;
    QProgressBar* m_validationProgress = nullptr;

    QWidget* m_batchTab = nullptr;
    QPushButton* m_batchConvertBtn = nullptr;
    QTableWidget* m_batchTable = nullptr;
    QProgressBar* m_batchProgress = nullptr;
    QLabel* m_batchStatusLabel = nullptr;

    QWidget* m_formatBrowserTab = nullptr;
    QTreeWidget* m_formatTree = nullptr;
    QLabel* m_formatInfoLabel = nullptr;
};

} // namespace ks
