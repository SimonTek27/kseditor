#pragma once

#include <QWidget>
#include <QString>
#include <QIcon>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QVector>

namespace ks {
namespace editor {

class RibbonButton : public QToolButton {
public:
    enum class Style { Normal, Primary, Danger };
    explicit RibbonButton(const QString& text, QWidget* parent = nullptr);
    void setStyle(Style style);
};

class RibbonGroup : public QWidget {
public:
    explicit RibbonGroup(const QString& title, QWidget* parent = nullptr);
    QToolButton* addButton(const QIcon& icon, const QString& text);
    void addWidget(QWidget* widget);
private:
    QVBoxLayout* m_layout;
};

class RibbonPanel : public QWidget {
public:
    explicit RibbonPanel(const QString& title, QWidget* parent = nullptr);
    RibbonGroup* addGroup(const QString& name);
private:
    QHBoxLayout* m_layout;
    QString m_title;
};

class RibbonSubTab : public QWidget {
public:
    explicit RibbonSubTab(const QString& title, const QIcon& icon = QIcon(), QWidget* parent = nullptr);
    RibbonPanel* addPanel(const QString& title);
private:
    QVBoxLayout* m_layout;
    QString m_title;
    QIcon m_icon;
};

class RibbonTab : public QWidget {
public:
    explicit RibbonTab(const QString& title, const QIcon& icon = QIcon(), QWidget* parent = nullptr);
    RibbonSubTab* addSubTab(const QString& title, const QIcon& icon = QIcon());
private:
    QVBoxLayout* m_layout;
    QString m_title;
    QIcon m_icon;
    QVector<RibbonSubTab*> m_subTabs;
};

} // namespace editor
} // namespace ks