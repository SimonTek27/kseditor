#pragma once

#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>
#include "ModManager.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QCheckBox;
class QProgressBar;
class QTextEdit;

namespace ks {

class ConflictResolutionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConflictResolutionDialog(const ModConflictDetector::ConflictReport& report, QWidget* parent = nullptr);

    QStringList getModsToDisable() const { return m_modsToDisable; }
    QStringList getModsToEnable() const { return m_modsToEnable; }
    bool hasChanges() const { return !m_modsToDisable.isEmpty() || !m_modsToEnable.isEmpty(); }

private:
    void setupUI();
    void populateConflicts();
    void updateButtonStates();

    void onUseNewer();
    void onUseOlder();
    void onSkip();
    void onMerge();
    void onAutoResolveAll();
    void onSelectionChanged();
    void onItemClicked(QTreeWidgetItem* item, int column);
    void updateDetailView();
    void applyResolution(const QString& action);

    QVector<QString> m_modsToDisable;
    QVector<QString> m_modsToEnable;

    ModConflictDetector::ConflictReport m_report;
    ModConflictDetector::FileConflict m_selectedConflict;
    QMap<QString, QString> m_resolutions;
    bool m_autoResolve = true;

    QLabel* m_totalLabel = nullptr;
    QLabel* m_criticalLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QTreeWidget* m_conflictTree = nullptr;
    QTextEdit* m_detailText = nullptr;
    QPushButton* m_useNewerBtn = nullptr;
    QPushButton* m_useOlderBtn = nullptr;
    QPushButton* m_skipBtn = nullptr;
    QPushButton* m_mergeBtn = nullptr;
    QPushButton* m_autoResolveAllBtn = nullptr;
    QTreeWidgetItem* m_selectedItem = nullptr;
};

} // namespace ks
