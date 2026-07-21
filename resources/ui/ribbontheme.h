#pragma once

#include <QColor>
#include <QString>
#include <QMap>
#include <QApplication>
#include <QPalette>
#include <QWidget>
#include <QMainWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#endif

namespace ks {
    namespace editor {

        struct RibbonTheme {
            QString name;
            QColor primary;          // Main tab color
            QColor primaryDark;      // Darker version for hover/pressed
            QColor accent;           // Accent for active elements
            QColor background;       // Ribbon background
            QColor titleBarBg;       // Title bar background
            QColor titleBarText;     // Title bar text
            QColor panelBg;          // Panel background
            QColor groupLabel;       // Group label text
            QColor buttonHover;      // Button hover state
            QColor buttonPressed;    // Button pressed state
            QColor borderColor;      // Subtle borders
            QColor windowBorder;     // Window frame border color
            QColor statusBarBg;      // Status bar background
            QColor statusBarText;    // Status bar text
            QColor dockTitleBg;      // Dock widget title background
            QColor dockTitleText;    // Dock widget title text
            QColor centralBg;        // Central widget background
        };

        class RibbonThemeManager {
        public:
            static RibbonThemeManager& instance();

            void registerTheme(const QString& key, const RibbonTheme& theme);
            const RibbonTheme& theme(const QString& key) const;
            QStringList themeKeys() const;
            QString currentTheme() const;

            void applyTheme(const QString& key);
            void applyWindowFrame(QWidget* window, const QString& key);
            void applyWindowFrame(QMainWindow* window, const QString& themeKey);

            // Predefined themes
            static RibbonTheme carTheme();
            static RibbonTheme trackTheme();
            static RibbonTheme characterTheme();
            static RibbonTheme showroomTheme();
            static RibbonTheme soundTheme();
            static RibbonTheme fontTheme();
            static RibbonTheme paintTheme();

        private:
            RibbonThemeManager();
            QMap<QString, RibbonTheme> m_themes;
            QString m_currentTheme;
            RibbonTheme m_fallback;

            QString generateStyleSheet(const RibbonTheme& theme) const;
            QString generateRibbonStyleSheet(const RibbonTheme& theme) const;
        };

    } // namespace editor
} // namespace ks
