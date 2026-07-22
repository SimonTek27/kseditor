#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QProcess>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QMap>

class QTreeWidget;
class QListWidget;
class QTextEdit;
class QProgressBar;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QCheckBox;
class QSpinBox;

namespace ks {

class ArchiveEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit ArchiveEditorModule(QWidget* parent = nullptr);
    ~ArchiveEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Archive Manager"; }
    QString moduleId() const override { return "archive"; }
    int getModulePriority() const override { return 30; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private:
    void setupBrowseTab();
    void setupCreateTab();
    void setupTestTab();
    void setupHashTab();
    void setupBatchTab();
    void loadArchiveContents(const QString& path);
    void onArchiveSelected(const QString& path);
    void parse7zListOutput(const QString& output);
    void showContentContextMenu(const QPoint& pos);
    void onContentItemDoubleClicked(QTreeWidgetItem* item, int column);
    void extractSelected();
    void extractAll();
    void extractFlat();
    void extractItems(const QList<QTreeWidgetItem*>& items, const QString& outputDir);
    void run7zCommand(const QStringList& args, const QString& operation);
    void onFormatChanged(int index);
    void addFileToList(const QString& file);
    void addDirectoryToList(const QString& dir);
    void updateFileListCount();
    void showFileListContextMenu(const QPoint& pos);
    void createArchive();
    void testArchive();
    void testListOnly();
    void calculateHash();
    void verifyHash();
    void processBatch();
    QString formatSize(qint64 bytes) const;

    QTabWidget* m_tabWidget = nullptr;

    // Browse / Extract tab
    QWidget* m_browseTab = nullptr;
    QLineEdit* m_archivePathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QTreeWidget* m_contentTree = nullptr;
    QLineEdit* m_extractPathEdit = nullptr;
    QPushButton* m_extractBrowseBtn = nullptr;
    QPushButton* m_extractAllBtn = nullptr;

    // Create tab
    QWidget* m_createTab = nullptr;
    QLineEdit* m_createPathEdit = nullptr;
    QPushButton* m_createBrowseBtn = nullptr;
    QPushButton* m_addFilesBtn = nullptr;
    QPushButton* m_removeFilesBtn = nullptr;
    QPushButton* m_clearFilesBtn = nullptr;
    QPushButton* m_createArchiveBtn = nullptr;
    QComboBox* m_formatCombo = nullptr;
    QComboBox* m_compressionCombo = nullptr;
    QSpinBox* m_compressionLevelSpin = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QCheckBox* m_encryptNamesCheck = nullptr;
    QComboBox* m_splitCombo = nullptr;
    QListWidget* m_fileList = nullptr;
    QLabel* m_fileCountLabel = nullptr;

    // Test tab
    QWidget* m_testTab = nullptr;
    QLineEdit* m_testPathEdit = nullptr;
    QPushButton* m_testBrowseBtn = nullptr;
    QPushButton* m_testBtn = nullptr;
    QTextEdit* m_testLog = nullptr;

    // Hash tab
    QWidget* m_hashTab = nullptr;
    QLineEdit* m_hashPathEdit = nullptr;
    QPushButton* m_hashBrowseBtn = nullptr;
    QComboBox* m_hashAlgoCombo = nullptr;
    QPushButton* m_hashBtn = nullptr;
    QTextEdit* m_hashResult = nullptr;

    // Batch tab
    QWidget* m_batchTab = nullptr;
    QListWidget* m_batchFilesList = nullptr;
    QPushButton* m_batchAddBtn = nullptr;
    QPushButton* m_batchRemoveBtn = nullptr;
    QComboBox* m_batchFormatCombo = nullptr;
    QLineEdit* m_batchOutputEdit = nullptr;
    QPushButton* m_batchOutputBrowseBtn = nullptr;
    QPushButton* m_batchProcessBtn = nullptr;

    QString m_currentArchive;
    QStringList m_pendingFiles;
};

} // namespace ks
