#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QSettings>
#include <QGuiApplication>

class WelcomeScreenQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString launchMode READ launchMode NOTIFY completed)
    Q_PROPERTY(QString recentProjectPath READ recentProjectPath NOTIFY completed)
    Q_PROPERTY(QVariantList recentProjects READ recentProjects CONSTANT)

public:
    explicit WelcomeScreenQmlBridge(QObject* parent = nullptr);

    QString launchMode() const { return m_launchMode; }
    QString recentProjectPath() const { return m_recentProjectPath; }
    QVariantList recentProjects() const { return m_recentProjects; }

    Q_INVOKABLE void launchApp(const QString& mode);
    Q_INVOKABLE void openRecent(const QString& path);
    Q_INVOKABLE void openHelp();

signals:
    void completed();
    void helpRequested();

private:
    void loadRecentProjects();

    QString m_launchMode;
    QString m_recentProjectPath;
    QVariantList m_recentProjects;
    QSettings m_settings{ QGuiApplication::organizationName(), QGuiApplication::applicationName() };
};
