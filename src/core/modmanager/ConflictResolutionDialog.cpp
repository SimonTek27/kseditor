#include "ConflictResolutionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QProgressBar>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollArea>

namespace ks {

ConflictResolutionDialog::ConflictResolutionDialog(const ModConflictDetector::ConflictReport& report, QWidget* parent)
    : QDialog(parent)
    , m_report(report)
{
    setWindowTitle(tr("Resolve Mod Conflicts"));
    setMinimumSize(800, 600);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    
    setupUI();
    populateConflicts();
    
    // Auto-resolve non-critical conflicts
    m_autoResolve = true;
    updateButtonStates();
}

void ConflictResolutionDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Header
    QLabel* headerLabel = new QLabel(tr("Mod Conflicts Detected"));
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(headerLabel);
    
    QLabel* descLabel = new QLabel(tr(
        "The following file conflicts were detected between mods. "
        "Choose how to resolve each conflict. Critical conflicts (marked with ⚠) "
        "cannot be auto-merged and must be resolved manually."));
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888; margin-bottom: 10px;");
    mainLayout->addWidget(descLabel);
    
    // Summary
    QHBoxLayout* summaryLayout = new QHBoxLayout();
    m_totalLabel = new QLabel(tr("Total conflicts: %1").arg(m_report.conflicts.size()));
    m_criticalLabel = new QLabel(tr("Critical: %1").arg(m_report.criticalConflicts.size()));
    m_criticalLabel->setStyleSheet("color: #ff4444; font-weight: bold;");
    summaryLayout->addWidget(m_totalLabel);
    summaryLayout->addStretch();
    summaryLayout->addWidget(m_criticalLabel);
    mainLayout->addLayout(summaryLayout);
    
    // Auto-resolve checkbox
    QCheckBox* autoCheck = new QCheckBox(tr("Auto-resolve non-critical conflicts (use newer mod's files)"));
    autoCheck->setChecked(true);
    connect(autoCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_autoResolve = checked;
        updateButtonStates();
    });
    mainLayout->addWidget(autoCheck);
    
    // Progress bar for batch operations
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Conflict list
    m_conflictTree = new QTreeWidget();
    m_conflictTree->setHeaderLabels({tr("File"), tr("Conflicting Mods"), tr("Type"), tr("Action")});
    m_conflictTree->setAlternatingRowColors(true);
    m_conflictTree->setRootIsDecorated(false);
    m_conflictTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_conflictTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_conflictTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_conflictTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_conflictTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_conflictTree->setMinimumHeight(300);
    mainLayout->addWidget(m_conflictTree, 1);
    
    // Detail view
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    
    QWidget* detailWidget = new QWidget();
    QVBoxLayout* detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* detailHeader = new QLabel(tr("Conflict Details"));
    detailHeader->setStyleSheet("font-weight: bold; padding: 5px;");
    detailLayout->addWidget(detailHeader);
    
    m_detailText = new QTextEdit();
    m_detailText->setReadOnly(true);
    m_detailText->setMaximumHeight(150);
    m_detailText->setStyleSheet("background: #2b2b2b; border: 1px solid #444; padding: 5px;");
    detailLayout->addWidget(m_detailText);
    
    splitter->addWidget(m_conflictTree);
    splitter->addWidget(detailWidget);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    m_useNewerBtn = new QPushButton(tr("Use Newer Mod's Version"));
    m_useNewerBtn->setIcon(QIcon::fromTheme("document-save"));
    m_useNewerBtn->setEnabled(false);
    connect(m_useNewerBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onUseNewer);
    
    m_useOlderBtn = new QPushButton(tr("Use Existing Mod's Version"));
    m_useOlderBtn->setEnabled(false);
    connect(m_useOlderBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onUseOlder);
    
    m_skipBtn = new QPushButton(tr("Skip (Keep Both)"));
    m_skipBtn->setEnabled(false);
    connect(m_skipBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onSkip);
    
    m_mergeBtn = new QPushButton(tr("Merge (Config Files Only)"));
    m_mergeBtn->setEnabled(false);
    connect(m_mergeBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onMerge);
    
    m_autoResolveAllBtn = new QPushButton(tr("Auto-Resolve All Non-Critical"));
    m_autoResolveAllBtn->setIcon(QIcon::fromTheme("view-refresh"));
    connect(m_autoResolveAllBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onAutoResolveAll);
    
    actionLayout->addWidget(m_useNewerBtn);
    actionLayout->addWidget(m_useOlderBtn);
    actionLayout->addWidget(m_skipBtn);
    actionLayout->addWidget(m_mergeBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_autoResolveAllBtn);
    mainLayout->addLayout(actionLayout);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    // Connect selection change
    connect(m_conflictTree, &QTreeWidget::itemSelectionChanged,
            this, &ConflictResolutionDialog::onSelectionChanged);
    connect(m_conflictTree, &QTreeWidget::itemClicked,
            this, &ConflictResolutionDialog::onItemClicked);
}

void ConflictResolutionDialog::populateConflicts()
{
    m_conflictTree->clear();
    m_selectedItem = nullptr;
    
    for (const auto& conflict : m_report.conflicts) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, conflict.filePath);
        item->setText(1, conflict.modNames.join(" vs "));
        item->setText(2, conflict.conflictType);
        item->setText(3, tr("Pending"));
        
        // Store conflict data
        item->setData(0, Qt::UserRole, QVariant::fromValue(conflict));
        
        // Highlight critical conflicts
        if (m_report.criticalConflicts.contains(conflict.filePath)) {
            item->setForeground(0, QBrush(QColor("#ff4444")));
            item->setToolTip(0, tr("⚠ Critical conflict - cannot be auto-merged"));
            item->setIcon(0, QIcon::fromTheme("dialog-warning"));
        } else {
            item->setToolTip(0, tr("Double-click for details"));
        }
        
        // Pre-select action based on conflict type
        if (conflict.conflictType == "merge") {
            item->setText(3, tr("Will merge (config)"));
        } else if (conflict.conflictType == "overwrite") {
            item->setText(3, tr("Will overwrite"));
        } else {
            item->setText(3, tr("Will skip"));
        }
        
        m_conflictTree->addTopLevelItem(item);
    }
    
    m_conflictTree->resizeColumnToContents(1);
    m_conflictTree->resizeColumnToContents(2);
    m_conflictTree->resizeColumnToContents(3);
    
    // Select first item
    if (m_conflictTree->topLevelItemCount() > 0) {
        m_conflictTree->setCurrentItem(m_conflictTree->topLevelItem(0));
    }
}

void ConflictResolutionDialog::updateButtonStates()
{
    bool hasSelection = m_selectedItem != nullptr;
    bool isCritical = hasSelection && m_report.criticalConflicts.contains(m_selectedItem->text(0));
    
    m_useNewerBtn->setEnabled(hasSelection && !isCritical);
    m_useOlderBtn->setEnabled(hasSelection && !isCritical);
    m_skipBtn->setEnabled(hasSelection);
    m_mergeBtn->setEnabled(hasSelection && !isCritical && m_selectedConflict.conflictType == "merge");
    
    // Update auto-resolve button
    int nonCriticalCount = 0;
    for (int i = 0; i < m_conflictTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_conflictTree->topLevelItem(i);
        if (!m_report.criticalConflicts.contains(item->text(0)) && item->text(3) == "Pending") {
            nonCriticalCount++;
        }
    }
    m_autoResolveAllBtn->setEnabled(nonCriticalCount > 0);
}

void ConflictResolutionDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selected = m_conflictTree->selectedItems();
    m_selectedItem = selected.isEmpty() ? nullptr : selected.first();
    updateButtonStates();
    updateDetailView();
}

void ConflictResolutionDialog::onItemClicked(QTreeWidgetItem* item, int column)
{
    m_selectedItem = item;
    m_selectedConflict = item->data(0, Qt::UserRole).value<ModConflictDetector::FileConflict>();
    updateButtonStates();
    updateDetailView();
}

void ConflictResolutionDialog::updateDetailView()
{
    if (!m_selectedItem || !m_selectedConflict.filePath.isEmpty()) {
        QString details;
        
        const auto& c = m_selectedConflict;
        details += QString("<b>File:</b> %1<br>").arg(c.filePath);
        details += QString("<b>Mods:</b> %1<br>").arg(c.modNames.join(" vs "));
        details += QString("<b>Type:</b> %1<br>").arg(c.conflictType);
        
        if (m_report.criticalConflicts.contains(c.filePath)) {
            details += "<br><b style='color: #ff4444;'>⚠ CRITICAL CONFLICT</b><br>";
            details += "This file is critical and cannot be auto-merged.<br>";
        }
        
        details += "<br><b>Mods involved:</b><ul>";
        for (const QString& mod : c.modNames) {
            details += QString("<li>%1</li>").arg(mod);
        }
        details += "</ul>";
        
        details += "<br><b>Recommended action:</b> ";
        if (c.conflictType == "merge") {
            details += "Merge configuration files (recommended for .ini, .json, .lua files).";
        } else if (c.conflictType == "overwrite") {
            details += "Choose which mod's version to keep (binary assets cannot be merged).";
        } else {
            details += "Skip - both mods provide this file identically.";
        }
        
        m_detailText->setHtml(details);
    } else {
        m_detailText->clear();
        m_detailText->setHtml("<i>Select a conflict to view details</i>");
    }
}

void ConflictResolutionDialog::onUseNewer()
{
    if (!m_selectedItem) return;
    applyResolution("newer");
}

void ConflictResolutionDialog::onUseOlder()
{
    if (!m_selectedItem) return;
    applyResolution("older");
}

void ConflictResolutionDialog::onSkip()
{
    if (!m_selectedItem) return;
    applyResolution("skip");
}

void ConflictResolutionDialog::onMerge()
{
    if (!m_selectedItem) return;
    applyResolution("merge");
}

void ConflictResolutionDialog::applyResolution(const QString& action)
{
    if (!m_selectedItem) return;
    
    m_selectedItem->setText(3, tr("%1 (resolved)").arg(action));
    m_selectedItem->setBackground(3, QBrush(QColor("#2e7d32")));
    m_selectedItem->setForeground(3, QBrush(Qt::white));
    
    // Store resolution for later processing
    m_resolutions[m_selectedConflict.filePath] = action;
    
    updateButtonStates();
    
    // Auto-select next unresolved conflict
    for (int i = 0; i < m_conflictTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_conflictTree->topLevelItem(i);
        if (item->text(3) == "Pending") {
            m_conflictTree->setCurrentItem(item);
            break;
        }
    }
}

void ConflictResolutionDialog::onAutoResolveAll()
{
    if (!m_autoResolve) return;
    
    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(0);
    m_progressBar->setValue(0);
    
    int resolved = 0;
    int total = 0;
    
    for (int i = 0; i < m_conflictTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_conflictTree->topLevelItem(i);
        if (item->text(3) == "Pending") {
            total++;
            bool isCritical = m_report.criticalConflicts.contains(item->text(0));
            
            if (!isCritical) {
                // Auto-resolve based on conflict type
                ModConflictDetector::FileConflict conflict = item->data(0, Qt::UserRole).value<ModConflictDetector::FileConflict>();
                QString action;
                
                if (conflict.conflictType == "merge") {
                    action = "merge";
                } else if (conflict.conflictType == "overwrite") {
                    // Use newer mod (conflict.modNames.contains("newer") ? "newer" : "older";
                } else {
                    action = "skip";
                }
                
                applyResolution(action);
                resolved++;
            }
        }
    }
    
    QMessageBox::information(this, tr("Auto-Resolve Complete"),
        tr("Resolved %1 of %2 non-critical conflicts automatically.").arg(resolved).arg(total));
    
    m_progressBar->setVisible(false);
    updateButtonStates();
}

} // namespace ks