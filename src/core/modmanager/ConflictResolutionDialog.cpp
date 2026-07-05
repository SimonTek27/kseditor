#include "ConflictResolutionDialog.h"
#include "ModManager.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QColor>
#include <QSet>
#include <algorithm>

namespace ks {

ConflictResolutionDialog::ConflictResolutionDialog(
    const QVector<ModEntry>& allMods,
    const QStringList& conflictStrings,
    QWidget* parent)
    : QDialog(parent)
    , m_mods(allMods)
{
    setWindowTitle("Mod Conflict Resolution");
    setMinimumSize(680, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    parseConflicts(conflictStrings);
    setupUI();
}

void ConflictResolutionDialog::parseConflicts(const QStringList& conflictStrings) {
    for (const QString& entry : conflictStrings) {
        QStringList parts = entry.split(" conflicts with ");
        if (parts.size() == 2) {
            ConflictPair pair;
            pair.modA = parts[0].trimmed();
            pair.modB = parts[1].trimmed();
            pair.reason = "These mods modify the same game asset or provide overlapping functionality";
            m_conflicts.append(pair);
        }
    }

    if (m_conflicts.isEmpty()) {
        for (const QString& entry : conflictStrings) {
            ConflictPair pair;
            pair.modA = entry;
            pair.modB = "";
            pair.reason = "";
            m_conflicts.append(pair);
        }
    }
}

void ConflictResolutionDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* header = new QLabel("Mod Conflict Resolution");
    header->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #cc4400,stop:1 #993300);"
        "color: white; font-size: 16px; font-weight: bold;"
        "padding: 16px 20px;");
    header->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(header);

    auto* content = new QWidget();
    content->setStyleSheet("background: #252526;");
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 12, 16, 12);
    contentLayout->setSpacing(10);

    m_headerLabel = new QLabel(
        QString("The following %1 conflict(s) were detected between your enabled mods. "
                "Choose which mod to keep enabled for each conflict.")
            .arg(m_conflicts.size()));
    m_headerLabel->setWordWrap(true);
    m_headerLabel->setStyleSheet("color: #cccccc; font-size: 12px; padding: 4px 0;");
    contentLayout->addWidget(m_headerLabel);

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabels({"", "Mod", "Conflicts With", "Reason", "Action"});
    m_tree->setColumnWidth(0, 28);
    m_tree->setColumnWidth(1, 160);
    m_tree->setColumnWidth(2, 160);
    m_tree->setColumnWidth(3, 180);
    m_tree->setColumnWidth(4, 120);
    m_tree->setAlternatingRowColors(true);
    m_tree->setRootIsDecorated(false);
    m_tree->setSelectionMode(QAbstractItemView::NoSelection);
    m_tree->setStyleSheet(
        "QTreeWidget { background: #1e1e1e; border: 1px solid #3f3f46; "
        "alternate-background-color: #222228; }"
        "QTreeWidget::item { padding: 6px 2px; border-bottom: 1px solid #2a2a30; }"
        "QHeaderView::section { background: #2a2a30; color: #a1a1aa; "
        "border: none; border-bottom: 1px solid #3f3f46; padding: 6px; }");
    contentLayout->addWidget(m_tree, 1);

    m_summaryLabel = new QLabel();
    m_summaryLabel->setStyleSheet("color: #a1a1aa; font-size: 11px; padding: 2px 0;");
    contentLayout->addWidget(m_summaryLabel);

    mainLayout->addWidget(content, 1);

    auto* footer = new QWidget();
    footer->setStyleSheet("background: #1e1e1e;");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 10);
    footerLayout->setSpacing(8);

    m_autoResolveBtn = new QPushButton("Auto-Resolve");
    m_autoResolveBtn->setStyleSheet(
        "QPushButton { background: #5a4a3a; color: white; border: none; "
        "border-radius: 4px; padding: 8px 18px; font-weight: bold; }"
        "QPushButton:hover { background: #7a6a4a; }");
    footerLayout->addWidget(m_autoResolveBtn);

    footerLayout->addStretch();

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(
        "QPushButton { background: #3e3e42; color: white; border: 1px solid #555; "
        "border-radius: 4px; padding: 8px 18px; }"
        "QPushButton:hover { background: #4e4e52; }");
    footerLayout->addWidget(cancelBtn);

    m_applyBtn = new QPushButton("Apply Resolution");
    m_applyBtn->setEnabled(false);
    m_applyBtn->setStyleSheet(
        "QPushButton { background: #cc4400; color: white; border: none; "
        "border-radius: 4px; padding: 8px 18px; font-weight: bold; }"
        "QPushButton:hover { background: #ee5500; }"
        "QPushButton:disabled { background: #3e3e42; color: #666; }");
    footerLayout->addWidget(m_applyBtn);

    mainLayout->addWidget(footer);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyBtn, &QPushButton::clicked, this, [this]() {
        applyChanges();
        accept();
    });
    connect(m_autoResolveBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onAutoResolve);

    populateTree();
    updateSummary();
}

void ConflictResolutionDialog::populateTree() {
    m_tree->clear();

    for (int i = 0; i < m_conflicts.size(); ++i) {
        const auto& pair = m_conflicts[i];

        auto findModInfo = [this](const QString& name) -> QPair<QString, bool> {
            for (const auto& mod : m_mods) {
                if (mod.name == name) {
                    return {mod.version.isEmpty() ? QStringLiteral("v1.0") : mod.version.toString(), mod.enabled};
                }
            }
            return {"v1.0", true};
        };

        auto infoA = findModInfo(pair.modA);
        auto infoB = pair.modB.isEmpty() ? QPair<QString, bool>{} : findModInfo(pair.modB);

        auto* item = new QTreeWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Checked);
        item->setText(1, pair.modA + "  (" + infoA.first + ")");
        item->setText(2, pair.modB.isEmpty() ? "(self)" : pair.modB + "  (" + infoB.first + ")");
        item->setText(3, pair.reason);
        item->setData(0, Qt::UserRole, i);

        bool aEnabled = infoA.second;
        bool bEnabled = pair.modB.isEmpty() || infoB.second;

        QString status;
        if (pair.resolved) {
            if (pair.chosenMod == pair.modA) {
                status = "Keep: " + pair.modA;
            } else {
                status = "Keep: " + pair.modB;
            }
        } else {
            status = "Unresolved";
        }
        item->setText(4, status);

        if (pair.resolved) {
            item->setForeground(4, QColor("#4ade80"));
        } else {
            item->setForeground(4, QColor("#fbbf24"));
        }

        m_tree->addTopLevelItem(item);

        auto* buttonWidget = new QWidget();
        auto* btnLayout = new QHBoxLayout(buttonWidget);
        btnLayout->setContentsMargins(4, 2, 4, 2);
        btnLayout->setSpacing(4);

        {
            auto* keepABtn = new QPushButton("Keep " + pair.modA.left(12));
            keepABtn->setStyleSheet(
                "QPushButton { background: #3b82f6; color: white; border: none; "
                "border-radius: 3px; padding: 4px 10px; font-size: 11px; }"
                "QPushButton:hover { background: #2563eb; }");
            btnLayout->addWidget(keepABtn);

            int idx = i;
            connect(keepABtn, &QPushButton::clicked, this, [this, idx]() {
                onKeepClicked(m_conflicts[idx], m_conflicts[idx].modA);
            });
        }

        if (!pair.modB.isEmpty()) {
            auto* keepBBtn = new QPushButton("Keep " + pair.modB.left(12));
            keepBBtn->setStyleSheet(
                "QPushButton { background: #3b82f6; color: white; border: none; "
                "border-radius: 3px; padding: 4px 10px; font-size: 11px; }"
                "QPushButton:hover { background: #2563eb; }");
            btnLayout->addWidget(keepBBtn);

            int idx = i;
            connect(keepBBtn, &QPushButton::clicked, this, [this, idx]() {
                onKeepClicked(m_conflicts[idx], m_conflicts[idx].modB);
            });
        }

        btnLayout->addStretch();
        m_tree->setItemWidget(item, 4, buttonWidget);
    }
}

void ConflictResolutionDialog::onKeepClicked(const ConflictPair& pair, const QString& keepMod) {
    for (int i = 0; i < m_conflicts.size(); ++i) {
        if (m_conflicts[i].modA == pair.modA && m_conflicts[i].modB == pair.modB) {
            m_conflicts[i].resolved = true;
            m_conflicts[i].chosenMod = keepMod;

            auto* item = m_tree->topLevelItem(i);
            if (item) {
                item->setCheckState(0, Qt::Checked);
                auto* w = qobject_cast<QWidget*>(m_tree->itemWidget(item, 4));
                if (w) {
                    for (auto* btn : w->findChildren<QPushButton*>()) {
                        btn->setEnabled(false);
                    }
                }
            }
            break;
        }
    }
    updateSummary();

    bool allResolved = true;
    for (const auto& c : m_conflicts) {
        if (!c.resolved) { allResolved = false; break; }
    }
    if (allResolved) {
        onAutoResolve();
    }
}

void ConflictResolutionDialog::onAutoResolve() {
    for (int i = 0; i < m_conflicts.size(); ++i) {
        auto& pair = m_conflicts[i];
        if (pair.resolved) continue;

        auto findEnabled = [this](const QString& name) -> bool {
            for (const auto& mod : m_mods) {
                if (mod.name == name) return mod.enabled;
            }
            return true;
        };

        bool aEnabled = findEnabled(pair.modA);
        bool bEnabled = pair.modB.isEmpty() || findEnabled(pair.modB);

        QString keep = aEnabled ? pair.modA : pair.modB;
        if (!keep.isEmpty()) {
            pair.resolved = true;
            pair.chosenMod = keep;

            auto* item = m_tree->topLevelItem(i);
            if (item) {
                item->setCheckState(0, Qt::Checked);
                item->setText(4, "Auto: " + keep);
                item->setForeground(4, QColor("#4ade80"));
                auto* w = qobject_cast<QWidget*>(m_tree->itemWidget(item, 4));
                if (w) {
                    for (auto* btn : w->findChildren<QPushButton*>()) {
                        btn->setEnabled(false);
                    }
                }
            }
        }
    }
    updateSummary();
}

void ConflictResolutionDialog::updateSummary() {
    int total = m_conflicts.size();
    int resolved = 0;
    for (const auto& c : m_conflicts) {
        if (c.resolved) resolved++;
    }

    m_summaryLabel->setText(
        QString("%1 conflict%2 — %3 resolved, %4 remaining")
            .arg(total).arg(total == 1 ? "" : "s")
            .arg(resolved).arg(total - resolved));

    m_applyBtn->setEnabled(resolved > 0);
    m_autoResolveBtn->setEnabled(resolved < total);
}

void ConflictResolutionDialog::applyChanges() {
    QSet<QString> keepSet;
    QSet<QString> allConflictMods;

    for (const auto& c : m_conflicts) {
        allConflictMods.insert(c.modA);
        if (!c.modB.isEmpty()) allConflictMods.insert(c.modB);
    }

    for (const auto& c : m_conflicts) {
        if (c.resolved && !c.chosenMod.isEmpty()) {
            keepSet.insert(c.chosenMod);
        }
    }

    for (const auto& c : m_conflicts) {
        if (!c.resolved || c.chosenMod.isEmpty()) continue;
        if (c.chosenMod == c.modA && !c.modB.isEmpty()) {
            m_modsToDisable.append(c.modB);
        } else if (c.chosenMod == c.modB) {
            m_modsToDisable.append(c.modA);
        }
    }

    m_modsToDisable.removeDuplicates();

    for (const auto& c : m_conflicts) {
        if (c.resolved && !c.chosenMod.isEmpty()) {
            if (!m_modsToDisable.contains(c.chosenMod)) {
                m_modsToEnable.append(c.chosenMod);
            }
        }
    }
    m_modsToEnable.removeDuplicates();
}

} // namespace ks
