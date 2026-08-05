#include "WelcomeScreen.h"
#include "core/help/HelpSystem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QCoreApplication>
#include <QIcon>
#include <QMouseEvent>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>

static QString getWelcomePath() { return ":/assets/welcome.png"; }

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_settings(new QSettings("ksEditor", "ksEditor", this))
{
    setWindowTitle("ksEditor 0.9.0");
    setWindowIcon(QIcon(":/icons/modeler.svg"));
    setMinimumSize(600, 600);
    setupUI();
    loadRecentProjects();
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

void WelcomeScreen::onHelpClicked() {
    ks::HelpSystem::instance()->showHelp();
}

void WelcomeScreen::onRecentItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty() || !QFileInfo(path).exists()) return;
    recentProjectPath = path;
    accept();
}

QIcon WelcomeScreen::makeWhiteIcon(const QString& resourcePath, int size) {
    QPixmap original(resourcePath);
    if (original.isNull()) return QIcon(resourcePath);

    QPixmap colored(size, size);
    colored.fill(Qt::transparent);

    QPainter painter(&colored);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, size, size, original);

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(0, 0, size, size, Qt::white);
    painter.end();

    return QIcon(colored);
}

void WelcomeScreen::loadRecentProjects() {
    if (!m_recentList) return;
    m_recentList->clear();

    QStringList recent = m_settings->value("recentProjects").toStringList();

    for (const QString& path : recent) {
        QFileInfo info(path);
        if (!info.exists()) continue;

        QString displayName = info.fileName();
        QDir parentDir = info.dir();
        QString parentName = parentDir.dirName();
        if (parentName == "cars") displayName += "  [Car]";
        else if (parentName == "tracks") displayName += "  [Track]";
        else if (parentName == "driver") displayName += "  [Character]";
        else if (parentName == "sfx") displayName += "  [Sound]";

        auto* item = new QListWidgetItem(displayName, m_recentList);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        if (path.contains("/cars/", Qt::CaseInsensitive))
            item->setIcon(makeWhiteIcon(":/icons/car-model.svg", 16));
        else if (path.contains("/tracks/", Qt::CaseInsensitive))
            item->setIcon(makeWhiteIcon(":/icons/track.svg", 16));
        else if (path.contains("/driver/", Qt::CaseInsensitive))
            item->setIcon(makeWhiteIcon(":/icons/character.svg", 16));
        else if (path.contains("/sfx/", Qt::CaseInsensitive))
            item->setIcon(makeWhiteIcon(":/icons/audio.svg", 16));
        else
            item->setIcon(makeWhiteIcon(":/icons/folder-open.svg", 16));
    }

    if (m_recentList->count() == 0) {
        auto* item = new QListWidgetItem("No recent projects", m_recentList);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor("#71717a"));
    }
}

void WelcomeScreen::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ── Title Bar ────────────────────────────────────────────────────────────
    QWidget* titleBar = new QWidget();
    titleBar->setStyleSheet("background: #1a1a1e;");
    titleBar->setFixedHeight(36);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel* title = new QLabel("Welcome to ksEditor");
    title->setStyleSheet("color: #00ffcc; font-weight: bold;");
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    QPushButton* settingsBtn = new QPushButton();
    settingsBtn->setFixedSize(36, 28);
    settingsBtn->setIcon(makeWhiteIcon(":/icons/settings.svg", 18));
    settingsBtn->setIconSize(QSize(18, 18));
    settingsBtn->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3e3e42; border-radius: 4px; }");
    settingsBtn->setToolTip("Settings");
    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        ks::HelpSystem::instance()->showHelp();
    });
    titleLayout->addWidget(settingsBtn);

    QPushButton* titleHelpBtn = new QPushButton("?");
    titleHelpBtn->setFixedSize(36, 28);
    QFont helpFont = titleHelpBtn->font();
    helpFont.setBold(true);
    titleHelpBtn->setFont(helpFont);
    titleHelpBtn->setStyleSheet("QPushButton { background: transparent; border: none; color: #ccc; } QPushButton:hover { background: #3e3e42; border-radius: 4px; }");
    titleHelpBtn->setToolTip("Help");
    titleHelpBtn->setObjectName("welcomeHelpBtn");
    connect(titleHelpBtn, &QPushButton::clicked, this, &WelcomeScreen::onHelpClicked);
    titleLayout->addWidget(titleHelpBtn);

    QPushButton* minBtn = new QPushButton();
    minBtn->setFixedSize(36, 28);
    minBtn->setIcon(makeWhiteIcon(":/icons/window-minimize.svg", 14));
    minBtn->setIconSize(QSize(14, 14));
    minBtn->setStyleSheet("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #3e3e42; border-radius: 4px; }");
    minBtn->setToolTip("Minimize");
    connect(minBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minBtn);

    QPushButton* closeBtn = new QPushButton();
    closeBtn->setFixedSize(36, 28);
    closeBtn->setIcon(makeWhiteIcon(":/icons/window-close.svg", 14));
    closeBtn->setIconSize(QSize(14, 14));
    closeBtn->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background-color: #c0392b; }");
    closeBtn->setToolTip("Close");
    closeBtn->setObjectName("welcomeCloseBtn");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(closeBtn);

    mainLayout->addWidget(titleBar);

    // ── Banner ───────────────────────────────────────────────────────────────
    QPixmap welcomePix(getWelcomePath());
    if (welcomePix.isNull()) {
        welcomePix = QPixmap(600, 180);
        welcomePix.fill(QColor(26, 26, 30));
        QPainter p(&welcomePix);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 24, QFont::Bold));
        p.drawText(welcomePix.rect(), Qt::AlignCenter, "ksEditor 1.16");
        p.end();
    }

    QLabel* header = new QLabel();
    header->setPixmap(welcomePix.scaled(600, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    header->setStyleSheet("background: #1a1a1e; padding: 15px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    // ── Content ──────────────────────────────────────────────────────────────
    QWidget* content = new QWidget();
    content->setStyleSheet("background: #252526;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 20, 40, 20);

    QLabel* subtitle = new QLabel("Assetto Corsa Modding Suite");
    subtitle->setStyleSheet("color: #aaaaaa; font-size: 14px;");
    subtitle->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(subtitle);

    // ── Suite ─────────────────────────────────────────────────────────────────
    QLabel* suiteLabel = new QLabel("SUITE");
    suiteLabel->setStyleSheet("color: #888; font-size: 11px; font-weight: bold; letter-spacing: 2px;");
    suiteLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(suiteLabel);

    contentLayout->addSpacing(8);

    auto* suiteGrid = new QGridLayout();
    suiteGrid->setSpacing(10);
    suiteGrid->setAlignment(Qt::AlignCenter);

    struct SuiteApp {
        const char* icon;
        const char* label;
        const char* tooltip;
        const char* mode;
    };
    const SuiteApp apps[] = {
        { ":/icons/modeler.svg",      "3D",               "Open 3D Modeler",      "modeler" },
        { ":/icons/livery.svg",       "Livery",           "Livery Editor",        "" },
        { ":/icons/licenseplate.svg", "License\nPlate",    "License Plate Editor", "" },
        { ":/icons/font.svg",         "Font\nCreator",     "Font Creator",         "font" },
        { ":/icons/display.svg",      "Display",          "Display Editor",       "" },
        { ":/icons/physics.svg",      "Physics",          "Physics Editor",       "physics" },
        { ":/icons/sound.svg",        "Audio\nStudio",    "Audio Studio",         "audiostudio" },
        { ":/icons/waveform.svg",     "Audio\nEditor",    "Audio Editor",         "audioeditor" },
        { ":/icons/preview.svg",      "Preview\nGenerator", "Preview Generator",  "" }
    };
    const int nApps = sizeof(apps) / sizeof(apps[0]);
    const int cols = 3;

    for (int i = 0; i < nApps; ++i) {
        auto* btn = new QPushButton();
        btn->setFixedSize(90, 90);
        btn->setIcon(makeWhiteIcon(apps[i].icon, 32));
        btn->setIconSize(QSize(32, 32));
        QString mode = apps[i].mode;
        btn->setToolTip(QString(apps[i].tooltip));
        btn->setStyleSheet(
            "QPushButton {"
            "  background: #2a2a2e; border: 1px solid #3f3f46; border-radius: 6px;"
            "  color: white; font-size: 10px;"
            "}"
            "QPushButton:hover { background: #3e3e42; border-color: #555; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, mode]() {
            if (!mode.isEmpty())
                launchApp(mode);
        });
        auto* btnLayout = new QVBoxLayout(btn);
        btnLayout->setContentsMargins(4, 8, 4, 8);
        btnLayout->setSpacing(4);
        btnLayout->setAlignment(Qt::AlignCenter);
        auto* iconLabel = new QLabel();
        iconLabel->setPixmap(btn->icon().pixmap(32, 32));
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        btnLayout->addWidget(iconLabel);
        auto* textLabel = new QLabel(apps[i].label);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        textLabel->setStyleSheet("color: white; font-size: 10px; background: transparent; border: none;");
        btnLayout->addWidget(textLabel);

        suiteGrid->addWidget(btn, i / cols, i % cols, Qt::AlignCenter);
    }

    contentLayout->addLayout(suiteGrid);

    // ── Recent Projects ──────────────────────────────────────────────────────
    contentLayout->addSpacing(16);

    QLabel* recentLabel = new QLabel("RECENT PROJECTS");
    recentLabel->setStyleSheet("color: #888; font-size: 11px; font-weight: bold; letter-spacing: 2px;");
    recentLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(recentLabel);

    contentLayout->addSpacing(8);

    m_recentList = new QListWidget(this);
    m_recentList->setObjectName("welcomeRecentList");
    m_recentList->setMinimumHeight(100);
    m_recentList->setMaximumHeight(150);
    m_recentList->setStyleSheet(
        "QListWidget {"
        "  background: #1a1a1e; color: #d4d4d8;"
        "  border: 1px solid #3f3f46; border-radius: 6px;"
        "  outline: none; font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-bottom: 1px solid #2a2a2e;"
        "}"
        "QListWidget::item:selected {"
        "  background: #3b82f6; color: white;"
        "}"
        "QListWidget::item:hover {"
        "  background: #2a2a2e;"
        "}"
    );
    connect(m_recentList, &QListWidget::itemDoubleClicked, this, &WelcomeScreen::onRecentItemDoubleClicked);
    contentLayout->addWidget(m_recentList);

    // ── Help Button ──────────────────────────────────────────────────────────
    contentLayout->addSpacing(16);

    QPushButton* helpBtn = new QPushButton("Help");
    helpBtn->setFixedWidth(120);
    helpBtn->setIcon(makeWhiteIcon(":/icons/help.svg", 18));
    helpBtn->setIconSize(QSize(18, 18));
    helpBtn->setToolTip("Open Help");
    helpBtn->setStyleSheet(
        "QPushButton { background: #3e3e42; color: white; border: 1px solid #555;"
        "  padding: 10px 20px; border-radius: 4px; }"
        "QPushButton:hover { background: #4e4e52; }"
    );
    connect(helpBtn, &QPushButton::clicked, this, &WelcomeScreen::onHelpClicked);

    QHBoxLayout* helpRow = new QHBoxLayout();
    helpRow->addStretch();
    helpRow->addWidget(helpBtn);
    helpRow->addStretch();
    contentLayout->addLayout(helpRow);

    contentLayout->addStretch();
    mainLayout->addWidget(content);
}

void WelcomeScreen::launchApp(const QString& mode) {
    launchMode = mode;
    accept();
}
