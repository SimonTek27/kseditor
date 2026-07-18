#include "RibbonTab.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

namespace ks {
namespace editor {

RibbonButton::RibbonButton(const QString& text, QWidget* parent)
    : QToolButton(parent)
{
    setText(text);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setFixedWidth(80);
    setAutoRaise(true);
}

void RibbonButton::setStyle(Style style)
{
    switch (style) {
        case Style::Normal:
            setStyleSheet("QToolButton { color: black; }");
            break;
        case Style::Primary:
            setStyleSheet("QToolButton { color: #0078D7; font-weight: bold; }");
            break;
        case Style::Danger:
            setStyleSheet("QToolButton { color: #E81123; font-weight: bold; }");
            break;
    }
}

RibbonGroup::RibbonGroup(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);
    
    if (!title.isEmpty()) {
        QLabel* label = new QLabel(title, this);
        label->setAlignment(Qt::AlignHCenter);
        label->setStyleSheet("font-size: 9px; color: #666;");
        m_layout->addWidget(label);
    }
    m_layout->addStretch();
}

QToolButton* RibbonGroup::addButton(const QIcon& icon, const QString& text)
{
    RibbonButton* btn = new RibbonButton(text, this);
    if (!icon.isNull()) btn->setIcon(icon);
    btn->setIconSize(QSize(24, 24));
    m_layout->insertWidget(m_layout->count() - 1, btn);
    return btn;
}

void RibbonGroup::addWidget(QWidget* widget)
{
    m_layout->insertWidget(m_layout->count() - 1, widget);
}

RibbonPanel::RibbonPanel(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(8);
    
    QLabel* label = new QLabel(title, this);
    label->setStyleSheet("font-weight: bold; font-size: 11px; color: #333;");
    m_layout->addWidget(label);
}

RibbonGroup* RibbonPanel::addGroup(const QString& name)
{
    RibbonGroup* group = new RibbonGroup(name, this);
    m_layout->addWidget(group);
    return group;
}

RibbonSubTab::RibbonSubTab(const QString& title, const QIcon& icon, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_icon(icon)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
}

RibbonPanel* RibbonSubTab::addPanel(const QString& title)
{
    RibbonPanel* panel = new RibbonPanel(title, this);
    m_layout->addWidget(panel);
    return panel;
}

RibbonTab::RibbonTab(const QString& title, const QIcon& icon, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_icon(icon)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
}

RibbonSubTab* RibbonTab::addSubTab(const QString& title, const QIcon& icon)
{
    RibbonSubTab* subTab = new RibbonSubTab(title, icon, this);
    m_layout->addWidget(subTab);
    m_subTabs.append(subTab);
    return subTab;
}

} // namespace editor
} // namespace ks