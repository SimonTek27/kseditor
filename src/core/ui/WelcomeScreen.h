#pragma once
#include <QDialog>
#include <QSettings>
#include <QListWidget>

class WelcomeScreen : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeScreen(QWidget* parent = nullptr);

    QString launchMode;
    QString recentProjectPath;

    void launchApp(const QString& mode);

private slots:
    void onHelpClicked();
    void onRecentItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void loadRecentProjects();
    static QIcon makeWhiteIcon(const QString& resourcePath, int size = 32);

    QPoint m_dragPos;
    QListWidget* m_recentList = nullptr;
    QSettings* m_settings = nullptr;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
