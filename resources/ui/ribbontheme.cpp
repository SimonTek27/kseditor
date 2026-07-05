#include "ribbontheme.h"

#ifdef Q_OS_WIN
#include <QWindow>
#include <QOperatingSystemVersion>
#endif

namespace ks {
namespace editor {

RibbonThemeManager::RibbonThemeManager() {
    m_themes["car"] = carTheme();
    m_themes["track"] = trackTheme();
    m_themes["character"] = characterTheme();
    m_themes["showroom"] = showroomTheme();
    m_themes["sound"] = soundTheme();
    m_themes["font"] = fontTheme();
    m_currentTheme = "car";
    m_fallback = m_themes.value("car");
}

RibbonThemeManager& RibbonThemeManager::instance() {
    static RibbonThemeManager mgr;
    return mgr;
}

void RibbonThemeManager::registerTheme(const QString& key, const RibbonTheme& theme) {
    m_themes[key] = theme;
}

const RibbonTheme& RibbonThemeManager::theme(const QString& key) const {
    auto it = m_themes.find(key);
    if (it != m_themes.end()) return it.value();
    return m_fallback;
}

QStringList RibbonThemeManager::themeKeys() const {
    return m_themes.keys();
}

QString RibbonThemeManager::currentTheme() const {
    return m_currentTheme;
}

void RibbonThemeManager::applyTheme(const QString& key) {
    if (m_themes.contains(key)) {
        m_currentTheme = key;
        QString style = generateStyleSheet(m_themes[key]);
        qApp->setStyleSheet(style);
    }
}

void RibbonThemeManager::applyWindowFrame(QWidget* window, const QString& key) {
    if (m_themes.contains(key)) {
        QString style = generateRibbonStyleSheet(m_themes[key]);
        window->setStyleSheet(style);
    }
}

void RibbonThemeManager::applyWindowFrame(QMainWindow* window, const QString& themeKey) {
    const RibbonTheme& t = theme(themeKey);

#ifdef Q_OS_WIN
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        HWND hwnd = reinterpret_cast<HWND>(window->winId());

        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                              &darkMode, sizeof(darkMode));

        COLORREF color = RGB(t.windowBorder.red(),
                             t.windowBorder.green(),
                             t.windowBorder.blue());
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));
    }
#endif

    if (window) {
        window->setStyleSheet(QString(R"(
            QMainWindow {
                background: %1;
                border: 2px solid %2;
                border-radius: 0px;
            }
        )").arg(t.centralBg.name(), t.windowBorder.name()));
    }
}

RibbonTheme RibbonThemeManager::carTheme() {
    RibbonTheme t;
    t.name = "Car Editor";
    t.primary = QColor("#007acc");
    t.primaryDark = QColor("#005a9e");
    t.accent = QColor("#0098ff");
    t.background = QColor("#252526");
    t.titleBarBg = QColor("#1e1e1e");
    t.titleBarText = QColor("#cccccc");
    t.panelBg = QColor("#2d2d30");
    t.groupLabel = QColor("#9d9d9d");
    t.buttonHover = QColor("#3e3e42");
    t.buttonPressed = QColor("#007acc");
    t.borderColor = QColor("#3e3e42");
    t.windowBorder = QColor("#3c3c3c");
    t.statusBarBg = QColor("#007acc");
    t.statusBarText = QColor("#ffffff");
    t.dockTitleBg = QColor("#2d2d30");
    t.dockTitleText = QColor("#cccccc");
    t.centralBg = QColor("#1e1e1e");
    return t;
}

RibbonTheme RibbonThemeManager::trackTheme() {
    RibbonTheme t = carTheme();
    t.name = "Track Editor";
    t.primary = QColor("#28a745");
    t.primaryDark = QColor("#1e7e34");
    t.accent = QColor("#20c997");
    return t;
}

RibbonTheme RibbonThemeManager::characterTheme() {
    RibbonTheme t = carTheme();
    t.name = "Character Editor";
    t.primary = QColor("#6f42c1");
    t.primaryDark = QColor("#553d7a");
    t.accent = QColor("#8375e3");
    return t;
}

RibbonTheme RibbonThemeManager::showroomTheme() {
    RibbonTheme t = carTheme();
    t.name = "Showroom";
    t.primary = QColor("#17a2b8");
    t.primaryDark = QColor("#117a8b");
    t.accent = QColor("#20c997");
    return t;
}

RibbonTheme RibbonThemeManager::soundTheme() {
    RibbonTheme t = carTheme();
    t.name = "Sound Editor";
    t.primary = QColor("#fd7e14");
    t.primaryDark = QColor("#d96b0a");
    t.accent = QColor("#ffc107");
    return t;
}

RibbonTheme RibbonThemeManager::fontTheme() {
    RibbonTheme t = carTheme();
    t.name = "Font Editor";
    t.primary = QColor("#e83e8c");
    t.primaryDark = QColor("#c22569");
    t.accent = QColor("#f175a8");
    return t;
}

QString RibbonThemeManager::generateStyleSheet(const RibbonTheme& theme) const {
    QString style = "QMainWindow { background: %1; color: %2; } "
        "QWidget { background: %1; color: %2; } "
        "QTabBar::tab { background: %3; color: %4; padding: 8px; } "
        "QTabBar::tab:selected { background: %5; color: %6; } "
        "QPushButton { background: %3; color: %2; border: 1px solid %7; padding: 6px 12px; } "
        "QPushButton:hover { background: %8; } "
        "QPushButton:pressed { background: %9; } "
        "QLineEdit, QComboBox { background: %3; color: %2; border: 1px solid %7; } "
        "QGroupBox { color: %4; } "
        "QScrollBar:vertical { background: %3; width: 10px; } "
        "QScrollBar::handle:vertical { background: %7; border-radius: 4px; } "
        "QStatusBar { background: %10; color: %11; } "
        "QDockWidget::title { background: %12; color: %13; }";

    return style
        .arg(theme.centralBg.name())
        .arg(theme.titleBarText.name())
        .arg(theme.background.name())
        .arg(theme.groupLabel.name())
        .arg(theme.primary.name())
        .arg(theme.accent.name())
        .arg(theme.borderColor.name())
        .arg(theme.buttonHover.name())
        .arg(theme.buttonPressed.name())
        .arg(theme.statusBarBg.name())
        .arg(theme.statusBarText.name())
        .arg(theme.dockTitleBg.name())
        .arg(theme.dockTitleText.name());
}

QString RibbonThemeManager::generateRibbonStyleSheet(const RibbonTheme& theme) const {
    QString style = "QMainWindow { background: %1; border: 1px solid %2; }";
    return style
        .arg(theme.centralBg.name())
        .arg(theme.windowBorder.name());
}

} // namespace editor
} // namespace ks
