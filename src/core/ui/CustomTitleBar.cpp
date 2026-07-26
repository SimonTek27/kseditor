#include "CustomTitleBar.h"
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QWindow>

CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
{
    m_window = window();
    if (m_window) {
        m_window->installEventFilter(this);
    }
    setupUI();
    setupConnections();
    setFixedHeight(32);
    setAttribute(Qt::WA_StyledBackground, true);
}

void CustomTitleBar::setupUI() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(8);

    // App icon button
    m_menuButton = new QToolButton(this);
    m_menuButton->setFixedSize(28, 28);
    m_menuButton->setIcon(QApplication::windowIcon());
    m_menuButton->setToolTip("Menu");
    m_menuButton->setCursor(Qt::ArrowCursor);
    m_menuButton->setPopupMode(QToolButton::InstantPopup);

    // Title
    m_titleLabel = new QLabel(this);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Help (?)
    m_helpButton = new QToolButton(this);
    m_helpButton->setFixedSize(28, 28);
    m_helpButton->setText("?");
    m_helpButton->setToolTip("Help");

    // Minimize
    m_minimizeButton = new QToolButton(this);
    m_minimizeButton->setFixedSize(28, 28);
    m_minimizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    m_minimizeButton->setToolTip("Minimize");

    // Maximize/Restore
    m_maximizeButton = new QToolButton(this);
    m_maximizeButton->setFixedSize(28, 28);
    updateMaximizeButton();
    m_maximizeButton->setToolTip("Maximize");

    // Close
    m_closeButton = new QToolButton(this);
    m_closeButton->setFixedSize(28, 28);
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_closeButton->setToolTip("Close");

    layout->addWidget(m_menuButton);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_helpButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    // Default theme (dark)
    applyTheme(QColor("#1E1E1E"), QColor("#3C3C3C"), QColor("#E0E0E0"), 
               QColor("#3E3E42"), QColor("#007ACC"), QColor("#E81123"));
}

void CustomTitleBar::setupConnections() {
    connect(m_menuButton, &QToolButton::clicked, this, [this]() {
        if (m_menu) {
            m_menu->exec(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
        } else {
            emit menuRequested();
        }
    });
    connect(m_helpButton, &QToolButton::clicked, this, &CustomTitleBar::helpRequested);
    connect(m_minimizeButton, &QToolButton::clicked, this, &CustomTitleBar::minimizeRequested);
    connect(m_maximizeButton, &QToolButton::clicked, this, &CustomTitleBar::maximizeRequested);
    connect(m_closeButton, &QToolButton::clicked, this, &CustomTitleBar::closeRequested);
}

void CustomTitleBar::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void CustomTitleBar::setWindowIcon(const QIcon& icon) {
    if (!icon.isNull()) {
        m_menuButton->setIcon(icon);
    }
}

void CustomTitleBar::setMenu(QMenu* menu) {
    m_menu = menu;
    m_menuButton->setMenu(menu);
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPos = event->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        m_window->move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    event->accept();
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit maximizeRequested();
    }
}

bool CustomTitleBar::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_window) {
        if (event->type() == QEvent::WindowStateChange) {
            updateMaximizeButton();
        } else if (event->type() == QEvent::WindowTitleChange) {
            setTitle(m_window->windowTitle());
        }
    }
    return QWidget::eventFilter(obj, event);
}

void CustomTitleBar::updateMaximizeButton() {
    if (m_window && m_window->isMaximized()) {
        m_maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        m_maximizeButton->setToolTip("Restore");
    } else {
        m_maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
        m_maximizeButton->setToolTip("Maximize");
    }
}

void CustomTitleBar::applyTheme(const QColor& background, const QColor& border, const QColor& text,
                                const QColor& buttonHover, const QColor& buttonPressed, const QColor& closeHover) {
    m_titleLabel->setStyleSheet(QString("color: %1; font-weight: 500; font-size: 12px;").arg(text.name()));
    
    m_closeButton->setStyleSheet(QString("QToolButton:hover { background: %1; border-radius: 2px; }").arg(closeHover.name()));
    
    setStyleSheet(QString(R"(
        CustomTitleBar {
            background: %1;
            border-bottom: 1px solid %2;
        }
        QToolButton {
            background: transparent;
            border: none;
            border-radius: 2px;
            padding: 4px;
        }
        QToolButton:hover {
            background: %3;
        }
        QToolButton:pressed {
            background: %4;
        }
        QLabel {
            background: transparent;
        }
    )").arg(background.name(), border.name(), buttonHover.name(), buttonPressed.name()));
}