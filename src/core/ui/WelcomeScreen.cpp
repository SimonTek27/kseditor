#include "WelcomeScreen.h"
#include "core/help/HelpSystem.h"
#include "core/help/HelpBrowser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QCursor>
#include <QDebug>
#include <QIcon>
#include <QMouseEvent>
#include <QCollator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static QString getWelcomePath() { return ":/assets/welcome.png"; }

// Returns false instantly for drives that are disconnected, empty (no media)
// or otherwise unavailable, avoiding the multi-second hang QDir::exists()
// would suffer on such paths (e.g. an empty optical/removable drive D:).
static bool isPathDriveReady(const QString& path) {
#ifdef Q_OS_WIN
    if (path.length() >= 2 && path[1] == QChar(':')) {
        const QString root = path.left(2) + "\\";
        const UINT type = GetDriveTypeW(root.toStdWString().c_str());
        if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_UNKNOWN)
            return false;
    }
#endif
    return true;
}

WelcomeScreen::WelcomeScreen(QWidget* parent) : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
    setWindowTitle("ksEditor 0.9.0");
    setMinimumSize(600, 550);
    setupUI();
    populateRecentProjects();
}

void WelcomeScreen::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= 36) {
        m_dragPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void WelcomeScreen::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && !m_dragPos.isNull()) {
        move(event->globalPos() - m_dragPos);
    }
    QDialog::mouseMoveEvent(event);
}

void WelcomeScreen::mouseReleaseEvent(QMouseEvent* event) {
    m_dragPos = QPoint();
    QDialog::mouseReleaseEvent(event);
}

void WelcomeScreen::onNewClicked() { selectedAction = New; accept(); }
void WelcomeScreen::onNewBlankClicked() { selectedAction = NewBlank; accept(); }
void WelcomeScreen::onOpenClicked() { selectedAction = Open; accept(); }
void WelcomeScreen::onHelpClicked() {
    ks::HelpSystem::instance()->showHelp();
}

void WelcomeScreen::onRecentDoubleClicked(QListWidgetItem* item) {
    selectedAction = Recent;
    recentPath = item->data(Qt::UserRole).toString();
    accept();
}

void WelcomeScreen::onRecentContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_recentList->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("Delete Project");
    connect(deleteAction, &QAction::triggered, this, [this, item]() {
        QString path = item->data(Qt::UserRole).toString();
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            "Delete Project", "Delete this project?\n\nPath: " + path,
            QMessageBox::Yes | QMessageBox::Cancel);
        if (btn == QMessageBox::Yes) {
            if (QDir(path).exists()) QDir(path).removeRecursively();
            delete item;
            updateProjectCount();
        }
    });
    menu.exec(QCursor::pos());
}

void WelcomeScreen::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* titleBar = new QWidget();
    titleBar->setStyleSheet("background: #1e1e1e;");
    titleBar->setFixedHeight(36);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel* title = new QLabel("Welcome to ksEditor");
    title->setStyleSheet("color: #00ffcc; font-weight: bold;");
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    QPushButton* settingsBtn = new QPushButton();
    settingsBtn->setFixedSize(36, 28);
    settingsBtn->setIcon(QIcon(":/icons/settings.svg"));
    settingsBtn->setIconSize(QSize(18, 18));
    settingsBtn->setStyleSheet("background: transparent; border: none; color: #cccccc;");
    settingsBtn->setToolTip("Settings");
    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        ks::HelpSystem::instance()->showHelp();
    });
    titleLayout->addWidget(settingsBtn);

    QPushButton* minBtn = new QPushButton();
    minBtn->setFixedSize(36, 28);
    minBtn->setIcon(QIcon(":/icons/window-minimize.svg"));
    minBtn->setIconSize(QSize(14, 14));
    minBtn->setStyleSheet("background: transparent; border: none; border-radius: 4px;");
    connect(minBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minBtn);

    QPushButton* closeBtn = new QPushButton();
    closeBtn->setFixedSize(36, 28);
    closeBtn->setIcon(QIcon(":/icons/window-close.svg"));
    closeBtn->setIconSize(QSize(14, 14));
    closeBtn->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background-color: #c0392b; }");
    closeBtn->setObjectName("welcomeCloseBtn");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(closeBtn);

    mainLayout->addWidget(titleBar);

    QPixmap welcomePix(getWelcomePath());
    if (welcomePix.isNull()) {
        welcomePix = QPixmap(600, 200);
        welcomePix.fill(QColor(30, 30, 32));
        QPainter p(&welcomePix);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 24, QFont::Bold));
        p.drawText(welcomePix.rect(), Qt::AlignCenter, "ksEditor 1.16");
        p.end();
    }

    QLabel* header = new QLabel();
    header->setPixmap(welcomePix.scaled(600, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    header->setStyleSheet("background: #1e1e1e; padding: 15px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QWidget* content = new QWidget();
    content->setStyleSheet("background: #252526;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 25, 40, 25);

    QLabel* subtitle = new QLabel("Assetto Corsa Modding Suite");
    subtitle->setStyleSheet("color: #aaaaaa; font-size: 14px;");
    subtitle->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(subtitle);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();

    QPushButton* newBtn = new QPushButton("New Project");
    newBtn->setMinimumWidth(140);
    newBtn->setIcon(QIcon(":/icons/document-new.svg"));
    newBtn->setIconSize(QSize(18, 18));
    newBtn->setStyleSheet("QPushButton { background: #007acc; color: white; border: none; padding: 12px 20px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #005a9e; }");
    connect(newBtn, &QPushButton::clicked, this, &WelcomeScreen::onNewClicked);
    buttonRow->addWidget(newBtn);

    QPushButton* newBlankBtn = new QPushButton("New Project (blank)");
    newBlankBtn->setMinimumWidth(160);
    newBlankBtn->setIcon(QIcon(":/icons/add.svg"));
    newBlankBtn->setIconSize(QSize(18, 18));
    newBlankBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 12px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(newBlankBtn, &QPushButton::clicked, this, &WelcomeScreen::onNewBlankClicked);
    buttonRow->addWidget(newBlankBtn);

    QPushButton* openBtn = new QPushButton("Open...");
    openBtn->setMinimumWidth(140);
    openBtn->setIcon(QIcon(":/icons/document-open.svg"));
    openBtn->setIconSize(QSize(18, 18));
    openBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 12px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(openBtn, &QPushButton::clicked, this, &WelcomeScreen::onOpenClicked);
    buttonRow->addWidget(openBtn);

    QPushButton* helpBtn = new QPushButton("Help");
    helpBtn->setMinimumWidth(140);
    helpBtn->setIcon(QIcon(":/icons/help.svg"));
    helpBtn->setIconSize(QSize(18, 18));
    helpBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 12px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(helpBtn, &QPushButton::clicked, this, &WelcomeScreen::onHelpClicked);
    buttonRow->addWidget(helpBtn);

    buttonRow->addStretch();
    contentLayout->addLayout(buttonRow);

    contentLayout->addSpacing(15);

    QLabel* recentLabel = new QLabel("Recent Projects");
    recentLabel->setStyleSheet("color: #007acc; font-weight: bold; font-size: 13px;");
    contentLayout->addWidget(recentLabel);

    m_recentList = new QListWidget();
    m_recentList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_recentList->setStyleSheet("QListWidget { background: #1e1e1e; color: #cccccc; border: 1px solid #3e3e42; } QListWidget::item { padding: 10px; border-bottom: 1px solid #2d2d30; } QListWidget::item:selected { background: #094771; } QListWidget::item:hover { background: #2a2d2e; }");
    connect(m_recentList, &QListWidget::itemDoubleClicked, this, &WelcomeScreen::onRecentDoubleClicked);
    connect(m_recentList, &QListWidget::customContextMenuRequested, this, &WelcomeScreen::onRecentContextMenu);
    contentLayout->addWidget(m_recentList);

    contentLayout->addSpacing(10);

    QWidget* statusBar = new QWidget();
    statusBar->setStyleSheet("background: #1e1e1e; padding: 8px;");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(10, 0, 10, 0);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #888888; font-size: 11px;");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();

    m_countLabel = new QLabel();
    m_countLabel->setStyleSheet("color: #666666; font-size: 11px;");
    statusLayout->addWidget(m_countLabel);

    contentLayout->addWidget(statusBar);
    mainLayout->addWidget(content);
}

void WelcomeScreen::populateRecentProjects() {
    m_recentList->clear();

    QStringList searchPaths = {
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ksProjects",
        QCoreApplication::applicationDirPath() + "/../moddev/content",
        QCoreApplication::applicationDirPath() + "/moddev/content"
    };

    QStringList steamPaths = {
        "C:/Program Files/Steam/steamapps/core/assettocorsa/moddev/content",
        "D:/SteamLibrary/steamapps/core/assettocorsa/moddev/content",
        "E:/SteamLibrary/steamapps/core/assettocorsa/moddev/content"
    };
    searchPaths.append(steamPaths);

    QMap<QString, QString> entries;
    for (const QString& basePath : searchPaths) {
        if (!isPathDriveReady(basePath))
            continue;
        QDir baseDir(basePath);
        if (baseDir.exists()) {
            QStringList subfolders = {"cars", "tracks", "drivers", "objects3d", "showroom", "fonts"};
            for (const QString& sub : subfolders) {
                QDir subDir(basePath + "/" + sub);
                if (subDir.exists()) {
                    for (const QString& project : subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                        QString display = sub + "/" + project;
                        if (!entries.contains(display))
                            entries.insert(display, subDir.absoluteFilePath(project));
                    }
                }
            }
        }
    }

    QCollator collator;
    collator.setNumericMode(true);
    QStringList sorted = entries.keys();
    std::sort(sorted.begin(), sorted.end(), [&](const QString& a, const QString& b) {
        return collator.compare(a, b) < 0;
    });

    const int maxProjects = 50;
    int count = 0;
    for (const QString& display : sorted) {
        if (count >= maxProjects)
            break;
        QListWidgetItem* item = new QListWidgetItem(display, m_recentList);
        item->setData(Qt::UserRole, entries.value(display));
        item->setToolTip(entries.value(display));
        ++count;
    }

    updateProjectCount();
}

void WelcomeScreen::updateProjectCount() {
    int count = m_recentList->count();
    m_countLabel->setText(QString::number(count) + " project" + (count == 1 ? "" : "s"));
}
