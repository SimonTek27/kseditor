#pragma once

#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QCheckBox;

namespace ks {

struct ModEntry;

struct ConflictPair {
    QString modA;
    QString modB;
    QString reason;
    bool resolved = false;
    QString chosenMod;
};

class ConflictResolutionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConflictResolutionDialog(const QVector<ModEntry>& allMods,
                                      const QStringList& conflictStrings,
                                      QWidget* parent = nullptr);

    QStringList getModsToDisable() const { return m_modsToDisable; }
    QStringList getModsToEnable() const { return m_modsToEnable; }
    bool hasChanges() const { return !m_modsToDisable.isEmpty() || !m_modsToEnable.isEmpty(); }

private:
    void setupUI();
    void parseConflicts(const QStringList& conflictStrings);
    void populateTree();
    void onKeepClicked(const ConflictPair& pair, const QString& keepMod);
    void onAutoResolve();
    void updateSummary();
    void applyChanges();

    QVector<ConflictPair> m_conflicts;
    const QVector<ModEntry>& m_mods;
    QStringList m_modsToDisable;
    QStringList m_modsToEnable;

    QTreeWidget* m_tree = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QLabel* m_headerLabel = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_autoResolveBtn = nullptr;
};

} // namespace ks
