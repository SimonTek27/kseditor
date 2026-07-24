#include "VcsEditorModule.h"
#include "GitStatusWidget.h"
#include "GitManager.h"
#include "core/editor/ModuleGuiBase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>

namespace ks {

VcsEditorModule::VcsEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_statusTab(nullptr)
    , m_repoPathEdit(nullptr)
    , m_browseRepoBtn(nullptr)
    , m_initRepoBtn(nullptr)
    , m_cloneRepoBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_gitWidget(nullptr)
    , m_repoStatusLabel(nullptr)
    , m_branchesTab(nullptr)
    , m_branchesTree(nullptr)
    , m_branchCombo(nullptr)
    , m_checkoutBranchBtn(nullptr)
    , m_createBranchBtn(nullptr)
    , m_deleteBranchBtn(nullptr)
    , m_mergeBranchBtn(nullptr)
    , m_newBranchEdit(nullptr)
    , m_historyTab(nullptr)
    , m_historyTree(nullptr)
    , m_commitDetails(nullptr)
    , m_historyLimitSpin(nullptr)
    , m_remotesTab(nullptr)
    , m_remotesTree(nullptr)
    , m_remoteCombo(nullptr)
    , m_pullBtn(nullptr)
    , m_pushBtn(nullptr)
    , m_fetchBtn(nullptr)
    , m_addRemoteBtn(nullptr)
    , m_removeRemoteBtn(nullptr)
    , m_stashesTab(nullptr)
    , m_stashesList(nullptr)
    , m_stashDetails(nullptr)
    , m_stashBtn(nullptr)
    , m_stashPopBtn(nullptr)
    , m_stashDropBtn(nullptr)
    , m_stashApplyBtn(nullptr)
    , m_tagsTab(nullptr)
    , m_tagsTree(nullptr)
    , m_createTagBtn(nullptr)
    , m_deleteTagBtn(nullptr)
    , m_pushTagBtn(nullptr)
    , m_newTagEdit(nullptr)
    , m_settingsTab(nullptr)
    , m_userNameEdit(nullptr)
    , m_userEmailEdit(nullptr)
    , m_autoFetchCheck(nullptr)
    , m_fetchIntervalSpin(nullptr)
    , m_diffToolCombo(nullptr)
    , m_mergeToolCombo(nullptr)
    , m_currentRepoPath("")
    , m_gitManager(nullptr)
{
    setObjectName("VcsEditorModule");
}

bool VcsEditorModule::initialize() {
    if (m_uiBuilt) return true;
    
    m_gitManager = new GitManager(this);
    
    ModuleGuiBase::initialize();
    
    loadSettings();
    return true;
}

void VcsEditorModule::shutdown() {
    saveSettings();
    ModuleGuiBase::shutdown();
}

void VcsEditorModule::importFile(const QString& filePath) {
    QFileInfo info(filePath);
    if (info.isDir()) {
        m_currentRepoPath = filePath;
        m_repoPathEdit->setText(filePath);
        refreshAll();
        logSuccess("Loaded repository: " + filePath);
    } else {
        log("Import not applicable for VCS module - select a git directory");
    }
}

void VcsEditorModule::exportFile(const QString& filePath) {
    if (m_currentRepoPath.isEmpty()) {
        logError("No repository to export");
        return;
    }
    log("Export: pushing to remotes...");
    runGitCommand({"push", "--all"}, m_currentRepoPath);
    logSuccess("Exported (pushed) repository");
}

void VcsEditorModule::buildUI() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );
    
    setupStatusTab();
    setupBranchesTab();
    setupHistoryTab();
    setupRemotesTab();
    setupStashesTab();
    setupTagsTab();
    setupSettingsTab();
    
    m_tabWidget->addTab(m_statusTab, "Status");
    m_tabWidget->addTab(m_branchesTab, "Branches");
    m_tabWidget->addTab(m_historyTab, "History");
    m_tabWidget->addTab(m_remotesTab, "Remotes");
    m_tabWidget->addTab(m_stashesTab, "Stashes");
    m_tabWidget->addTab(m_tagsTab, "Tags");
    m_tabWidget->addTab(m_settingsTab, "Settings");
    
    m_mainLayout->insertWidget(1, m_tabWidget, 1);
}

void VcsEditorModule::setupStatusTab() {
    m_statusTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_statusTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Repository path
    QGroupBox* repoGroup = createGroupBox("Repository");
    QHBoxLayout* repoLayout = new QHBoxLayout(repoGroup);
    
    repoLayout->addWidget(createLabel("Path:"));
    m_repoPathEdit = new QLineEdit();
    m_repoPathEdit->setPlaceholderText("Select a git repository...");
    connect(m_repoPathEdit, &QLineEdit::textChanged, this, &VcsEditorModule::onRepoPathChanged);
    repoLayout->addWidget(m_repoPathEdit, 1);
    
    m_browseRepoBtn = createButton("Browse...");
    connect(m_browseRepoBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Select Git Repository");
        if (!dir.isEmpty()) {
            m_repoPathEdit->setText(dir);
            loadRepo(dir);
        }
    });
    repoLayout->addWidget(m_browseRepoBtn);
    
    m_initRepoBtn = createButton("Init", "primary");
    connect(m_initRepoBtn, &QPushButton::clicked, this, &VcsEditorModule::onInitRepoClicked);
    repoLayout->addWidget(m_initRepoBtn);
    
    m_cloneRepoBtn = createButton("Clone");
    connect(m_cloneRepoBtn, &QPushButton::clicked, this, &VcsEditorModule::onCloneRepoClicked);
    repoLayout->addWidget(m_cloneRepoBtn);
    
    layout->addWidget(repoGroup);
    
    // Status bar
    QGroupBox* toolbarGroup = createGroupBox("Commands");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarGroup);
    
    m_refreshBtn = createButton("Refresh", "success");
    connect(m_refreshBtn, &QPushButton::clicked, this, &VcsEditorModule::onRefreshClicked);
    toolbarLayout->addWidget(m_refreshBtn);
    
    QPushButton* stageAllBtn = createButton("Stage All");
    connect(stageAllBtn, &QPushButton::clicked, this, &VcsEditorModule::onStageAllClicked);
    toolbarLayout->addWidget(stageAllBtn);
    
    QPushButton* unstageAllBtn = createButton("Unstage All", "warning");
    connect(unstageAllBtn, &QPushButton::clicked, this, &VcsEditorModule::onUnstageAllClicked);
    toolbarLayout->addWidget(unstageAllBtn);
    
    QPushButton* discardBtn = createButton("Discard Changes", "danger");
    connect(discardBtn, &QPushButton::clicked, this, &VcsEditorModule::onDiscardChangesClicked);
    toolbarLayout->addWidget(discardBtn);
    
    toolbarLayout->addSpacing(20);
    
    m_pullBtn = createButton("Pull");
    connect(m_pullBtn, &QPushButton::clicked, this, &VcsEditorModule::onPullClicked);
    toolbarLayout->addWidget(m_pullBtn);
    
    m_pushBtn = createButton("Push", "primary");
    connect(m_pushBtn, &QPushButton::clicked, this, &VcsEditorModule::onPushClicked);
    toolbarLayout->addWidget(m_pushBtn);
    
    m_fetchBtn = createButton("Fetch");
    connect(m_fetchBtn, &QPushButton::clicked, this, &VcsEditorModule::onFetchClicked);
    toolbarLayout->addWidget(m_fetchBtn);
    
    toolbarLayout->addStretch();
    
    m_repoStatusLabel = createLabel("No repository loaded", "color: #ff6b6b; font-weight: bold;");
    toolbarLayout->addWidget(m_repoStatusLabel);
    
    layout->addWidget(toolbarGroup);
    
    // Git status widget (reused from existing implementation)
    m_gitWidget = new GitStatusWidget();
    m_gitWidget->setStyleSheet("background: #1e1e1e; border: 1px solid #3a3a3a;");
    layout->addWidget(m_gitWidget, 1);
}

void VcsEditorModule::setupBranchesTab() {
    m_branchesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_branchesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Branch controls
    QGroupBox* ctrlGroup = createGroupBox("Branch Operations");
    QHBoxLayout* ctrlLayout = new QHBoxLayout(ctrlGroup);
    
    ctrlLayout->addWidget(createLabel("Branch:"));
    m_branchCombo = new QComboBox();
    m_branchCombo->setMinimumWidth(200);
    connect(m_branchCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &VcsEditorModule::onBranchChanged);
    ctrlLayout->addWidget(m_branchCombo);
    
    m_checkoutBranchBtn = createButton("Checkout", "success");
    connect(m_checkoutBranchBtn, &QPushButton::clicked, this, &VcsEditorModule::onCheckoutBranchClicked);
    ctrlLayout->addWidget(m_checkoutBranchBtn);
    
    m_mergeBranchBtn = createButton("Merge");
    connect(m_mergeBranchBtn, &QPushButton::clicked, this, &VcsEditorModule::onMergeBranchClicked);
    ctrlLayout->addWidget(m_mergeBranchBtn);
    
    ctrlLayout->addStretch();
    
    m_newBranchEdit = new QLineEdit();
    m_newBranchEdit->setPlaceholderText("New branch name...");
    m_newBranchEdit->setMaximumWidth(200);
    connect(m_newBranchEdit, &QLineEdit::returnPressed, this, &VcsEditorModule::onCreateBranchClicked);
    ctrlLayout->addWidget(m_newBranchEdit);
    
    m_createBranchBtn = createButton("Create", "primary");
    connect(m_createBranchBtn, &QPushButton::clicked, this, &VcsEditorModule::onCreateBranchClicked);
    ctrlLayout->addWidget(m_createBranchBtn);
    
    m_deleteBranchBtn = createButton("Delete", "danger");
    connect(m_deleteBranchBtn, &QPushButton::clicked, this, &VcsEditorModule::onDeleteBranchClicked);
    ctrlLayout->addWidget(m_deleteBranchBtn);
    
    layout->addWidget(ctrlGroup);
    
    // Branches tree
    m_branchesTree = createTreeWidget({"Branch", "Type", "Latest Commit", "Author", "Date", "Status"});
    m_branchesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_branchesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_branchesTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_branchesTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_branchesTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_branchesTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_branchesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_branchesTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTreeWidgetItem* item = m_branchesTree->itemAt(pos);
        if (!item) return;
        QMenu menu;
        QAction* checkoutAct = menu.addAction("Checkout");
        connect(checkoutAct, &QAction::triggered, this, &VcsEditorModule::onCheckoutBranchClicked);
        QAction* mergeAct = menu.addAction("Merge into current");
        connect(mergeAct, &QAction::triggered, this, &VcsEditorModule::onMergeBranchClicked);
        QAction* delAct = menu.addAction("Delete");
        connect(delAct, &QAction::triggered, this, &VcsEditorModule::onDeleteBranchClicked);
        QAction* copyAct = menu.addAction("Copy name");
        connect(copyAct, &QAction::triggered, this, [item]() { QApplication::clipboard()->setText(item->text(0)); });
        menu.exec(m_branchesTree->viewport()->mapToGlobal(pos));
    });
    layout->addWidget(m_branchesTree, 1);
}

void VcsEditorModule::setupHistoryTab() {
    m_historyTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_historyTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* histGroup = createGroupBox("Commit History");
    QVBoxLayout* histLayout = new QVBoxLayout(histGroup);
    
    QHBoxLayout* histCtrlLayout = new QHBoxLayout();
    histCtrlLayout->addWidget(createLabel("Limit:"));
    m_historyLimitSpin = createSpinBox(1, 1000, 100);
    m_historyLimitSpin->setMaximumWidth(80);
    connect(m_historyLimitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { populateHistory(); });
    histCtrlLayout->addWidget(m_historyLimitSpin);
    
    QPushButton* refreshHistBtn = createButton("Refresh");
    connect(refreshHistBtn, &QPushButton::clicked, this, &VcsEditorModule::populateHistory);
    histCtrlLayout->addWidget(refreshHistBtn);
    
    histCtrlLayout->addStretch();
    histLayout->addLayout(histCtrlLayout);
    
    QSplitter* histSplitter = createSplitter(Qt::Vertical);
    
    m_historyTree = createTreeWidget({"Commit", "Author", "Date", "Message", "Branch"});
    m_historyTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_historyTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_historyTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_historyTree, &QTreeWidget::itemClicked, this, &VcsEditorModule::onLogItemClicked);
    connect(m_historyTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTreeWidgetItem* item = m_historyTree->itemAt(pos);
        if (!item) return;
        QMenu menu;
        QAction* copyAct = menu.addAction("Copy Commit Hash");
        connect(copyAct, &QAction::triggered, this, [item]() { QApplication::clipboard()->setText(item->text(0)); });
        QAction* revertAct = menu.addAction("Revert Commit");
        connect(revertAct, &QAction::triggered, this, [this, item]() {
            if (confirmAction("Revert", "Revert commit " + item->text(0) + "?")) {
                QString output = runGitCommand({"revert", "--no-edit", item->text(0)});
                log(output);
                populateHistory();
                if (m_gitWidget) m_gitWidget->refresh();
            }
        });
        QAction* diffAct = menu.addAction("Show Diff");
        connect(diffAct, &QAction::triggered, this, [this, item]() {
            QString diff = runGitCommand({"diff", item->text(0) + "~1", item->text(0)});
            if (m_commitDetails) m_commitDetails->setText(diff);
        });
        menu.exec(m_historyTree->viewport()->mapToGlobal(pos));
    });
    histSplitter->addWidget(m_historyTree);
    
    m_commitDetails = new QTextEdit();
    m_commitDetails->setReadOnly(true);
    m_commitDetails->setMaximumHeight(200);
    m_commitDetails->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; border: 1px solid #3a3a3a; }");
    histSplitter->addWidget(m_commitDetails);
    
    histLayout->addWidget(histSplitter);
    layout->addWidget(histGroup, 1);
}

void VcsEditorModule::setupRemotesTab() {
    m_remotesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_remotesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* remoteGroup = createGroupBox("Remote Repositories");
    QVBoxLayout* remoteLayout = new QVBoxLayout(remoteGroup);
    
    QHBoxLayout* ctrlLayout = new QHBoxLayout();
    m_remoteCombo = new QComboBox();
    m_remoteCombo->setMinimumWidth(150);
    connect(m_remoteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VcsEditorModule::onRemoteChanged);
    ctrlLayout->addWidget(m_remoteCombo);
    
    m_fetchBtn = createButton("Fetch");
    connect(m_fetchBtn, &QPushButton::clicked, this, &VcsEditorModule::onFetchClicked);
    ctrlLayout->addWidget(m_fetchBtn);
    
    m_pullBtn = createButton("Pull");
    connect(m_pullBtn, &QPushButton::clicked, this, &VcsEditorModule::onPullClicked);
    ctrlLayout->addWidget(m_pullBtn);
    
    m_pushBtn = createButton("Push", "success");
    connect(m_pushBtn, &QPushButton::clicked, this, &VcsEditorModule::onPushClicked);
    ctrlLayout->addWidget(m_pushBtn);
    
    m_addRemoteBtn = createButton("Add Remote", "primary");
    connect(m_addRemoteBtn, &QPushButton::clicked, this, &VcsEditorModule::onAddRemoteClicked);
    ctrlLayout->addWidget(m_addRemoteBtn);
    
    m_removeRemoteBtn = createButton("Remove", "danger");
    connect(m_removeRemoteBtn, &QPushButton::clicked, this, &VcsEditorModule::onRemoveRemoteClicked);
    ctrlLayout->addWidget(m_removeRemoteBtn);
    
    ctrlLayout->addStretch();
    remoteLayout->addLayout(ctrlLayout);
    
    m_remotesTree = createTreeWidget({"Remote", "URL", "Fetch URL", "Push URL", "Last Fetched", "Status"});
    m_remotesTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_remotesTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_remotesTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_remotesTree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_remotesTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_remotesTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_remotesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_remotesTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTreeWidgetItem* item = m_remotesTree->itemAt(pos);
        if (!item) return;
        QMenu menu;
        QAction* fetchAct = menu.addAction("Fetch");
        connect(fetchAct, &QAction::triggered, this, &VcsEditorModule::onFetchClicked);
        QAction* pullAct = menu.addAction("Pull");
        connect(pullAct, &QAction::triggered, this, &VcsEditorModule::onPullClicked);
        QAction* removeAct = menu.addAction("Remove");
        connect(removeAct, &QAction::triggered, this, &VcsEditorModule::onRemoveRemoteClicked);
        menu.exec(m_remotesTree->viewport()->mapToGlobal(pos));
    });
    remoteLayout->addWidget(m_remotesTree, 1);
    
    layout->addWidget(remoteGroup, 1);
}

void VcsEditorModule::setupStashesTab() {
    m_stashesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_stashesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* stashGroup = createGroupBox("Stashes");
    QVBoxLayout* stashLayout = new QVBoxLayout(stashGroup);
    
    QHBoxLayout* ctrlLayout = new QHBoxLayout();
    m_stashBtn = createButton("Stash Changes", "primary");
    connect(m_stashBtn, &QPushButton::clicked, this, &VcsEditorModule::onStashClicked);
    ctrlLayout->addWidget(m_stashBtn);
    
    m_stashPopBtn = createButton("Pop Stash", "success");
    connect(m_stashPopBtn, &QPushButton::clicked, this, &VcsEditorModule::onStashPopClicked);
    ctrlLayout->addWidget(m_stashPopBtn);
    
    m_stashApplyBtn = createButton("Apply Stash");
    connect(m_stashApplyBtn, &QPushButton::clicked, this, &VcsEditorModule::onStashPopClicked);
    ctrlLayout->addWidget(m_stashApplyBtn);
    
    m_stashDropBtn = createButton("Drop Stash", "danger");
    connect(m_stashDropBtn, &QPushButton::clicked, this, [this]() {
        if (m_stashesList->currentItem() && confirmAction("Drop Stash", "Drop this stash?")) {
            runGitCommand({"stash", "drop", "stash@" + QString::number(m_stashesList->currentRow())});
            populateStashes();
        }
    });
    ctrlLayout->addWidget(m_stashDropBtn);
    
    ctrlLayout->addStretch();
    stashLayout->addLayout(ctrlLayout);
    
    QSplitter* stashSplitter = createSplitter(Qt::Vertical);
    
    m_stashesList = new QListWidget();
    m_stashesList->setAlternatingRowColors(true);
    m_stashesList->setStyleSheet("QListWidget { background: #1e1e1e; color: #ddd; border: 1px solid #3a3a3a; font-size: 11px; } QListWidget::item { padding: 4px; } QListWidget::item:selected { background: #3a5a8a; }");
    stashSplitter->addWidget(m_stashesList);
    
    m_stashDetails = new QTextEdit();
    m_stashDetails->setReadOnly(true);
    m_stashDetails->setMaximumHeight(150);
    m_stashDetails->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; border: 1px solid #3a3a3a; }");
    stashSplitter->addWidget(m_stashDetails);
    
    stashLayout->addWidget(stashSplitter);
    layout->addWidget(stashGroup, 1);
}

void VcsEditorModule::setupTagsTab() {
    m_tagsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_tagsTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* tagGroup = createGroupBox("Tags");
    QVBoxLayout* tagLayout = new QVBoxLayout(tagGroup);
    
    QHBoxLayout* ctrlLayout = new QHBoxLayout();
    ctrlLayout->addWidget(createLabel("New Tag:"));
    m_newTagEdit = new QLineEdit();
    m_newTagEdit->setPlaceholderText("v1.0.0");
    m_newTagEdit->setMaximumWidth(200);
    connect(m_newTagEdit, &QLineEdit::returnPressed, this, &VcsEditorModule::onCreateTagClicked);
    ctrlLayout->addWidget(m_newTagEdit);
    
    m_createTagBtn = createButton("Create Tag", "success");
    connect(m_createTagBtn, &QPushButton::clicked, this, &VcsEditorModule::onCreateTagClicked);
    ctrlLayout->addWidget(m_createTagBtn);
    
    m_deleteTagBtn = createButton("Delete Tag", "danger");
    connect(m_deleteTagBtn, &QPushButton::clicked, this, &VcsEditorModule::onDeleteBranchClicked);
    ctrlLayout->addWidget(m_deleteTagBtn);
    
    m_pushTagBtn = createButton("Push Tags");
    connect(m_pushTagBtn, &QPushButton::clicked, this, [this]() {
        runGitCommand({"push", "--tags"});
        log("Tags pushed");
    });
    ctrlLayout->addWidget(m_pushTagBtn);
    
    ctrlLayout->addStretch();
    tagLayout->addLayout(ctrlLayout);
    
    m_tagsTree = createTreeWidget({"Tag", "Commit", "Type", "Author", "Date", "Message"});
    m_tagsTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tagsTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tagsTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tagsTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tagsTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tagsTree->header()->setSectionResizeMode(5, QHeaderView::Stretch);
    tagLayout->addWidget(m_tagsTree, 1);
    
    layout->addWidget(tagGroup, 1);
}

void VcsEditorModule::setupSettingsTab() {
    m_settingsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_settingsTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* userGroup = createGroupBox("User Configuration");
    QFormLayout* userLayout = new QFormLayout(userGroup);
    
    m_userNameEdit = new QLineEdit();
    m_userNameEdit->setPlaceholderText("Your name");
    connect(m_userNameEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_userNameEdit->text().isEmpty()) {
            QProcess::execute("git", {"config", "--global", "user.name", m_userNameEdit->text()});
        }
    });
    userLayout->addRow("Name:", m_userNameEdit);
    
    m_userEmailEdit = new QLineEdit();
    m_userEmailEdit->setPlaceholderText("email@example.com");
    connect(m_userEmailEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_userEmailEdit->text().isEmpty()) {
            QProcess::execute("git", {"config", "--global", "user.email", m_userEmailEdit->text()});
        }
    });
    userLayout->addRow("Email:", m_userEmailEdit);
    
    layout->addWidget(userGroup);
    
    QGroupBox* autoGroup = createGroupBox("Automatic Operations");
    QFormLayout* autoLayout = new QFormLayout(autoGroup);
    
    m_autoFetchCheck = createCheckBox("Auto-fetch from remote", true);
    connect(m_autoFetchCheck, &QCheckBox::toggled, this, [this](bool) { saveSettings(); });
    autoLayout->addRow(m_autoFetchCheck);
    
    m_fetchIntervalSpin = createSpinBox(1, 300, 30, " min");
    connect(m_fetchIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { saveSettings(); });
    autoLayout->addRow("Fetch Interval:", m_fetchIntervalSpin);
    
    layout->addWidget(autoGroup);
    
    QGroupBox* toolGroup = createGroupBox("External Tools");
    QFormLayout* toolLayout = new QFormLayout(toolGroup);
    
    m_diffToolCombo = createComboBox({"Built-in", "vimdiff", "meld", "kdiff3", "bcomp", "winmerge", "araxis", "custom..."});
    connect(m_diffToolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { saveSettings(); });
    toolLayout->addRow("Diff Tool:", m_diffToolCombo);
    
    m_mergeToolCombo = createComboBox({"Built-in", "vimdiff", "meld", "kdiff3", "bcomp", "winmerge", "araxis", "custom..."});
    toolLayout->addRow("Merge Tool:", m_mergeToolCombo);
    
    toolLayout->addRow("", createLabel("Changes take effect on next operation", "color: #666; font-size: 10px;"));
    
    layout->addWidget(toolGroup);
    layout->addStretch();
}

void VcsEditorModule::loadRepo(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        updateStatusLabel("Directory does not exist", false);
        return;
    }
    
    // Check if it's a git repo
    QString gitDir = path + "/.git";
    if (!QDir(gitDir).exists()) {
        updateStatusLabel("Not a git repository. Click 'Init' to create one.", false);
        return;
    }
    
    m_currentRepoPath = path;
    m_gitWidget->setRepoPath(path);
    
    // Update UI
    updateStatusLabel(QString("Repository: %1").arg(QFileInfo(path).fileName()), true);
    
    // Refresh all displays
    refreshAll();
}

void VcsEditorModule::refreshAll() {
    if (m_currentRepoPath.isEmpty()) return;
    
    populateBranches();
    populateRemotes();
    populateStashes();
    populateTags();
    populateHistory();
    
    if (m_gitWidget) m_gitWidget->refresh();
}

void VcsEditorModule::updateStatusLabel(const QString& msg, bool success) {
    if (m_repoStatusLabel) {
        m_repoStatusLabel->setText(msg);
        m_repoStatusLabel->setStyleSheet(success ? "color: #6bff6b; font-weight: bold;" : "color: #ff6b6b; font-weight: bold;");
    }
}

QString VcsEditorModule::runGitCommand(const QStringList& args, const QString& workDir) {
    QString pwd = workDir.isEmpty() ? m_currentRepoPath : workDir;
    if (pwd.isEmpty()) {
        logError("No repository path set");
        return "";
    }
    
    QProcess proc;
    proc.setWorkingDirectory(pwd);
    proc.start("git", args);
    
    if (!proc.waitForFinished(30000)) {
        proc.kill();
        QString err = "Git command timed out: git " + args.join(" ");
        logError(err);
        return err;
    }
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString error = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    
    if (proc.exitCode() != 0 && !error.isEmpty()) {
        logError("git " + args.join(" ") + ": " + error);
        return output.isEmpty() ? error : output + "\n" + error;
    }
    
    if (!output.isEmpty()) log("git " + args.join(" ") + ":\n" + output);
    return output;
}

void VcsEditorModule::populateBranches() {
    m_branchesTree->clear();
    m_branchCombo->clear();
    
    if (m_currentRepoPath.isEmpty()) return;
    
    // Get branches
    QString output = runGitCommand({"branch", "-a"});
    QStringList branches = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : branches) {
        bool isCurrent = line.startsWith('*');
        QString branchName = line.mid(2).trimmed();
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_branchesTree);
        item->setText(0, branchName);
        item->setText(1, branchName.startsWith("remotes/") ? "Remote" : "Local");
        item->setText(5, isCurrent ? "Current" : "");
        
        if (isCurrent) {
            item->setForeground(0, QBrush(QColor("#6bff6b")));
            item->setForeground(5, QBrush(QColor("#6bff6b")));
        }
        
        m_branchCombo->addItem(branchName);
        
        // Get latest commit info
        QString log = runGitCommand({"log", "--oneline", "-1", branchName});
        if (!log.isEmpty()) {
            QStringList logParts = log.split(' ', Qt::SkipEmptyParts);
            if (logParts.size() > 1) {
                item->setText(2, logParts[0] + " " + logParts.mid(1).join(' '));
            }
        }
    }
}

void VcsEditorModule::populateRemotes() {
    m_remotesTree->clear();
    m_remoteCombo->clear();
    
    if (m_currentRepoPath.isEmpty()) return;
    
    QString output = runGitCommand({"remote", "-v"});
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QMap<QString, QPair<QString, QString>> remotes; // name -> (fetch, push)
    for (const QString& line : lines) {
        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            QString name = parts[0];
            QString url = parts[1];
            if (url.contains("(fetch)")) remotes[name].first = url.section(" (", 0, 0).trimmed();
            else if (url.contains("(push)")) remotes[name].second = url.section(" (", 0, 0).trimmed();
        }
    }
    
    for (auto it = remotes.begin(); it != remotes.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_remotesTree);
        item->setText(0, it.key());
        item->setText(1, it.value().first);
        item->setText(2, it.value().second.isEmpty() ? it.value().first : it.value().second);
        item->setText(5, "Active");
        m_remoteCombo->addItem(it.key());
    }
}

void VcsEditorModule::populateStashes() {
    m_stashesList->clear();
    
    if (m_currentRepoPath.isEmpty()) return;
    
    QString output = runGitCommand({"stash", "list"});
    QStringList stashes = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& stash : stashes) {
        m_stashesList->addItem(stash);
    }
}

void VcsEditorModule::populateTags() {
    m_tagsTree->clear();
    
    if (m_currentRepoPath.isEmpty()) return;
    
    QString output = runGitCommand({"tag", "-l", "--format=%(refname:short)|||%(objectname)|||%(taggerdate)|||%(contents:subject)"});
    QStringList tags = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& tagLine : tags) {
        QStringList parts = tagLine.split("|||");
        if (parts.isEmpty()) continue;
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_tagsTree);
        item->setText(0, parts[0]);
        if (parts.size() > 1) item->setText(1, parts[1]);
        if (parts.size() > 3) item->setText(5, parts[3]);
    }
}

void VcsEditorModule::populateHistory() {
    m_historyTree->clear();
    
    if (m_currentRepoPath.isEmpty()) return;
    
    int limit = m_historyLimitSpin ? m_historyLimitSpin->value() : 100;
    QString output = runGitCommand({"log", "--oneline", "--graph", "--all", "--decorate", 
        "--format=format:%H|||%an|||%ai|||%s|||%D", "-" + QString::number(limit)});
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        // Skip graph chars
        QString clean = line;
        while (clean.startsWith('|') || clean.startsWith('*') || clean.startsWith('/') || 
               clean.startsWith('\\') || clean.startsWith(' ') || clean.startsWith('-')) {
            clean = clean.mid(1);
        }
        clean = clean.trimmed();
        
        QStringList parts = clean.split("|||");
        if (parts.size() < 4) continue;
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_historyTree);
        item->setText(0, parts[0].left(8)); // Short hash
        item->setText(1, parts[1]); // Author
        item->setText(2, parts[2].left(19)); // Date (ISO format)
        item->setText(3, parts[3]); // Message
        
        if (parts.size() > 4 && !parts[4].isEmpty()) {
            item->setText(4, parts[4]); // Branch/tag info
        }
        
        item->setData(0, Qt::UserRole, parts[0]); // Full hash
    }
}

// Slots
void VcsEditorModule::onRepoPathChanged(const QString& path) {
    m_currentRepoPath = path;
    if (QDir(path).exists(".git")) {
        refreshAll();
        updateStatusLabel("Repository loaded", true);
    } else {
        updateStatusLabel("No git repository", false);
    }
}

void VcsEditorModule::onRefreshClicked() {
    if (m_currentRepoPath.isEmpty()) return;
    refreshAll();
    log("Repository refreshed");
}

void VcsEditorModule::onInitRepoClicked() {
    if (m_currentRepoPath.isEmpty()) {
        QString dir = selectDirectory("Initialize Repository In");
        if (dir.isEmpty()) return;
        m_currentRepoPath = dir;
        m_repoPathEdit->setText(dir);
    }
    
    QDir dir(m_currentRepoPath);
    if (!dir.exists(".git")) {
        runGitCommand({"init"}, m_currentRepoPath);
        updateStatusLabel("Repository initialized", true);
        logSuccess("Initialized git repository in " + m_currentRepoPath);
        refreshAll();
    } else {
        logError("Already a git repository");
    }
}

void VcsEditorModule::onCloneRepoClicked() {
    bool ok;
    QString url = QInputDialog::getText(this, "Clone Repository", "Repository URL:", QLineEdit::Normal, "", &ok);
    if (!ok || url.isEmpty()) return;
    
    QString dest = selectDirectory("Clone Destination");
    if (dest.isEmpty()) return;
    
    log("Cloning " + url + " to " + dest + "...");
    runGitCommand({"clone", url, dest});
    
    m_currentRepoPath = dest;
    m_repoPathEdit->setText(dest);
    refreshAll();
    logSuccess("Repository cloned successfully");
}

void VcsEditorModule::onBranchChanged(const QString& branch) {
    if (!branch.isEmpty()) {
        runGitCommand({"checkout", branch}, m_currentRepoPath);
        updateStatusLabel(QString("Switched to branch: %1").arg(branch), true);
        populateHistory();
        if (m_gitWidget) m_gitWidget->refresh();
        log("Switched to branch: " + branch);
    }
}

void VcsEditorModule::onCreateBranchClicked() {
    QString name = m_newBranchEdit->text().trimmed();
    if (name.isEmpty()) {
        logError("Please enter a branch name");
        return;
    }
    
    runGitCommand({"branch", name}, m_currentRepoPath);
    m_newBranchEdit->clear();
    populateBranches();
    logSuccess("Created branch: " + name);
}

void VcsEditorModule::onDeleteBranchClicked() {
    QString branch = m_branchCombo->currentText();
    if (branch.isEmpty()) return;
    
    if (confirmAction("Delete Branch", QString("Delete branch '%1'?").arg(branch))) {
        runGitCommand({"branch", "-d", branch}, m_currentRepoPath);
        populateBranches();
        logSuccess("Deleted branch: " + branch);
    }
}

void VcsEditorModule::onCheckoutBranchClicked() {
    QString branch = m_branchCombo->currentText();
    if (branch.isEmpty()) return;
    
    runGitCommand({"checkout", branch}, m_currentRepoPath);
    populateBranches();
    if (m_gitWidget) m_gitWidget->refresh();
    logSuccess("Switched to branch: " + branch);
}

void VcsEditorModule::onMergeBranchClicked() {
    QString branch = m_branchCombo->currentText();
    if (branch.isEmpty()) return;
    
    if (confirmAction("Merge Branch", QString("Merge '%1' into current branch?").arg(branch))) {
        QString output = runGitCommand({"merge", branch}, m_currentRepoPath);
        populateBranches();
        if (m_gitWidget) m_gitWidget->refresh();
        log(output);
    }
}

void VcsEditorModule::onCommitClicked() {
    if (m_gitWidget) m_gitWidget->refresh();
    log("Commit dialog opened");
}

void VcsEditorModule::onStageAllClicked() {
    if (m_currentRepoPath.isEmpty()) return;
    runGitCommand({"add", "-A"}, m_currentRepoPath);
    if (m_gitWidget) m_gitWidget->refresh();
    log("Staged all changes");
}

void VcsEditorModule::onUnstageAllClicked() {
    if (m_currentRepoPath.isEmpty()) return;
    runGitCommand({"reset"}, m_currentRepoPath);
    if (m_gitWidget) m_gitWidget->refresh();
    log("Unstaged all changes");
}

void VcsEditorModule::onDiscardChangesClicked() {
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Discard Changes", 
        "This will discard ALL uncommitted changes. Are you sure?\n\nThis cannot be undone!",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runGitCommand({"checkout", "--", "."}, m_currentRepoPath);
        runGitCommand({"clean", "-fd"}, m_currentRepoPath);
        if (m_gitWidget) m_gitWidget->refresh();
        logSuccess("All changes discarded");
    }
}

void VcsEditorModule::onPullClicked() {
    if (m_currentRepoPath.isEmpty()) {
        logError("No repository loaded");
        return;
    }
    log("Pulling from remote...");
    QString output = runGitCommand({"pull"}, m_currentRepoPath);
    refreshAll();
    log(output);
}

void VcsEditorModule::onPushClicked() {
    if (m_currentRepoPath.isEmpty()) {
        logError("No repository loaded");
        return;
    }
    log("Pushing to remote...");
    QString output = runGitCommand({"push"}, m_currentRepoPath);
    log(output);
}

void VcsEditorModule::onFetchClicked() {
    if (m_currentRepoPath.isEmpty()) {
        logError("No repository loaded");
        return;
    }
    log("Fetching from remote...");
    QString output = runGitCommand({"fetch", "--all"}, m_currentRepoPath);
    populateBranches();
    log(output);
}

void VcsEditorModule::onLogItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    
    QString hash = item->text(0);
    QString fullHash = item->data(0, Qt::UserRole).toString();
    
    if (fullHash.isEmpty()) return;
    
    QString diff = runGitCommand({"show", fullHash, "--stat", "--format=format:%H%n%an <%ae>%n%ai%n%s%n%b"}, m_currentRepoPath);
    if (m_commitDetails) {
        m_commitDetails->clear();
        m_commitDetails->append("=== Commit: " + fullHash + " ===\n");
        m_commitDetails->append(diff);
    }
}

void VcsEditorModule::onRemoteChanged(int index) {
    if (index >= 0 && index < m_remoteCombo->count()) {
        QString text = m_remoteCombo->itemText(index);
        log("Selected remote: " + text);
    }
}

void VcsEditorModule::onAddRemoteClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Remote", "Remote name:", QLineEdit::Normal, "origin", &ok);
    if (!ok || name.isEmpty()) return;
    
    QString url = QInputDialog::getText(this, "Add Remote", "Remote URL:", QLineEdit::Normal, "", &ok);
    if (!ok || url.isEmpty()) return;
    
    runGitCommand({"remote", "add", name, url}, m_currentRepoPath);
    populateRemotes();
    logSuccess("Added remote: " + name + " -> " + url);
}

void VcsEditorModule::onRemoveRemoteClicked() {
    QString remote = m_remoteCombo->currentText();
    if (remote.isEmpty()) return;
    
    if (confirmAction("Remove Remote", QString("Remove remote '%1'?").arg(remote))) {
        runGitCommand({"remote", "remove", remote}, m_currentRepoPath);
        populateRemotes();
        logSuccess("Removed remote: " + remote);
    }
}

void VcsEditorModule::onStashClicked() {
    if (m_currentRepoPath.isEmpty()) return;
    
    bool ok;
    QString msg = QInputDialog::getText(this, "Stash Changes", "Optional message:", QLineEdit::Normal, "", &ok);
    QStringList args = {"stash"};
    if (!msg.isEmpty()) { args << "push" << "-m" << msg; }
    else { args << "save"; }
    
    runGitCommand(args, m_currentRepoPath);
    populateStashes();
    if (m_gitWidget) m_gitWidget->refresh();
    logSuccess("Changes stashed");
}

void VcsEditorModule::onStashPopClicked() {
    if (m_stashesList->currentItem()) {
        runGitCommand({"stash", "pop", "stash@" + QString::number(m_stashesList->currentRow())}, m_currentRepoPath);
        populateStashes();
        if (m_gitWidget) m_gitWidget->refresh();
        log("Stash applied");
    }
}

void VcsEditorModule::onTagClicked() {
    auto* item = m_tagsTree ? m_tagsTree->currentItem() : nullptr;
    if (item) {
        QString tagName = item->text(0);
        log(QString("Tag: %1").arg(tagName));
        QString info = runGitCommand({"tag", "-l", "-n", tagName}, m_currentRepoPath);
        if (!info.isEmpty())
            log("Tag info: " + info.trimmed());
    }
}

void VcsEditorModule::onCreateTagClicked() {
    QString name = m_newTagEdit->text().trimmed();
    if (name.isEmpty()) {
        logError("Please enter a tag name");
        return;
    }
    
    bool ok;
    QString msg = QInputDialog::getText(this, "Tag Message", "Message (optional):", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    
    QStringList args = {"tag", "-a", name};
    if (!msg.isEmpty()) args << "-m" << msg;
    runGitCommand(args, m_currentRepoPath);
    
    m_newTagEdit->clear();
    populateTags();
    logSuccess("Created tag: " + name);
}

void VcsEditorModule::onShowContextMenu(const QPoint& pos) {
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(sender());
    if (!tree || !tree->itemAt(pos)) return;
    
    QMenu menu;
    menu.exec(tree->viewport()->mapToGlobal(pos));
}

void VcsEditorModule::onFileItemClicked(QTreeWidgetItem* item, int column) {
    if (!item) return;
    QString filePath = item->data(0, Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        QString diff = runGitCommand({"diff", "--", filePath}, m_currentRepoPath);
        if (diff.isEmpty())
            diff = runGitCommand({"diff", "--cached", "--", filePath}, m_currentRepoPath);
        if (!diff.isEmpty() && m_commitDetails) {
            m_commitDetails->clear();
            m_commitDetails->append("=== Diff: " + filePath + " ===\n");
            m_commitDetails->append(diff);
        }
    }
}

void VcsEditorModule::loadSettings() {
    QSettings settings;
    settings.beginGroup("VcsEditor");
    
    m_userNameEdit->setText(settings.value("userName", "").toString());
    m_userEmailEdit->setText(settings.value("userEmail", "").toString());
    m_autoFetchCheck->setChecked(settings.value("autoFetch", true).toBool());
    m_fetchIntervalSpin->setValue(settings.value("fetchInterval", 30).toInt());
    m_diffToolCombo->setCurrentIndex(settings.value("diffTool", 0).toInt());
    m_mergeToolCombo->setCurrentIndex(settings.value("mergeTool", 0).toInt());
    
    settings.endGroup();
}

void VcsEditorModule::saveSettings() {
    QSettings settings;
    settings.beginGroup("VcsEditor");
    
    settings.setValue("userName", m_userNameEdit->text());
    settings.setValue("userEmail", m_userEmailEdit->text());
    settings.setValue("autoFetch", m_autoFetchCheck->isChecked());
    settings.setValue("fetchInterval", m_fetchIntervalSpin->value());
    settings.setValue("diffTool", m_diffToolCombo->currentIndex());
    settings.setValue("mergeTool", m_mergeToolCombo->currentIndex());
    
    settings.endGroup();
}

void VcsEditorModule::onActivation() {
    log("Version Control activated");
    if (!m_currentRepoPath.isEmpty()) refreshAll();
}

void VcsEditorModule::onDeactivation() {
    log("Version Control deactivated");
}

} // namespace ks

#include "VcsEditorModule.moc"