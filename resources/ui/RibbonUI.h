#pragma once

#include <QWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QLabel>
#include <QGroupBox>
#include <QFrame>
#include <QButtonGroup>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QEnterEvent>
#include <QColor>

#include "ribbontheme.h"

namespace ks {
namespace editor {

struct RibbonStyle {
    static const int TAB_HEIGHT = 32;
    static const int BUTTON_SIZE = 32;
    static const int PANEL_HEIGHT = 80;
    static const int ICON_SIZE = 24;
};

class RibbonButton : public QToolButton {
    Q_OBJECT
public:
    enum class Style { Normal, Primary, Danger, Success, Warning };
    explicit RibbonButton(QWidget* parent = nullptr);
    RibbonButton(const QString& text, QWidget* parent = nullptr);
    RibbonButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
    void setStyle(Style style);
    void setTextBelow(bool below);
    void setCompact(bool compact);
    void setAction(QAction* action);
    void applyTheme(const RibbonTheme& theme);
protected:
    void init();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
private:
    void resolveColors(QColor& background, QColor& foreground) const;
    Style m_style = Style::Normal;
    bool m_textBelow = true;
    bool m_compact = false;
    QAction* m_action = nullptr;
    bool hovering = false;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonGroup : public QFrame {
    Q_OBJECT
public:
    explicit RibbonGroup(QWidget* parent = nullptr);
    RibbonGroup(const QString& title, QWidget* parent = nullptr);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    RibbonButton* addButton(const QString& text);
    RibbonButton* addButton(const QIcon& icon, const QString& text);
    void addWidget(QWidget* widget);
    void addSeparator();
    void addStretch();
    void applyTheme(const RibbonTheme& theme);
    const QList<QWidget*>& widgets() const { return m_widgets; }
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QString m_title;
    QList<QWidget*> m_widgets;
    QHBoxLayout* m_layout = nullptr;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonPanel : public QFrame {
    Q_OBJECT
public:
    explicit RibbonPanel(QWidget* parent = nullptr);
    RibbonPanel(const QString& title, QWidget* parent = nullptr);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    RibbonGroup* addGroup(const QString& title);
    void addGroup(RibbonGroup* group);
    void addStretch();
    void applyTheme(const RibbonTheme& theme);
    const QList<RibbonGroup*>& groups() const { return m_groups; }
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QString m_title;
    QList<RibbonGroup*> m_groups;
    QHBoxLayout* m_layout = nullptr;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonSubTab : public QWidget {
    Q_OBJECT
public:
    explicit RibbonSubTab(QWidget* parent = nullptr);
    RibbonSubTab(const QString& title, const QIcon& icon, QWidget* parent = nullptr);
    ~RibbonSubTab() override;
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    void setIcon(const QIcon& icon);
    QIcon icon() const { return m_icon; }
    RibbonPanel* addPanel(const QString& title);
    void addPanel(RibbonPanel* panel);
    void insertPanel(int index, RibbonPanel* panel);
    const QList<RibbonPanel*>& panels() const { return m_panels; }
    int panelCount() const { return m_panels.size(); }
    void setActive(bool active);
    bool isActive() const { return m_active; }
    void setTabColor(const QColor& color);
    void applyTheme(const RibbonTheme& theme);
    const RibbonTheme* theme() const { return m_theme; }
signals:
    void activated();
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void updateScrollArea();
    void adjustPanelHeights();
    QString m_title;
    QIcon m_icon;
    bool m_active = false;
    QColor m_accentColor;
    QColor m_tabColor;
    QList<RibbonPanel*> m_panels;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QHBoxLayout* m_contentLayout = nullptr;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonSubTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit RibbonSubTabBar(QWidget* parent = nullptr);
    void setTabIcon(int index, const QIcon& icon);
    void applyTheme(const RibbonTheme& theme);
signals:
    void currentChanged(int index);
    void subTabAdded(int index);
    void subTabRemoved(int index);
protected:
    void paintEvent(QPaintEvent* event) override;
};

class RibbonTab : public QWidget {
    Q_OBJECT
public:
    explicit RibbonTab(QWidget* parent = nullptr);
    RibbonTab(const QString& title, QWidget* parent = nullptr);
    RibbonTab(const QString& title, const QIcon& icon, QWidget* parent = nullptr);
    ~RibbonTab() override;
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    void setIcon(const QIcon& icon);
    QIcon icon() const { return m_icon; }
    RibbonSubTabBar* subTabBar() const { return m_subTabBar; }
    RibbonSubTab* addSubTab(const QString& title, const QIcon& icon = QIcon());
    void insertSubTab(int index, RibbonSubTab* subTab);
    void removeSubTab(int index);
    RibbonSubTab* subTab(int index) const;
    int subTabCount() const;
    int currentSubTabIndex() const;
    void setCurrentSubTabIndex(int index);
    RibbonPanel* addPanel(const QString& title);
    void addPanel(RibbonPanel* panel);
    void insertPanel(int index, RibbonPanel* panel);
    const QList<RibbonPanel*>& panels() const { return m_panels; }
    int panelCount() const { return m_panels.size(); }
    virtual void refresh() {}
    void setTabColor(const QColor& color);
    void applyTheme(const RibbonTheme& theme);
    const RibbonTheme* theme() const { return m_theme; }
signals:
    void tabChanged();
    void subTabChanged(int index);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void updateScrollArea();
    void adjustPanelHeights();
    void onSubTabChanged(int index);
    QString m_title;
    QIcon m_icon;
    QColor m_tabColor;
    QList<RibbonPanel*> m_panels;
    QList<RibbonSubTab*> m_subTabs;
    RibbonSubTabBar* m_subTabBar = nullptr;
    QStackedWidget* m_subTabStack = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QHBoxLayout* m_contentLayout = nullptr;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonBar : public QWidget {
    Q_OBJECT
public:
    explicit RibbonBar(QWidget* parent = nullptr);
    ~RibbonBar() override;
    void addTab(RibbonTab* tab);
    void insertTab(int index, RibbonTab* tab);
    void removeTab(int index);
    RibbonTab* tab(int index) const;
    RibbonTab* currentTab() const;
    int currentIndex() const;
    int tabCount() const;
    void setCurrentIndex(int index);
    void setCurrentTab(RibbonTab* tab);
    void applyTheme(const QString& themeKey);
    void setTabHighlight(int index, bool highlight);
    void setTabIcon(int index, const QIcon& icon);
signals:
    void currentChanged(int index);
    void tabAdded(int index);
    void tabRemoved(int index);
    void themeChanged(const QString& themeKey);
private slots:
    void onTabClicked(int index);
private:
    void updateLayout();
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    const RibbonTheme* m_theme = nullptr;
};

class RibbonWindowManager : public QObject {
    Q_OBJECT
public:
    enum class Mode { Common, Car, Track, Character };
    explicit RibbonWindowManager(QWidget* parent = nullptr);
    ~RibbonWindowManager() override;
    RibbonBar* ribbonBar() const { return m_ribbonBar; }
    Mode mode() const { return m_mode; }
    void setMode(Mode mode);
signals:
    void modeChanged(Mode mode);
private slots:
    void onModeChanged(int index);
private:
    void setupRibbon();
    void updateVisibility();
    RibbonBar* m_ribbonBar = nullptr;
    Mode m_mode = Mode::Common;
};

} // namespace editor
} // namespace ks