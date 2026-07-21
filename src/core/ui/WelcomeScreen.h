#pragma once
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QLabel;

class WelcomeScreen : public QDialog {
    Q_OBJECT
public:
    enum Action { None, New, NewBlank, Open, Help, Recent };

    explicit WelcomeScreen(QWidget* parent = nullptr);

    Action selectedAction = None;
    QString recentPath;
    QLabel* m_statusLabel = nullptr;

private slots:
    void onNewClicked();
    void onNewBlankClicked();
    void onOpenClicked();
    void onHelpClicked();
    void onRecentDoubleClicked(QListWidgetItem* item);
    void onRecentContextMenu(const QPoint& pos);

private:
    void setupUI();
    void populateRecentProjects();
    void updateProjectCount();

    QListWidget* m_recentList;
    QLabel* m_countLabel;
    QPoint m_dragPos;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
