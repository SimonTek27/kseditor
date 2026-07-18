#pragma once

#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

class CustomTitleBar : public QWidget {
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget* parent = nullptr);
    
    void setTitle(const QString& title);
    void setWindowIcon(const QIcon& icon);
    void setMenu(QMenu* menu);
    void applyTheme(const QColor& background, const QColor& border, const QColor& text, 
                    const QColor& buttonHover, const QColor& buttonPressed, const QColor& closeHover);
    
    QToolButton* menuButton() const { return m_menuButton; }
    QToolButton* minimizeButton() const { return m_minimizeButton; }
    QToolButton* maximizeButton() const { return m_maximizeButton; }
    QToolButton* closeButton() const { return m_closeButton; }

signals:
    void menuRequested();
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void updateMaximizeButton();
    
    QToolButton* m_menuButton = nullptr;
    QLabel* m_titleLabel = nullptr;
    QToolButton* m_minimizeButton = nullptr;
    QToolButton* m_maximizeButton = nullptr;
    QToolButton* m_closeButton = nullptr;
    QMenu* m_menu = nullptr;
    
    QPoint m_dragPos;
    bool m_dragging = false;
    QWidget* m_window = nullptr;
};