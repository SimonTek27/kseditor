#pragma once
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QLabel;

class WelcomeScreen : public QDialog {
    Q_OBJECT
public:
    enum Action { None, New, Open, Recent };

    explicit WelcomeScreen(QWidget* parent = nullptr);

    Action selectedAction = None;
    QString recentPath;
    QLabel* m_statusLabel = nullptr;

private slots:
    void onNewClicked();
    void onOpenClicked();
    void onRecentDoubleClicked(QListWidgetItem* item);
    void onRecentContextMenu(const QPoint& pos);

private:
    void setupUI();
    void populateRecentProjects();
    void updateProjectCount();

    QListWidget* m_recentList;
    QLabel* m_countLabel;
};
