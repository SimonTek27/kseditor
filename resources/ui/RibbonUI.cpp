#include "RibbonUI.h"
#include "ribbontheme.h"

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
#include <QPainter>
#include <QMouseEvent>
#include <QSequentialAnimationGroup>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QProxyStyle>

namespace ks {
namespace editor {

// ==================== RibbonButton ====================

RibbonButton::RibbonButton(QWidget* parent)
    : QToolButton(parent)
{
    init();
}

RibbonButton::RibbonButton(const QString& text, QWidget* parent)
    : QToolButton(parent)
{
    setText(text);
    init();
}

RibbonButton::RibbonButton(const QIcon& icon, const QString& text, QWidget* parent)
    : QToolButton(parent)
{
    setIcon(icon);
    setText(text);
    init();
}

void RibbonButton::init() {
    setIconSize(QSize(RibbonStyle::ICON_SIZE, RibbonStyle::ICON_SIZE));
    setMinimumSize(RibbonStyle::BUTTON_SIZE, RibbonStyle::BUTTON_SIZE);
    setMouseTracking(true);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void RibbonButton::setStyle(Style style) {
    m_style = style;
    update();
}

void RibbonButton::setTextBelow(bool below) {
    m_textBelow = below;
    setToolButtonStyle(below ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonTextBesideIcon);
    update();
}

void RibbonButton::setCompact(bool compact) {
    m_compact = compact;
    update();
}

void RibbonButton::setAction(QAction* action) {
    m_action = action;
    if (action) {
        setText(action->text());
        setIcon(action->icon());
        setToolTip(action->toolTip());
        connect(this, &QToolButton::clicked, action, &QAction::trigger);
    }
}

void RibbonButton::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bgColor = palette().window().color();
    QColor fgColor = palette().windowText().color();

    if (m_style == Style::Primary) {
        bgColor = QColor("#E63946");
        fgColor = Qt::white;
    } else if (m_style == Style::Danger) {
        fgColor = QColor("#FF4444");
    } else if (m_style == Style::Success) {
        bgColor = QColor("#28A745");
        fgColor = Qt::white;
    } else if (m_style == Style::Warning) {
        bgColor = QColor("#FFC107");
        fgColor = Qt::black;
    }

    if (isDown()) {
        bgColor = bgColor.darker(110);
    } else if (hovering) {
        bgColor = bgColor.lighter(110);
    }

    p.fillRect(rect(), bgColor);
    QToolButton::paintEvent(event);
}

void RibbonButton::enterEvent(QEnterEvent* e) { hovering = true; QToolButton::enterEvent(e); update(); }
void RibbonButton::leaveEvent(QEvent* e) { hovering = false; QToolButton::leaveEvent(e); update(); }

// ==================== RibbonGroup ====================

RibbonGroup::RibbonGroup(QWidget* parent)
    : QFrame(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
}

RibbonGroup::RibbonGroup(const QString& title, QWidget* parent)
    : RibbonGroup(parent)
{
    setTitle(title);
}

void RibbonGroup::setTitle(const QString& title) {
    m_title = title;
    update();
}

RibbonButton* RibbonGroup::addButton(const QString& text) {
    auto* btn = new RibbonButton(text, this);
    m_layout->addWidget(btn);
    m_widgets.append(btn);
    return btn;
}

RibbonButton* RibbonGroup::addButton(const QIcon& icon, const QString& text) {
    auto* btn = new RibbonButton(icon, text, this);
    m_layout->addWidget(btn);
    m_widgets.append(btn);
    return btn;
}

void RibbonGroup::addWidget(QWidget* widget) {
    m_layout->addWidget(widget);
    m_widgets.append(widget);
}

void RibbonGroup::addSeparator() {
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("background: #4A1520;");
    sep->setFixedWidth(1);
    m_layout->addWidget(sep);
}

void RibbonGroup::addStretch() {
    m_layout->addStretch();
}

void RibbonGroup::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (!m_title.isEmpty()) {
        p.setPen(QColor("#F08080"));
        p.setFont(QFont("Segoe UI", 9));
        QRect labelRect(0, rect().bottom() - 16, rect().width(), 14);
        p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter, m_title);
    }
}

// ==================== RibbonPanel ====================

RibbonPanel::RibbonPanel(QWidget* parent)
    : QFrame(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(8);
    setFixedHeight(RibbonStyle::PANEL_HEIGHT);
    setFrameShape(QFrame::NoFrame);
}

RibbonPanel::RibbonPanel(const QString& title, QWidget* parent)
    : RibbonPanel(parent)
{
    setTitle(title);
}

void RibbonPanel::setTitle(const QString& title) {
    m_title = title;
    update();
}

RibbonGroup* RibbonPanel::addGroup(const QString& title) {
    auto* group = new RibbonGroup(title, this);
    m_layout->addWidget(group);
    m_groups.append(group);
    return group;
}

void RibbonPanel::addGroup(RibbonGroup* group) {
    m_layout->addWidget(group);
    m_groups.append(group);
}

void RibbonPanel::addStretch() {
    m_layout->addStretch();
}

void RibbonPanel::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), palette().window().color());
}

// ==================== RibbonSubTabBar ====================

RibbonSubTabBar::RibbonSubTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setDrawBase(false);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    setMovable(false);
    setDocumentMode(false);
}

void RibbonSubTabBar::setTabIcon(int index, const QIcon& icon) {
    if (index >= 0 && index < count()) {
        QTabBar::setTabIcon(index, icon);
    }
}

void RibbonSubTabBar::applyTheme(const RibbonTheme& theme) {
    setStyleSheet(QString(R"(
        QTabBar::tab {
            background: %1;
            color: %2;
            padding: 6px 16px;
            border: none;
            border-bottom: 2px solid transparent;
            font-weight: 500;
            font-size: 11px;
            min-width: 80px;
            margin-right: 2px;
        }
        QTabBar::tab:hover {
            background: %3;
            color: %4;
        }
        QTabBar::tab:selected {
            background: %5;
            color: %6;
            border-bottom: 2px solid %7;
        }
    )").arg(theme.background.name())
        .arg(theme.groupLabel.name())
        .arg(theme.buttonHover.name())
        .arg(theme.accent.name())
        .arg(theme.panelBg.name())
        .arg(theme.titleBarText.name())
        .arg(theme.primary.name()));
}

void RibbonSubTabBar::paintEvent(QPaintEvent* event) {
    QStylePainter painter(this);
    QStyleOptionTab opt;

    for (int i = 0; i < count(); ++i) {
        initStyleOption(&opt, i);
        if (!tabIcon(i).isNull()) {
            opt.icon = tabIcon(i);
        }
        painter.drawControl(QStyle::CE_TabBarTab, opt);
    }
}

// ==================== RibbonSubTab ====================

RibbonSubTab::RibbonSubTab(QWidget* parent)
    : QWidget(parent)
{
    m_contentWidget = new QWidget(this);
    m_contentLayout = new QHBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_contentWidget);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_scrollArea);
}

RibbonSubTab::RibbonSubTab(const QString& title, const QIcon& icon, QWidget* parent)
    : RibbonSubTab(parent)
{
    setTitle(title);
    setIcon(icon);
}

RibbonSubTab::~RibbonSubTab() {
}

void RibbonSubTab::setTitle(const QString& title) {
    m_title = title;
}

void RibbonSubTab::setIcon(const QIcon& icon) {
    m_icon = icon;
}

RibbonPanel* RibbonSubTab::addPanel(const QString& title) {
    auto* panel = new RibbonPanel(title, m_contentWidget);
    addPanel(panel);
    return panel;
}

void RibbonSubTab::addPanel(RibbonPanel* panel) {
    m_panels.append(panel);
    m_contentLayout->addWidget(panel);
    adjustPanelHeights();
}

void RibbonSubTab::insertPanel(int index, RibbonPanel* panel) {
    m_panels.insert(index, panel);
    m_contentLayout->insertWidget(index, panel);
    adjustPanelHeights();
}

void RibbonSubTab::setTabColor(const QColor& color) {
    m_tabColor = color;
    update();
}

void RibbonSubTab::applyTheme(const RibbonTheme& theme) {
    setTabColor(theme.primary);
    setStyleSheet(QString(R"(
        RibbonSubTab { background: %1; }
        RibbonPanel { background: %2; border-right: 1px solid %3; }
        RibbonGroup QLabel { color: %4; }
        QScrollArea { background: transparent; }
    )").arg(theme.background.name())
        .arg(theme.panelBg.name())
        .arg(theme.borderColor.name())
        .arg(theme.groupLabel.name()));
}

void RibbonSubTab::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), m_tabColor.isValid() ? m_tabColor : palette().window().color());
}

void RibbonSubTab::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateScrollArea();
    adjustPanelHeights();
}

void RibbonSubTab::updateScrollArea() {
    if (m_scrollArea && m_contentWidget) {
        int totalWidth = 0;
        for (auto* panel : m_panels) {
            totalWidth += panel->width();
        }
        m_contentWidget->setMinimumWidth(qMax(totalWidth, width()));
    }
}

void RibbonSubTab::adjustPanelHeights() {
    const int panelH = (height() > 10) ? height() - 10 : RibbonStyle::PANEL_HEIGHT;
    for (auto* panel : m_panels) {
        panel->setFixedHeight(panelH);
    }
}

// ==================== RibbonTab ====================

RibbonTab::RibbonTab(QWidget* parent)
    : QWidget(parent)
{
    // Sub-tab bar
    m_subTabBar = new RibbonSubTabBar(this);
    m_subTabBar->setFixedHeight(30);

    // Stacked widget for sub-tabs
    m_subTabStack = new QStackedWidget(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_subTabBar);
    mainLayout->addWidget(m_subTabStack);

    connect(m_subTabBar, &QTabBar::currentChanged, this, &RibbonTab::onSubTabChanged);
}

RibbonTab::RibbonTab(const QString& title, QWidget* parent)
    : RibbonTab(parent)
{
    setTitle(title);
}

RibbonTab::~RibbonTab() {
}

void RibbonTab::setTitle(const QString& title) {
    m_title = title;
}

void RibbonTab::setIcon(const QIcon& icon) {
    m_icon = icon;
}

RibbonSubTab* RibbonTab::addSubTab(const QString& title, const QIcon& icon) {
    auto* subTab = new RibbonSubTab(title, icon, this);
    insertSubTab(m_subTabs.size(), subTab);
    return subTab;
}

void RibbonTab::insertSubTab(int index, RibbonSubTab* subTab) {
    m_subTabs.insert(index, subTab);
    m_subTabBar->insertTab(index, subTab->title());
    if (!subTab->icon().isNull()) {
        m_subTabBar->setTabIcon(index, subTab->icon());
    }
    m_subTabStack->insertWidget(index, subTab);
    emit subTabChanged(index);
}

void RibbonTab::removeSubTab(int index) {
    if (index < 0 || index >= m_subTabs.size()) return;
    auto* subTab = m_subTabs.takeAt(index);
    m_subTabBar->removeTab(index);
    m_subTabStack->removeWidget(subTab);
    delete subTab;
}

RibbonSubTab* RibbonTab::subTab(int index) const {
    if (index >= 0 && index < m_subTabs.size()) {
        return m_subTabs[index];
    }
    return nullptr;
}

int RibbonTab::subTabCount() const {
    return m_subTabs.size();
}

int RibbonTab::currentSubTabIndex() const {
    return m_subTabBar->currentIndex();
}

void RibbonTab::setCurrentSubTabIndex(int index) {
    if (index >= 0 && index < m_subTabBar->count()) {
        m_subTabBar->setCurrentIndex(index);
        m_subTabStack->setCurrentIndex(index);
    }
}

RibbonPanel* RibbonTab::addPanel(const QString& title) {
    // For backward compatibility - add to first sub-tab or create one
    if (m_subTabs.isEmpty()) {
        addSubTab("General", QIcon());
    }
    return m_subTabs.first()->addPanel(title);
}

void RibbonTab::addPanel(RibbonPanel* panel) {
    if (m_subTabs.isEmpty()) {
        addSubTab("General", QIcon());
    }
    m_subTabs.first()->addPanel(panel);
}

void RibbonTab::insertPanel(int index, RibbonPanel* panel) {
    if (m_subTabs.isEmpty()) {
        addSubTab("General", QIcon());
    }
    m_subTabs.first()->insertPanel(index, panel);
}

void RibbonTab::setTabColor(const QColor& color) {
    m_tabColor = color;
    update();
}

void RibbonTab::applyTheme(const RibbonTheme& theme) {
    setTabColor(theme.primary);
    m_subTabBar->applyTheme(theme);
    for (auto* subTab : m_subTabs) {
        subTab->applyTheme(theme);
    }
    setStyleSheet(QString(R"(
        RibbonTab { background: %1; }
        RibbonPanel { background: %2; border-right: 1px solid %3; }
        RibbonGroup QLabel { color: %4; }
        QScrollArea { background: transparent; }
        RibbonSubTabBar { background: %1; }
    )").arg(theme.background.name())
        .arg(theme.panelBg.name())
        .arg(theme.borderColor.name())
        .arg(theme.groupLabel.name()));
}

void RibbonTab::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), m_tabColor.isValid() ? m_tabColor : palette().window().color());
}

void RibbonTab::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void RibbonTab::onSubTabChanged(int index) {
    m_subTabStack->setCurrentIndex(index);
    emit subTabChanged(index);
}

// ==================== RibbonBar ====================

RibbonBar::RibbonBar(QWidget* parent)
    : QWidget(parent)
{
    m_tabBar = new QTabBar(this);
    m_tabBar->setDrawBase(false);
    m_tabBar->setExpanding(false);
    m_tabBar->setIconSize(QSize(20, 20));
    m_tabBar->setElideMode(Qt::ElideRight);

    m_stackedWidget = new QStackedWidget(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_tabBar);
    mainLayout->addWidget(m_stackedWidget);

    connect(m_tabBar, &QTabBar::currentChanged, this, &RibbonBar::onTabClicked);
}

RibbonBar::~RibbonBar() {
}

void RibbonBar::addTab(RibbonTab* tab) {
    int index = m_tabBar->addTab(tab->title());
    if (!tab->icon().isNull()) {
        m_tabBar->setTabIcon(index, tab->icon());
    }
    m_stackedWidget->addWidget(tab);
    emit tabAdded(index);
}

void RibbonBar::insertTab(int index, RibbonTab* tab) {
    int idx = m_tabBar->insertTab(index, tab->title());
    if (!tab->icon().isNull()) {
        m_tabBar->setTabIcon(idx, tab->icon());
    }
    m_stackedWidget->insertWidget(index, tab);
    emit tabAdded(index);
}

void RibbonBar::removeTab(int index) {
    auto* tab = m_stackedWidget->widget(index);
    m_tabBar->removeTab(index);
    m_stackedWidget->removeWidget(tab);
    delete tab;
    emit tabRemoved(index);
}

RibbonTab* RibbonBar::tab(int index) const {
    return qobject_cast<RibbonTab*>(m_stackedWidget->widget(index));
}

RibbonTab* RibbonBar::currentTab() const {
    return tab(currentIndex());
}

int RibbonBar::currentIndex() const {
    return m_tabBar->currentIndex();
}

int RibbonBar::tabCount() const {
    return m_tabBar->count();
}

void RibbonBar::setCurrentIndex(int index) {
    m_tabBar->setCurrentIndex(index);
    m_stackedWidget->setCurrentIndex(index);
    emit currentChanged(index);
}

void RibbonBar::setCurrentTab(RibbonTab* tab) {
    int index = m_stackedWidget->indexOf(tab);
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

void RibbonBar::onTabClicked(int index) {
    m_stackedWidget->setCurrentIndex(index);
    emit currentChanged(index);
}

void RibbonBar::updateLayout() {
    updateGeometry();
}

void RibbonBar::setTabIcon(int index, const QIcon& icon) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setTabIcon(index, icon);
    }
}

void RibbonBar::applyTheme(const QString& themeKey) {
    RibbonThemeManager& manager = RibbonThemeManager::instance();
    manager.applyTheme(themeKey);

    const RibbonTheme& theme = manager.theme(themeKey);

    m_tabBar->setStyleSheet(QString(R"(
        QTabBar::tab {
            background: %1;
            color: %2;
            padding: 8px 24px;
            border: none;
            border-bottom: 3px solid transparent;
            font-weight: bold;
            font-size: 12px;
        }
        QTabBar::tab:hover {
            background: %3;
            color: %4;
        }
        QTabBar::tab:selected {
            background: %5;
            color: %6;
            border-bottom: 3px solid %7;
        }
    )").arg(theme.background.name())
        .arg(theme.groupLabel.name())
        .arg(theme.buttonHover.name())
        .arg(theme.accent.name())
        .arg(theme.panelBg.name())
        .arg(theme.titleBarText.name())
        .arg(theme.primary.name()));

    for (int i = 0; i < tabCount(); ++i) {
        if (RibbonTab* t = tab(i)) {
            t->applyTheme(theme);
        }
    }

    emit themeChanged(themeKey);
}

void RibbonBar::setTabHighlight(int index, bool highlight) {
    if (index < 0 || index >= m_tabBar->count()) return;

    if (highlight) {
        RibbonThemeManager& manager = RibbonThemeManager::instance();
        QStringList keys = manager.themeKeys();
        if (index < keys.size()) {
            const RibbonTheme& theme = manager.theme(keys[index]);
            m_tabBar->setTabTextColor(index, theme.accent);
        }
    } else {
        m_tabBar->setTabTextColor(index, QColor("#E0D0A0"));
    }
}

// ==================== RibbonWindowManager ====================

RibbonWindowManager::RibbonWindowManager(QWidget* parent)
    : QObject(parent)
{
    m_ribbonBar = new RibbonBar(parent);
    setupRibbon();
}

RibbonWindowManager::~RibbonWindowManager() {
    delete m_ribbonBar;
}

void RibbonWindowManager::setMode(Mode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        updateVisibility();
        emit modeChanged(mode);
    }
}

void RibbonWindowManager::onModeChanged(int index) {
    setMode(static_cast<Mode>(index));
}

void RibbonWindowManager::setupRibbon() {
    auto* commonTab = new RibbonTab("Common", m_ribbonBar);
    auto* commonPanel = new RibbonPanel("Tools", commonTab);
    commonPanel->addGroup("Edit");
    m_ribbonBar->addTab(commonTab);

    auto* carTab = new RibbonTab("Car", m_ribbonBar);
    m_ribbonBar->addTab(carTab);

    auto* trackTab = new RibbonTab("Track", m_ribbonBar);
    m_ribbonBar->addTab(trackTab);

    auto* characterTab = new RibbonTab("Character", m_ribbonBar);
    m_ribbonBar->addTab(characterTab);
}

void RibbonWindowManager::updateVisibility() {
    // Visibility logic based on mode
}

void RibbonButton::mousePressEvent(QMouseEvent* event)
{
    QToolButton::mousePressEvent(event);
}

} // namespace editor
} // namespace ks
